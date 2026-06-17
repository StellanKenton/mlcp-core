#include "system/runtime/runtime.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#endif

#include "service/log/logger.h"

namespace mlcp::hmi::system {

namespace {

constexpr std::chrono::milliseconds kServiceThreadInterval {25};
constexpr std::chrono::milliseconds kIoThreadInterval {50};
constexpr std::chrono::milliseconds kSafetyThreadInterval {50};
constexpr std::chrono::milliseconds kMonitorThreadInterval {200};

const char* roleName(RuntimeThreadRole role)
{
    switch (role) {
    case RuntimeThreadRole::service:
        return "service";
    case RuntimeThreadRole::io:
        return "io";
    case RuntimeThreadRole::safety:
        return "safety";
    case RuntimeThreadRole::monitor:
        return "monitor";
    }

    return "unknown";
}

#if defined(__linux__)
void applyCurrentThreadName(const std::string& name)
{
    constexpr std::size_t kLinuxThreadNameMaxSize = 15U;
    const std::string limitedName = name.substr(0U, kLinuxThreadNameMaxSize);
    static_cast<void>(pthread_setname_np(pthread_self(), limitedName.c_str()));
}
#else
void applyCurrentThreadName(const std::string&)
{
}
#endif

} // namespace

struct Runtime::RuntimeWorker {
    RuntimeWorker(RuntimeThreadRole role, std::chrono::milliseconds interval)
        : role(role),
          interval(interval),
          name(std::string("runtime-") + roleName(role))
    {
    }

    ~RuntimeWorker()
    {
        stop();
    }

    RuntimeWorker(const RuntimeWorker&) = delete;
    RuntimeWorker& operator=(const RuntimeWorker&) = delete;

    struct PeriodicTaskRecord {
        RuntimePeriodicTask task;
        RuntimeTaskSnapshot snapshot;
        std::chrono::steady_clock::time_point nextRunAt {};
    };

    void start()
    {
        std::lock_guard<std::mutex> lifecycleLock(lifecycleMutex);
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (state == RuntimeThreadState::running || state == RuntimeThreadState::starting) {
                return;
            }
        }

        if (thread.joinable()) {
            thread.join();
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            stopRequested.store(false);
            heartbeatCount = 0U;
            lastError.clear();
            state = RuntimeThreadState::starting;
        }

        thread = std::thread(&RuntimeWorker::run, this);
    }

    void stop()
    {
        std::lock_guard<std::mutex> lifecycleLock(lifecycleMutex);
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (state == RuntimeThreadState::stopped && !thread.joinable()) {
                return;
            }

            if (state != RuntimeThreadState::failed) {
                state = RuntimeThreadState::stopping;
            }
            stopRequested.store(true);
        }

        wakeup.notify_all();

        if (thread.joinable()) {
            thread.join();
        }

