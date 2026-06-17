#include "service/temp/tempService.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <utility>

#include "service/log/logger.h"

namespace mlcp::hmi::service::temp {
namespace {

constexpr double kMinSupportedTargetCelsius = 15.0;
constexpr double kMaxSupportedTargetCelsius = 60.0;
constexpr double kMinToleranceCelsius = 0.1;
constexpr std::chrono::milliseconds kRuntimeTickInterval {25};
constexpr const char* kRuntimeTickTaskName = "service.temp.tick";

void validateConfig(const TempServiceConfig& config)
{
    if (config.targetCelsius < kMinSupportedTargetCelsius ||
        config.targetCelsius > kMaxSupportedTargetCelsius) {
        throw std::invalid_argument("temperature target is out of supported range");
    }

    if (config.toleranceCelsius < kMinToleranceCelsius) {
        throw std::invalid_argument("temperature tolerance is too small");
    }

    if (config.proportionalGain <= 0.0) {
        throw std::invalid_argument("temperature proportional gain must be positive");
    }

    if (config.minFanDuty < 0 || config.maxFanDuty > 255 ||
        config.minFanDuty > config.maxFanDuty) {
        throw std::invalid_argument("fan duty range is invalid");
    }

    if (config.manualFanDuty < config.minFanDuty || config.manualFanDuty > config.maxFanDuty) {
        throw std::invalid_argument("manual fan duty is out of configured range");
    }

    if (config.controlInterval.count() <= 0) {
        throw std::invalid_argument("temperature control interval must be positive");
    }
}

} // namespace

TempService::TempService(TemperatureReader temperatureReader,
                         FanDutyWriter fanDutyWriter,
                         StateReporter stateReporter)
    : temperatureReader_(std::move(temperatureReader)),
      fanDutyWriter_(std::move(fanDutyWriter)),
      stateReporter_(std::move(stateReporter))
{
    if (!temperatureReader_) {
        throw std::invalid_argument("temperature reader is required");
    }

    if (!fanDutyWriter_) {
        throw std::invalid_argument("fan duty writer is required");
    }
}

const char* TempService::name() const
{
    return "tempService";
}

void TempService::start()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = TempServiceState::running;
        lastError_.clear();
        nextControlAt_ = std::chrono::steady_clock::now();
    }

    reportState(TempServiceState::running);
    LOG_I("temp service started");
}

void TempService::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = TempServiceState::stopped;
        latestFanDuty_ = 0;
    }

    try {
        fanDutyWriter_(0);
    } catch (const std::exception& error) {
        LOG_W("temp service failed to stop fan: {}", error.what());
    }

    reportState(TempServiceState::stopped);
    LOG_I("temp service stopped");
}

void TempService::tick()
{
    TempServiceConfig config;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == TempServiceState::stopped) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (!isControlDueLocked(now)) {
            return;
        }

        nextControlAt_ = now + config_.controlInterval;
        config = config_;
    }

    try {
        const double temperatureCelsius = temperatureReader_();
        const int fanDuty = calculateFanDuty(temperatureCelsius);
        fanDutyWriter_(fanDuty);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            latestTemperatureCelsius_ = temperatureCelsius;
            latestFanDuty_ = fanDuty;
            ++controlCount_;
            lastError_.clear();
            state_ = TempServiceState::running;
        }

        reportState(TempServiceState::running);
        LOG_D("temp control tick: current={:.2f}C, target={:.2f}C, fanDuty={}",
              temperatureCelsius,
              config.targetCelsius,
              fanDuty);
    } catch (const std::exception& error) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = error.what();
            state_ = TempServiceState::degraded;
        }
        reportState(TempServiceState::degraded);
        LOG_W("temp control tick failed: {}", error.what());
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = "unknown exception";
            state_ = TempServiceState::degraded;
        }
        reportState(TempServiceState::degraded);
        LOG_W("temp control tick failed: unknown exception");
    }
}

std::vector<mlcp::hmi::system::RuntimePeriodicTask> TempService::runtimeTasks()
{
    return {
        mlcp::hmi::system::RuntimePeriodicTask {
            kRuntimeTickTaskName,
            mlcp::hmi::system::RuntimeThreadRole::service,
            kRuntimeTickInterval,
            [this]() {
                tick();
            },
        },
    };
}

void TempService::setTargetCelsius(double targetCelsius)
{
    std::lock_guard<std::mutex> lock(mutex_);
    TempServiceConfig nextConfig = config_;
    nextConfig.targetCelsius = targetCelsius;
    validateConfig(nextConfig);
    config_ = nextConfig;
}

void TempService::setConfig(const TempServiceConfig& config)
{
    validateConfig(config);
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

TempServiceConfig TempService::config() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

TempServiceSnapshot TempService::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    TempServiceSnapshot result;
    result.state = state_;
    result.temperatureAutoControl = config_.temperatureAutoControl;
    result.targetCelsius = config_.targetCelsius;
    result.toleranceCelsius = config_.toleranceCelsius;
    result.manualFanDuty = config_.manualFanDuty;
    result.minFanDuty = config_.minFanDuty;
    result.maxFanDuty = config_.maxFanDuty;
    result.latestTemperatureCelsius = latestTemperatureCelsius_;
    result.latestFanDuty = latestFanDuty_;
    result.controlCount = controlCount_;
    result.lastError = lastError_;
    return result;
}

int TempService::calculateFanDuty(double temperatureCelsius) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!config_.temperatureAutoControl) {
        return std::clamp(config_.manualFanDuty, config_.minFanDuty, config_.maxFanDuty);
    }

    const double errorCelsius = temperatureCelsius - config_.targetCelsius;
    if (errorCelsius <= config_.toleranceCelsius) {
        return 0;
    }

    const double requestedDuty =
        std::ceil((errorCelsius - config_.toleranceCelsius) * config_.proportionalGain);
    const int duty = static_cast<int>(requestedDuty);
    return std::clamp(duty, config_.minFanDuty, config_.maxFanDuty);
}

bool TempService::isControlDueLocked(std::chrono::steady_clock::time_point now) const
{
    return nextControlAt_ == std::chrono::steady_clock::time_point {} ||
           now >= nextControlAt_;
}

void TempService::reportState(TempServiceState state)
{
    if (stateReporter_) {
        stateReporter_(state);
    }
}

const char* tempServiceStateName(TempServiceState state)
{
    switch (state) {
    case TempServiceState::stopped:
        return "stopped";
    case TempServiceState::running:
        return "running";
    case TempServiceState::degraded:
        return "degraded";
    }

    return "unknown";
}

} // namespace mlcp::hmi::service::temp
