#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

#include "service/service.h"

namespace mlcp::hmi::service::temp {

enum class TempServiceState {
    stopped,
    running,
    degraded,
};

struct TempServiceConfig {
    bool temperatureAutoControl {true};
    double targetCelsius {37.0};
    double toleranceCelsius {0.5};
    double proportionalGain {32.0};
    int manualFanDuty {0};
    int minFanDuty {0};
    int maxFanDuty {255};
    std::chrono::milliseconds controlInterval {std::chrono::milliseconds(500)};
};

struct TempServiceSnapshot {
    TempServiceState state {TempServiceState::stopped};
    bool temperatureAutoControl {true};
    double targetCelsius {37.0};
    double toleranceCelsius {0.5};
    int manualFanDuty {0};
    int minFanDuty {0};
    int maxFanDuty {255};
    std::optional<double> latestTemperatureCelsius;
    int latestFanDuty {0};
    std::uint64_t controlCount {0};
    std::string lastError;
};

class TempService : public mlcp::hmi::service::IService {
public:
    using TemperatureReader = std::function<double()>;
    using FanDutyWriter = std::function<void(int)>;
    using StateReporter = std::function<void(TempServiceState)>;

    TempService(TemperatureReader temperatureReader,
                FanDutyWriter fanDutyWriter,
                StateReporter stateReporter = {});

    const char* name() const override;
    void start() override;
    void stop() override;
    void tick() override;
    std::vector<mlcp::hmi::system::RuntimePeriodicTask> runtimeTasks() override;
    void setTargetCelsius(double targetCelsius);
    void setConfig(const TempServiceConfig& config);
    TempServiceConfig config() const;
    TempServiceSnapshot snapshot() const;

private:
    int calculateFanDuty(double temperatureCelsius) const;
    bool isControlDueLocked(std::chrono::steady_clock::time_point now) const;
    void reportState(TempServiceState state);

    TemperatureReader temperatureReader_;
    FanDutyWriter fanDutyWriter_;
    StateReporter stateReporter_;
    mutable std::mutex mutex_;
    TempServiceConfig config_;
    TempServiceState state_ {TempServiceState::stopped};
    std::optional<double> latestTemperatureCelsius_;
    int latestFanDuty_ {0};
    std::uint64_t controlCount_ {0};
    std::string lastError_;
    std::chrono::steady_clock::time_point nextControlAt_ {};
};

const char* tempServiceStateName(TempServiceState state);

} // namespace mlcp::hmi::service::temp