        std::lock_guard<std::mutex> lock(mutex);
        if (state != RuntimeThreadState::failed) {
            state = RuntimeThreadState::stopped;
        }
    }

    bool isRunning() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return state == RuntimeThreadState::running || state == RuntimeThreadState::starting;
    }

    RuntimeThreadSnapshot snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        RuntimeThreadSnapshot result;
        result.role = role;
        result.state = state;
        result.name = name;
        result.heartbeatCount = heartbeatCount;
        result.lastHeartbeat = lastHeartbeat;
        result.lastError = lastError;
        result.tasks.reserve(periodicTasks.size());
        for (const PeriodicTaskRecord& task : periodicTasks) {
            result.tasks.push_back(task.snapshot);
        }
        return result;
    }

    void registerPeriodicTask(RuntimePeriodicTask task)
    {
        if (task.name.empty()) {
            throw std::invalid_argument("runtime periodic task name is required");
        }
        if (!task.run) {
            throw std::invalid_argument("runtime periodic task handler is required");
        }
        if (task.interval.count() <= 0) {
            throw std::invalid_argument("runtime periodic task interval must be positive");
        }
        if (task.role != role) {
            throw std::invalid_argument("runtime periodic task role does not match worker role");
        }

        std::lock_guard<std::mutex> lock(mutex);
        const auto iterator = findPeriodicTaskLocked(task.name);
        if (iterator != periodicTasks.end()) {
            iterator->task = std::move(task);
            iterator->nextRunAt = std::chrono::steady_clock::now();
            iterator->snapshot.runCount = 0U;
            iterator->snapshot.lastRunAt = {};
            iterator->snapshot.lastError.clear();
        } else {
            PeriodicTaskRecord record;
            record.snapshot.name = task.name;
            record.snapshot.role = task.role;
            record.task = std::move(task);
            record.nextRunAt = std::chrono::steady_clock::now();
            periodicTasks.push_back(std::move(record));
        }
        wakeup.notify_all();
    }

    void unregisterPeriodicTask(const std::string& taskName)
    {
        std::lock_guard<std::mutex> lock(mutex);
        periodicTasks.erase(std::remove_if(periodicTasks.begin(), periodicTasks.end(),
                                           [&taskName](const PeriodicTaskRecord& record) {
                                               return record.snapshot.name == taskName;
                                           }),
                            periodicTasks.end());
    }

    bool postTask(std::function<void()> task)
    {
        if (!task) {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (state != RuntimeThreadState::running && state != RuntimeThreadState::starting) {
                return false;
            }
            postedTasks.push(std::move(task));
        }

        wakeup.notify_all();
        return true;
    }

    bool isCurrentThread() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return threadId != std::thread::id {} && threadId == std::this_thread::get_id();
    }

    void run()
    {
        applyCurrentThreadName(name);
        markRunning(std::this_thread::get_id());
        LOG_I("runtime thread started, role={}", roleName(role));

        try {
            runLoop();
            markStopped();
            LOG_I("runtime thread stopped, role={}", roleName(role));
        } catch (const std::exception& error) {
            markFailed(error.what());
            LOG_E("runtime thread failed, role={}, error={}", roleName(role), error.what());
        } catch (...) {
            markFailed("unknown exception");
            LOG_E("runtime thread failed, role={}, error=unknown exception", roleName(role));
        }
    }

    void runLoop()
    {
        while (!stopRequested.load()) {
            markHeartbeat();
            drainPostedTasks();
            runPeriodicTasks();

            std::unique_lock<std::mutex> lock(mutex);
            wakeup.wait_until(lock, nextWakeupAtLocked(), [this]() {
                return stopRequested.load() || !postedTasks.empty();
            });
        }
    }

    void drainPostedTasks()
    {
        std::queue<std::function<void()>> pendingTasks;
        {
            std::lock_guard<std::mutex> lock(mutex);
            pendingTasks.swap(postedTasks);
        }

        while (!pendingTasks.empty()) {
            try {
                pendingTasks.front()();
            } catch (const std::exception& error) {
                LOG_W("runtime posted task failed, role={}, error={}",
                      roleName(role),
                      error.what());
            } catch (...) {
                LOG_W("runtime posted task failed, role={}, error=unknown exception",
                      roleName(role));
            }
            pendingTasks.pop();
        }
    }

    struct DuePeriodicTask {
        std::string name;
        std::function<void()> run;
    };

    void runPeriodicTasks()
    {
        std::vector<DuePeriodicTask> dueTasks;
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto now = std::chrono::steady_clock::now();
            for (PeriodicTaskRecord& record : periodicTasks) {
                if (now < record.nextRunAt) {
                    continue;
                }

                DuePeriodicTask dueTask;
                dueTask.name = record.snapshot.name;
                dueTask.run = record.task.run;
                dueTasks.push_back(std::move(dueTask));
                record.nextRunAt = now + record.task.interval;
            }
        }

        for (const DuePeriodicTask& task : dueTasks) {
            try {
                task.run();
                markPeriodicTaskRun(task.name);
            } catch (const std::exception& error) {
                markPeriodicTaskError(task.name, error.what());
                LOG_W("runtime periodic task failed, role={}, name={}, error={}",
                      roleName(role),
                      task.name,
                      error.what());
            } catch (...) {
                markPeriodicTaskError(task.name, "unknown exception");
                LOG_W("runtime periodic task failed, role={}, name={}, error=unknown exception",
                      roleName(role),
                      task.name);
            }
        }
    }

    std::chrono::steady_clock::time_point nextWakeupAtLocked() const
    {
        auto nextWakeupAt = std::chrono::steady_clock::now() + interval;
        for (const PeriodicTaskRecord& record : periodicTasks) {
            nextWakeupAt = std::min(nextWakeupAt, record.nextRunAt);
        }
        return nextWakeupAt;
    }

    void markRunning(std::thread::id currentThreadId)
    {
        std::lock_guard<std::mutex> lock(mutex);
        threadId = currentThreadId;
        state = RuntimeThreadState::running;
        lastHeartbeat = std::chrono::steady_clock::now();
    }

    void markStopped()
    {
        std::lock_guard<std::mutex> lock(mutex);
        threadId = {};
        state = RuntimeThreadState::stopped;
    }

    void markFailed(const std::string& error)
    {
        std::lock_guard<std::mutex> lock(mutex);
        threadId = {};
        lastError = error;
        state = RuntimeThreadState::failed;
    }

    void markHeartbeat()
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++heartbeatCount;
        lastHeartbeat = std::chrono::steady_clock::now();
    }

    void markPeriodicTaskRun(const std::string& taskName)
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto iterator = findPeriodicTaskLocked(taskName);
        if (iterator == periodicTasks.end()) {
            return;
        }

        ++iterator->snapshot.runCount;
        iterator->snapshot.lastRunAt = std::chrono::steady_clock::now();
        iterator->snapshot.lastError.clear();
    }

    void markPeriodicTaskError(const std::string& taskName, const std::string& error)
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto iterator = findPeriodicTaskLocked(taskName);
        if (iterator == periodicTasks.end()) {
            return;
        }

        iterator->snapshot.lastError = error;
        iterator->snapshot.lastRunAt = std::chrono::steady_clock::now();
    }

    std::vector<PeriodicTaskRecord>::iterator findPeriodicTaskLocked(
        const std::string& taskName)
    {
        return std::find_if(periodicTasks.begin(), periodicTasks.end(),
                            [&taskName](const PeriodicTaskRecord& record) {
                                return record.snapshot.name == taskName;
                            });
    }

    RuntimeThreadRole role;
    std::chrono::milliseconds interval;
    std::string name;
    std::atomic<bool> stopRequested {false};
    std::mutex lifecycleMutex;
    mutable std::mutex mutex;
    std::condition_variable wakeup;
    std::thread thread;
    std::thread::id threadId {};
    RuntimeThreadState state {RuntimeThreadState::stopped};
    std::uint64_t heartbeatCount {0};
    std::chrono::steady_clock::time_point lastHeartbeat {};
    std::string lastError;
    std::vector<PeriodicTaskRecord> periodicTasks;
    std::queue<std::function<void()>> postedTasks;
};

