#include "bridge/fanSettingsMapper.h"

#include <stdexcept>
#include <string>

namespace mlcp::hmi::bridge {
namespace {

UiError makeError(UiErrorCode code,
                  const std::string& technicalMessage,
                  const std::string& userMessage)
{
    UiError error;
    error.code = code;
    error.technicalMessage = technicalMessage;
    error.userMessage = userMessage;
    return error;
}

} // namespace

FanSettingsUiState toFanSettingsUiState(
    const mlcp::hmi::service::temp::TempServiceSnapshot& snapshot)
{
    FanSettingsUiState state;
    state.available = snapshot.state != mlcp::hmi::service::temp::TempServiceState::stopped;
    state.temperatureAutoControl = snapshot.temperatureAutoControl;
    state.targetCelsius = snapshot.targetCelsius;
    state.toleranceCelsius = snapshot.toleranceCelsius;
    state.manualFanDuty = snapshot.manualFanDuty;
    state.minFanDuty = snapshot.minFanDuty;
    state.maxFanDuty = snapshot.maxFanDuty;
    state.latestTemperatureCelsius = snapshot.latestTemperatureCelsius;
    state.latestFanDuty = snapshot.latestFanDuty;
    state.dataStatusText = "Loaded";
    state.operationStatusText.clear();

    if (!snapshot.lastError.empty()) {
        state.error = makeError(
            UiErrorCode::tempServiceUnavailable,
            snapshot.lastError,
            "Temperature service reports a runtime fault");
        state.operationStatusText = state.error.userMessage;
    }

    return state;
}

UiError toFanSettingsUiError(const std::exception& error)
{
    const std::string message = error.what();
    if (dynamic_cast<const std::invalid_argument*>(&error) != nullptr) {
        if (message.find("temperature target") != std::string::npos) {
            return makeError(UiErrorCode::invalidTempTarget, message, message);
        }

        if (message.find("fan duty") != std::string::npos) {
            return makeError(UiErrorCode::invalidFanDuty, message, message);
        }

        return makeError(UiErrorCode::invalidConfig, message, message);
    }

    return makeError(UiErrorCode::backendFailure, message, message);
}

UiError toUnknownUiError()
{
    return makeError(UiErrorCode::backendFailure, "unknown error", "unknown error");
}

} // namespace mlcp::hmi::bridge
