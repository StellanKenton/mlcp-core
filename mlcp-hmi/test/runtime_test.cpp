#include "system/runtime/runtime.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

bool hasRole(const std::vector<mlcp::hmi::system::RuntimeThreadSnapshot>& snapshots,
             mlcp::hmi::system::RuntimeThreadRole role)
{
    return std::any_of(snapshots.begin(), snapshots.end(), [role](const auto& snapshot) {
        return snapshot.role == role;
    });
}

bool allStopped(const std::vector<mlcp::hmi::system::RuntimeThreadSnapshot>& snapshots)
{
    return std::all_of(snapshots.begin(), snapshots.end(), [](const auto& snapshot) {
        return snapshot.state == mlcp::hmi::system::RuntimeThreadState::stopped;
    });
}

mlcp::hmi::system::RuntimeThreadSnapshot findSnapshot(
    const std::vector<mlcp::hmi::system::RuntimeThreadSnapshot>& snapshots,
    mlcp::hmi::system::RuntimeThreadRole role)
{
    const auto iterator = std::find_if(snapshots.begin(), snapshots.end(),
                                       [role](const auto& snapshot) {
                                           return snapshot.role == role;
                                       });
    assert(iterator != snapshots.end());
    return *iterator;
}

mlcp::hmi::system::RuntimeTaskSnapshot findTaskSnapshot(
    const mlcp::hmi::system::RuntimeThreadSnapshot& threadSnapshot,
    const std::string& taskName)
{
    const auto iterator =
        std::find_if(threadSnapshot.tasks.begin(), threadSnapshot.tasks.end(),
                     [&taskName](const auto& taskSnapshot) {
                         return taskSnapshot.name == taskName;
                     });
    assert(iterator != threadSnapshot.tasks.end());
    return *iterator;
}

} // namespace

int main()
{
    mlcp::hmi::system::Runtime runtime;
    std::atomic<std::uint64_t> ioPeriodicTaskCount {0U};
    std::atomic<std::uint64_t> monitorPeriodicTaskCount {0U};
    std::atomic<std::uint64_t> failingTaskCount {0U};
    std::atomic<std::uint64_t> servicePeriodicTaskCount {0U};
    std::atomic<std::uint64_t> ioTaskCount {0U};

    runtime.registerPeriodicTask(mlcp::hmi::system::RuntimePeriodicTask{
        "ioPeriodicTask",
        mlcp::hmi::system::RuntimeThreadRole::io,
        std::chrono::milliseconds(10),
        [&ioPeriodicTaskCount]() {
            ++ioPeriodicTaskCount;
        },
    });
    runtime.registerPeriodicTask(mlcp::hmi::system::RuntimePeriodicTask{
        "monitorPeriodicTask",
        mlcp::hmi::system::RuntimeThreadRole::monitor,
        std::chrono::milliseconds(20),
        [&monitorPeriodicTaskCount]() {
            ++monitorPeriodicTaskCount;
        },
    });
    runtime.registerPeriodicTask(mlcp::hmi::system::RuntimePeriodicTask{
        "failingTask",
        mlcp::hmi::system::RuntimeThreadRole::io,
        std::chrono::milliseconds(15),
        [&failingTaskCount]() {
            ++failingTaskCount;
            throw std::runtime_error("periodic task failed");
        },
    });
    runtime.registerPeriodicTask(mlcp::hmi::system::RuntimePeriodicTask{
        "servicePeriodicTask",
        mlcp::hmi::system::RuntimeThreadRole::service,
        std::chrono::milliseconds(10),
        [&servicePeriodicTaskCount]() {
            ++servicePeriodicTaskCount;
        },
    });

    const auto initialSnapshots = runtime.threadSnapshots();
    assert(initialSnapshots.size() == 4U);
    assert(hasRole(initialSnapshots, mlcp::hmi::system::RuntimeThreadRole::service));
    assert(hasRole(initialSnapshots, mlcp::hmi::system::RuntimeThreadRole::io));
    assert(hasRole(initialSnapshots, mlcp::hmi::system::RuntimeThreadRole::safety));
    assert(hasRole(initialSnapshots, mlcp::hmi::system::RuntimeThreadRole::monitor));
    assert(allStopped(initialSnapshots));

    runtime.start();
    assert(runtime.isRunning());

    std::thread::id serviceTaskThreadId;
    runtime.runServiceTaskAndWait([&serviceTaskThreadId]() {
        serviceTaskThreadId = std::this_thread::get_id();
    });
    assert(serviceTaskThreadId != std::this_thread::get_id());

    std::thread::id ioTaskThreadId;
    runtime.runIoTaskAndWait([&ioTaskThreadId, &ioTaskCount]() {
        ioTaskThreadId = std::this_thread::get_id();
        ++ioTaskCount;
    });
    assert(ioTaskThreadId != std::this_thread::get_id());
    assert(ioTaskCount == 1U);

    std::this_thread::sleep_for(std::chrono::milliseconds(260));

    const auto runningSnapshots = runtime.threadSnapshots();
    assert(runningSnapshots.size() == 4U);
    assert(std::all_of(runningSnapshots.begin(), runningSnapshots.end(), [](const auto& snapshot) {
        return snapshot.heartbeatCount > 0U;
    }));

    const auto ioSnapshot =
        findSnapshot(runningSnapshots, mlcp::hmi::system::RuntimeThreadRole::io);
    const auto serviceSnapshot =
        findSnapshot(runningSnapshots, mlcp::hmi::system::RuntimeThreadRole::service);
    const auto monitorSnapshot =
        findSnapshot(runningSnapshots, mlcp::hmi::system::RuntimeThreadRole::monitor);
    const auto ioTaskSnapshot = findTaskSnapshot(ioSnapshot, "ioPeriodicTask");
    const auto failingTaskSnapshot = findTaskSnapshot(ioSnapshot, "failingTask");
    const auto serviceTaskSnapshot = findTaskSnapshot(serviceSnapshot, "servicePeriodicTask");
    const auto monitorTaskSnapshot = findTaskSnapshot(monitorSnapshot, "monitorPeriodicTask");
    assert(ioPeriodicTaskCount > 0U);
    assert(ioTaskSnapshot.runCount > 0U);
    assert(ioTaskSnapshot.lastError.empty());
    assert(failingTaskCount > 0U);
    assert(failingTaskSnapshot.lastError == "periodic task failed");
    assert(monitorPeriodicTaskCount > 0U);
    assert(monitorTaskSnapshot.runCount > 0U);
    assert(servicePeriodicTaskCount > 0U);
    assert(serviceTaskSnapshot.runCount > 0U);

    runtime.unregisterPeriodicTask("ioPeriodicTask");
    const auto snapshotsAfterUnregister = runtime.threadSnapshots();
    const auto ioSnapshotAfterUnregister =
        findSnapshot(snapshotsAfterUnregister, mlcp::hmi::system::RuntimeThreadRole::io);
    assert(std::none_of(ioSnapshotAfterUnregister.tasks.begin(),
                        ioSnapshotAfterUnregister.tasks.end(),
                        [](const auto& taskSnapshot) {
                            return taskSnapshot.name == "ioPeriodicTask";
                        }));

    runtime.stop();
    assert(!runtime.isRunning());
    assert(allStopped(runtime.threadSnapshots()));

    runtime.start();
    assert(runtime.isRunning());
    runtime.stop();
    assert(!runtime.isRunning());

    return 0;
}
