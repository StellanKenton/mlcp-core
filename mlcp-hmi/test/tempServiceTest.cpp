#include "service/temp/tempService.h"

#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

using mlcp::hmi::service::temp::TempService;
using mlcp::hmi::service::temp::TempServiceConfig;
using mlcp::hmi::service::temp::TempServiceState;

void testControlsFanFromTemperature()
{
    double currentTemperature = 40.0;
    std::vector<int> fanDuties;
    TempServiceState reportedState = TempServiceState::stopped;

    TempService service(
        [&currentTemperature]() {
            return currentTemperature;
        },
        [&fanDuties](int duty) {
            fanDuties.push_back(duty);
        },
        [&reportedState](TempServiceState state) {
            reportedState = state;
        });

    TempServiceConfig config;
    config.targetCelsius = 37.0;
    config.toleranceCelsius = 0.5;
    config.proportionalGain = 32.0;
    config.controlInterval = std::chrono::milliseconds(1);
    service.setConfig(config);

    service.start();
    service.tick();

    assert(!fanDuties.empty());
    assert(fanDuties.back() == 80);
    assert(reportedState == TempServiceState::running);

    currentTemperature = 37.2;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    service.tick();
    assert(fanDuties.back() == 0);

    const auto snapshot = service.snapshot();
    assert(snapshot.state == TempServiceState::running);
    assert(snapshot.latestTemperatureCelsius.has_value());
    assert(*snapshot.latestTemperatureCelsius == 37.2);
    assert(snapshot.latestFanDuty == 0);
    assert(snapshot.controlCount == 2U);
}

void testReportsDegradedWhenReadFails()
{
    TempServiceState reportedState = TempServiceState::stopped;
    TempService service(
        []() -> double {
            throw std::runtime_error("sensor timeout");
        },
        [](int) {},
        [&reportedState](TempServiceState state) {
            reportedState = state;
        });

    service.start();
    service.tick();

    const auto snapshot = service.snapshot();
    assert(snapshot.state == TempServiceState::degraded);
    assert(snapshot.lastError == "sensor timeout");
    assert(reportedState == TempServiceState::degraded);
}

void testManualFanDutyOverridesTemperatureControl()
{
    std::vector<int> fanDuties;
    TempService service(
        []() {
            return 25.0;
        },
        [&fanDuties](int duty) {
            fanDuties.push_back(duty);
        });

    TempServiceConfig config;
    config.temperatureAutoControl = false;
    config.manualFanDuty = 96;
    config.maxFanDuty = 128;
    config.controlInterval = std::chrono::milliseconds(1);
    service.setConfig(config);

    service.start();
    service.tick();

    assert(!fanDuties.empty());
    assert(fanDuties.back() == 96);

    const auto snapshot = service.snapshot();
    assert(!snapshot.temperatureAutoControl);
    assert(snapshot.manualFanDuty == 96);
    assert(snapshot.maxFanDuty == 128);
}

void testStoresConfiguredMaxFanDuty()
{
    TempService service(
        []() {
            return 45.0;
        },
        [](int) {});

    TempServiceConfig config = service.config();
    config.maxFanDuty = 210;
    config.manualFanDuty = 0;
    service.setConfig(config);

    assert(service.config().maxFanDuty == 210);
    assert(service.snapshot().maxFanDuty == 210);
}

void testStopTurnsFanOff()
{
    std::vector<int> fanDuties;
    TempService service(
        []() {
            return 45.0;
        },
        [&fanDuties](int duty) {
            fanDuties.push_back(duty);
        });

    service.start();
    service.stop();

    assert(!fanDuties.empty());
    assert(fanDuties.back() == 0);
    assert(service.snapshot().state == TempServiceState::stopped);
}

} // namespace

int main()
{
    testControlsFanFromTemperature();
    testReportsDegradedWhenReadFails();
    testManualFanDutyOverridesTemperatureControl();
    testStoresConfiguredMaxFanDuty();
    testStopTurnsFanOff();

    return 0;
}
