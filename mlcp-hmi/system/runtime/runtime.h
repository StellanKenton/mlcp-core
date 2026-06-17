#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mlcp::hmi::system {

enum class RuntimeThreadRole {
    service,
    io,
    safety,
    monitor,
};

enum class RuntimeThreadState {
    stopped,
    starting,
    running,
    stopping,
    failed,
};

struct RuntimeTaskSnapshot {
    std::string name;
    RuntimeThreadRole role {RuntimeThreadRole::service};
    std::uint64_t runCount {0U};
    std::chrono::steady_clock::time_point lastRunAt {};
    std::string lastError;
};

struct RuntimePeriodicTask {
    std::string name;
    RuntimeThreadRole role {RuntimeThreadRole::service};
    std::chrono::milliseconds interval {std::chrono::milliseconds(1000)};
    std::function<void()> run;
};

struct RuntimeThreadSnapshot {
    RuntimeThreadRole role {RuntimeThreadRole::service};
    RuntimeThreadState state {RuntimeThreadState::stopped};
    std::string name;
    std::uint64_t heartbeatCount {0};
    std::chrono::steady_clock::time_point lastHeartbeat {};
    std::string lastError;
    std::vector<RuntimeTaskSnapshot> tasks;
};

class Runtime {
public:
    Runtime();
    ~Runtime();

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    void start();
    void stop();
    void postServiceTask(std::function<void()> task);
    void runServiceTaskAndWait(std::function<void()> task);
    void postIoTask(std::function<void()> task);
    void runIoTaskAndWait(std::function<void()> task);
    void registerPeriodicTask(RuntimePeriodicTask task);
    void unregisterPeriodicTask(const std::string& name);

    bool isRunning() const;
    std::vector<RuntimeThreadSnapshot> threadSnapshots() const;

private:
    struct RuntimeWorker;

    std::vector<std::unique_ptr<RuntimeWorker>> workers_;
};

} // namespace mlcp::hmi::system
