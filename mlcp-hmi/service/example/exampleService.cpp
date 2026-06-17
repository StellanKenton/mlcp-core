#include "service/example/exampleService.h"

#include <exception>
#include <stdexcept>
#include <utility>

#include "service/log/logger.h"

namespace mlcp::hmi::service::example {
namespace {

constexpr std::chrono::milliseconds kRuntimeTickInterval {25};
constexpr const char* kRuntimeTickTaskName = "service.example.tick";

void validateConfig(const ExampleServiceConfig& config)
{
    if (config.lowerLimit > config.upperLimit) {
        throw std::invalid_argument("example lower limit is greater than upper limit");
    }
    if (config.controlInterval.count() <= 0) {
        throw std::invalid_argument("example control interval must be positive");
    }
}

} // namespace

ExampleService::ExampleService(ValueReader valueReader, StateReporter stateReporter)
    : valueReader_(std::move(valueReader)),
      stateReporter_(std::move(stateReporter))
{
    if (!valueReader_) {
        throw std::invalid_argument("example value reader is required");
    }
}

const char* ExampleService::name() const
{
    return "exampleService";
}

void ExampleService::start()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = ExampleServiceState::running;
        lastError_.clear();
        nextTickAt_ = std::chrono::steady_clock::now();
    }

    reportState(ExampleServiceState::running);
    LOG_I("example service started");
}

void ExampleService::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = ExampleServiceState::stopped;
    }

    reportState(ExampleServiceState::stopped);
    LOG_I("example service stopped");
}

void ExampleService::tick()
{
    ExampleServiceConfig config;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == ExampleServiceState::stopped) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (!isTickDueLocked(now)) {
            return;
        }

        nextTickAt_ = now + config_.controlInterval;
        config = config_;
    }

    if (!config.enabled) {
        return;
    }

    try {
        const double value = valueReader_();
        if (value < config.lowerLimit || value > config.upperLimit) {
            throw std::runtime_error("example value is out of configured range");
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            latestValue_ = value;
            ++tickCount_;
            lastError_.clear();
            state_ = ExampleServiceState::running;
        }
        reportState(ExampleServiceState::running);
    } catch (const std::exception& error) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = error.what();
            state_ = ExampleServiceState::degraded;
        }
        reportState(ExampleServiceState::degraded);
        LOG_W("example service tick failed: {}", error.what());
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = "unknown exception";
            state_ = ExampleServiceState::degraded;
        }
        reportState(ExampleServiceState::degraded);
        LOG_W("example service tick failed: unknown exception");
    }
}

std::vector<mlcp::hmi::system::RuntimePeriodicTask> ExampleService::runtimeTasks()
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

void ExampleService::setConfig(const ExampleServiceConfig& config)
{
    validateConfig(config);
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

ExampleServiceConfig ExampleService::config() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

ExampleServiceSnapshot ExampleService::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExampleServiceSnapshot result;
    result.state = state_;
    result.enabled = config_.enabled;
    result.lowerLimit = config_.lowerLimit;
    result.upperLimit = config_.upperLimit;
    result.latestValue = latestValue_;
    result.tickCount = tickCount_;
    result.lastError = lastError_;
    return result;
}

bool ExampleService::isTickDueLocked(std::chrono::steady_clock::time_point now) const
{
    return nextTickAt_ == std::chrono::steady_clock::time_point {} || now >= nextTickAt_;
}

void ExampleService::reportState(ExampleServiceState state)
{
    if (stateReporter_) {
        stateReporter_(state);
    }
}

const char* exampleServiceStateName(ExampleServiceState state)
{
    switch (state) {
    case ExampleServiceState::stopped:
        return "stopped";
    case ExampleServiceState::running:
        return "running";
    case ExampleServiceState::degraded:
        return "degraded";
    }

    return "unknown";
}

} // namespace mlcp::hmi::service::example
