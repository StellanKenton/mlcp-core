#include "service/temp/temperatureSampler.h"

#include <cassert>
#include <stdexcept>
#include <string>

namespace {

using mlcp::hmi::service::temp::TemperatureSampler;

void testStoresLatestTemperature()
{
    double currentTemperature = 36.8;
    TemperatureSampler sampler([&currentTemperature]() {
        return currentTemperature;
    });

    sampler.sample();
    currentTemperature = 37.1;
    sampler.sample();

    const auto snapshot = sampler.snapshot();
    assert(snapshot.latestTemperatureCelsius.has_value());
    assert(*snapshot.latestTemperatureCelsius == 37.1);
    assert(snapshot.sampleCount == 2U);
    assert(snapshot.lastError.empty());
    assert(sampler.latestTemperatureCelsiusOrThrow() == 37.1);
}

void testRecordsReadFailure()
{
    TemperatureSampler sampler([]() -> double {
        throw std::runtime_error("sensor timeout");
    });

    bool sampleFailed = false;
    try {
        sampler.sample();
    } catch (const std::runtime_error&) {
        sampleFailed = true;
    }

    assert(sampleFailed);
    const auto snapshot = sampler.snapshot();
    assert(!snapshot.latestTemperatureCelsius.has_value());
    assert(snapshot.sampleCount == 0U);
    assert(snapshot.lastError == "sensor timeout");

    bool latestFailed = false;
    try {
        static_cast<void>(sampler.latestTemperatureCelsiusOrThrow());
    } catch (const std::runtime_error& error) {
        latestFailed = std::string(error.what()) == "sensor timeout";
    }
    assert(latestFailed);
}

} // namespace

int main()
{
    testStoresLatestTemperature();
    testRecordsReadFailure();

    return 0;
}
