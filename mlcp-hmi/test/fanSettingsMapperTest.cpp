#include <cassert>
#include <stdexcept>

#include "bridge/fanSettingsMapper.h"

using mlcp::hmi::bridge::UiErrorCode;
using mlcp::hmi::bridge::toFanSettingsUiError;
using mlcp::hmi::bridge::toFanSettingsUiState;
using mlcp::hmi::service::temp::TempServiceSnapshot;
using mlcp::hmi::service::temp::TempServiceState;

void mapsRunningSnapshotToUiState()
{
    TempServiceSnapshot snapshot;
    snapshot.state = TempServiceState::running;
    snapshot.temperatureAutoControl = false;
    snapshot.targetCelsius = 38.5;
    snapshot.toleranceCelsius = 0.4;
    snapshot.manualFanDuty = 96;
    snapshot.minFanDuty = 12;
    snapshot.maxFanDuty = 180;
    snapshot.latestTemperatureCelsius = 37.8;
    snapshot.latestFanDuty = 104;

    const auto state = toFanSettingsUiState(snapshot);

    assert(state.available);
    assert(!state.temperatureAutoControl);
    assert(state.targetCelsius == 38.5);
    assert(state.toleranceCelsius == 0.4);
    assert(state.manualFanDuty == 96);
    assert(state.minFanDuty == 12);
    assert(state.maxFanDuty == 180);
    assert(state.latestTemperatureCelsius.has_value());
    assert(*state.latestTemperatureCelsius == 37.8);
    assert(state.latestFanDuty == 104);
    assert(state.dataStatusText == "Loaded");
    assert(state.error.code == UiErrorCode::none);
}

void mapsMissingTemperatureWithoutFakeValue()
{
    TempServiceSnapshot snapshot;
    snapshot.state = TempServiceState::running;

    const auto state = toFanSettingsUiState(snapshot);

    assert(state.available);
    assert(!state.latestTemperatureCelsius.has_value());
}

void mapsSnapshotErrorToStructuredError()
{
    TempServiceSnapshot snapshot;
    snapshot.state = TempServiceState::degraded;
    snapshot.lastError = "temperature sensor timeout";

    const auto state = toFanSettingsUiState(snapshot);

    assert(state.available);
    assert(state.error.code == UiErrorCode::tempServiceUnavailable);
    assert(state.error.technicalMessage == "temperature sensor timeout");
    assert(!state.operationStatusText.empty());
}

void mapsInvalidArgumentToStableCode()
{
    const auto error = toFanSettingsUiError(
        std::invalid_argument("manual fan duty is out of configured range"));

    assert(error.code == UiErrorCode::invalidFanDuty);
    assert(error.technicalMessage == "manual fan duty is out of configured range");
}

int main()
{
    mapsRunningSnapshotToUiState();
    mapsMissingTemperatureWithoutFakeValue();
    mapsSnapshotErrorToStructuredError();
    mapsInvalidArgumentToStableCode();
    return 0;
}
