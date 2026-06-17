#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace mlcp::hmi::service::temp {

struct TemperatureSamplerSnapshot {
    std::optional<double> latestTemperatureCelsius;
    std::uint64_t sampleCount {0U};
    std::string lastError;
};

class TemperatureSampler {
public:
    using TemperatureReader = std::function<double()>;

    explicit TemperatureSampler(TemperatureReader temperatureReader);

    void sample();
    double latestTemperatureCelsiusOrThrow() const;
    TemperatureSamplerSnapshot snapshot() const;

private:
    TemperatureReader temperatureReader_;
    mutable std::mutex mutex_;
    std::optional<double> latestTemperatureCelsius_;
    std::uint64_t sampleCount_ {0U};
    std::string lastError_;
};

} // namespace mlcp::hmi::service::temp
