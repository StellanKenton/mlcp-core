#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "service/service.h"

namespace mlcp::hmi::service::example {

enum class ExampleServiceState {
    stopped,
    running,
    degraded,
};

struct ExampleServiceConfig {
    bool enabled {true};
    double lowerLimit {0.0};
    double upperLimit {100.0};
    std::chrono::milliseconds controlInterval {std::chrono::milliseconds(500)};
};

struct ExampleServiceSnapshot {
    ExampleServiceState state {ExampleServiceState::stopped};
    bool enabled {true};
    double lowerLimit {0.0};
    double upperLimit {100.0};
    std::optional<double> latestValue;
    std::uint64_t tickCount {0U};
    std::string lastError;
};

class ExampleService : public mlcp::hmi::service::IService {
public:
    using ValueReader = std::function<double()>;
    using StateReporter = std::function<void(ExampleServiceState)>;

    ExampleService(ValueReader valueReader, StateReporter stateReporter = {});

    const char* name() const override;
    void start() override;
    void stop() override;
    void tick() override;
    std::vector<mlcp::hmi::system::RuntimePeriodicTask> runtimeTasks() override;

    void setConfig(const ExampleServiceConfig& config);
    ExampleServiceConfig config() const;
    ExampleServiceSnapshot snapshot() const;

private:
    bool isTickDueLocked(std::chrono::steady_clock::time_point now) const;
    void reportState(ExampleServiceState state);

    ValueReader valueReader_;
    StateReporter stateReporter_;
    mutable std::mutex mutex_;
    ExampleServiceConfig config_;
    ExampleServiceState state_ {ExampleServiceState::stopped};
    std::optional<double> latestValue_;
    std::uint64_t tickCount_ {0U};
    std::string lastError_;
    std::chrono::steady_clock::time_point nextTickAt_ {};
};

const char* exampleServiceStateName(ExampleServiceState state);

} // namespace mlcp::hmi::service::example
