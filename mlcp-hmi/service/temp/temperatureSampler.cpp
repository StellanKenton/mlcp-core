#include "service/temp/temperatureSampler.h"

#include <exception>
#include <stdexcept>
#include <utility>

namespace mlcp::hmi::service::temp {

TemperatureSampler::TemperatureSampler(TemperatureReader temperatureReader)
    : temperatureReader_(std::move(temperatureReader))
{
    if (!temperatureReader_) {
        throw std::invalid_argument("temperature reader is required");
    }
}

void TemperatureSampler::sample()
{
    try {
        const double temperatureCelsius = temperatureReader_();
        std::lock_guard<std::mutex> lock(mutex_);
        latestTemperatureCelsius_ = temperatureCelsius;
        ++sampleCount_;
        lastError_.clear();
    } catch (const std::exception& error) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = error.what();
        }
        throw;
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = "unknown exception";
        }
        throw;
    }
}

double TemperatureSampler::latestTemperatureCelsiusOrThrow() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!lastError_.empty()) {
        throw std::runtime_error(lastError_);
    }
    if (latestTemperatureCelsius_.has_value()) {
        return *latestTemperatureCelsius_;
    }

    throw std::runtime_error("temperature sample is not ready");
}

TemperatureSamplerSnapshot TemperatureSampler::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    TemperatureSamplerSnapshot result;
    result.latestTemperatureCelsius = latestTemperatureCelsius_;
    result.sampleCount = sampleCount_;
    result.lastError = lastError_;
    return result;
}

} // namespace mlcp::hmi::service::temp