Runtime::Runtime()
    : workers_()
{
    workers_.push_back(
        std::make_unique<RuntimeWorker>(RuntimeThreadRole::service, kServiceThreadInterval));
    workers_.push_back(
        std::make_unique<RuntimeWorker>(RuntimeThreadRole::io, kIoThreadInterval));
    workers_.push_back(
        std::make_unique<RuntimeWorker>(RuntimeThreadRole::safety, kSafetyThreadInterval));
    workers_.push_back(
        std::make_unique<RuntimeWorker>(RuntimeThreadRole::monitor, kMonitorThreadInterval));
}

Runtime::~Runtime()
{
    stop();
}

void Runtime::start()
{
    for (const auto& worker : workers_) {
        worker->start();
    }
}

void Runtime::stop()
{
    for (const auto& worker : workers_) {
        worker->stop();
    }
}

void Runtime::postServiceTask(std::function<void()> task)
{
    for (const auto& worker : workers_) {
        const RuntimeThreadSnapshot snapshot = worker->snapshot();
        if (snapshot.role == RuntimeThreadRole::service) {
            if (!worker->postTask(std::move(task))) {
                throw std::runtime_error("failed to post service task");
            }
            return;
        }
    }

    throw std::runtime_error("service runtime worker not found");
}

void Runtime::runServiceTaskAndWait(std::function<void()> task)
{
    if (!task) {
        return;
    }

    for (const auto& worker : workers_) {
        const RuntimeThreadSnapshot snapshot = worker->snapshot();
        if (snapshot.role == RuntimeThreadRole::service && worker->isCurrentThread()) {
            throw std::runtime_error("cannot wait for service task from service runtime thread");
        }
    }

    auto completion = std::make_shared<std::promise<void>>();
    std::future<void> future = completion->get_future();
    postServiceTask([task = std::move(task), completion]() mutable {
        try {
            task();
            completion->set_value();
        } catch (...) {
            completion->set_exception(std::current_exception());
        }
    });

    future.get();
}

void Runtime::postIoTask(std::function<void()> task)
{
    for (const auto& worker : workers_) {
        const RuntimeThreadSnapshot snapshot = worker->snapshot();
        if (snapshot.role == RuntimeThreadRole::io) {
            if (!worker->postTask(std::move(task))) {
                throw std::runtime_error("failed to post io task");
            }
            return;
        }
    }

    throw std::runtime_error("io runtime worker not found");
}

void Runtime::runIoTaskAndWait(std::function<void()> task)
{
    if (!task) {
        return;
    }

    for (const auto& worker : workers_) {
        const RuntimeThreadSnapshot snapshot = worker->snapshot();
        if (snapshot.role == RuntimeThreadRole::io && worker->isCurrentThread()) {
            throw std::runtime_error("cannot wait for io task from io runtime thread");
        }
    }

    auto completion = std::make_shared<std::promise<void>>();
    std::future<void> future = completion->get_future();
    postIoTask([task = std::move(task), completion]() mutable {
        try {
            task();
            completion->set_value();
        } catch (...) {
            completion->set_exception(std::current_exception());
        }
    });

    future.get();
}

void Runtime::registerPeriodicTask(RuntimePeriodicTask task)
{
    for (const auto& worker : workers_) {
        const RuntimeThreadSnapshot snapshot = worker->snapshot();
        if (snapshot.role == task.role) {
            worker->registerPeriodicTask(std::move(task));
            return;
        }
    }

    throw std::runtime_error("runtime periodic task worker not found");
}

void Runtime::unregisterPeriodicTask(const std::string& name)
{
    for (const auto& worker : workers_) {
        worker->unregisterPeriodicTask(name);
    }
}

bool Runtime::isRunning() const
{
    for (const auto& worker : workers_) {
        if (!worker->isRunning()) {
            return false;
        }
    }

    return !workers_.empty();
}

std::vector<RuntimeThreadSnapshot> Runtime::threadSnapshots() const
{
    std::vector<RuntimeThreadSnapshot> snapshots;
    snapshots.reserve(workers_.size());

    for (const auto& worker : workers_) {
        snapshots.push_back(worker->snapshot());
    }

    return snapshots;
}

} // namespace mlcp::hmi::system
