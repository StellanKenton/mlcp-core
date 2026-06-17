#include "bridge/fanSettingsViewModel.h"

#include <exception>

#include "bridge/fanSettingsMapper.h"

namespace mlcp::hmi::bridge {
namespace {

QString formatTemperatureText(const std::optional<double>& value)
{
    if (!value.has_value()) {
        return "--";
    }

    return QString("%1 C").arg(*value, 0, 'f', 1);
}

bool sameConfig(const FanSettingsUiState& left, const FanSettingsUiState& right)
{
    return left.temperatureAutoControl == right.temperatureAutoControl &&
        left.targetCelsius == right.targetCelsius &&
        left.toleranceCelsius == right.toleranceCelsius &&
        left.manualFanDuty == right.manualFanDuty &&
        left.minFanDuty == right.minFanDuty &&
        left.maxFanDuty == right.maxFanDuty;
}

bool sameRuntimeSnapshot(const FanSettingsUiState& left, const FanSettingsUiState& right)
{
    return left.latestFanDuty == right.latestFanDuty &&
        left.latestTemperatureCelsius == right.latestTemperatureCelsius;
}

bool sameOperationStatus(const FanSettingsUiState& left, const FanSettingsUiState& right)
{
    return left.operationStatusText == right.operationStatusText &&
        left.error.code == right.error.code &&
        left.error.userMessage == right.error.userMessage;
}

} // namespace

FanSettingsViewModel::FanSettingsViewModel(
    IHmiBackend& backend,
    QObject* parent)
    : QObject(parent),
      backend_(backend)
{
    refresh();
}

bool FanSettingsViewModel::available() const
{
    return state_.available;
}

bool FanSettingsViewModel::temperatureAutoControl() const
{
    return state_.temperatureAutoControl;
}

double FanSettingsViewModel::targetCelsius() const
{
    return state_.targetCelsius;
}

double FanSettingsViewModel::toleranceCelsius() const
{
    return state_.toleranceCelsius;
}

int FanSettingsViewModel::manualFanDuty() const
{
    return state_.manualFanDuty;
}

int FanSettingsViewModel::maxFanDuty() const
{
    return state_.maxFanDuty;
}

int FanSettingsViewModel::latestFanDuty() const
{
    return state_.latestFanDuty;
}

bool FanSettingsViewModel::hasLatestTemperature() const
{
    return state_.latestTemperatureCelsius.has_value();
}

double FanSettingsViewModel::latestTemperatureCelsius() const
{
    return state_.latestTemperatureCelsius.value_or(0.0);
}

QString FanSettingsViewModel::latestTemperatureText() const
{
    return formatTemperatureText(state_.latestTemperatureCelsius);
}

QString FanSettingsViewModel::dataStatusText() const
{
    return QString::fromStdString(state_.dataStatusText);
}

QString FanSettingsViewModel::operationStatusText() const
{
    return QString::fromStdString(state_.operationStatusText);
}

QString FanSettingsViewModel::statusText() const
{
    if (!state_.operationStatusText.empty()) {
        return operationStatusText();
    }

    return dataStatusText();
}

int FanSettingsViewModel::errorCode() const
{
    return static_cast<int>(state_.error.code);
}

QString FanSettingsViewModel::errorText() const
{
    return QString::fromStdString(state_.error.userMessage);
}

bool FanSettingsViewModel::hasError() const
{
    return hasUiError(state_.error);
}

void FanSettingsViewModel::refresh()
{
    try {
        FanSettingsUiState nextState = toFanSettingsUiState(backend_.tempServiceSnapshot());
        nextState.operationStatusText = state_.operationStatusText;
        nextState.error = state_.error;
        applyState(nextState);
    } catch (const std::exception& error) {
        applyRefreshFailure(toFanSettingsUiError(error));
    } catch (...) {
        applyRefreshFailure(toUnknownUiError());
    }
}

bool FanSettingsViewModel::applySettings(bool temperatureAutoControl,
                                         double targetCelsius,
                                         int manualFanDuty,
                                         int maxFanDuty)
{
    FanSettingsCommand command;
    command.temperatureAutoControl = temperatureAutoControl;
    command.targetCelsius = targetCelsius;
    command.manualFanDuty = manualFanDuty;
    command.maxFanDuty = maxFanDuty;
    return applyFanSettingsCommand(command);
}

void FanSettingsViewModel::applyState(const FanSettingsUiState& state)
{
    const bool availabilityChangedValue = state.available != state_.available;
    const bool configChangedValue = !sameConfig(state, state_);
    const bool runtimeChangedValue = !sameRuntimeSnapshot(state, state_);
    const bool dataStatusChangedValue = state.dataStatusText != state_.dataStatusText;
    const bool operationChangedValue = !sameOperationStatus(state, state_);

    if (!availabilityChangedValue &&
        !configChangedValue &&
        !runtimeChangedValue &&
        !dataStatusChangedValue &&
        !operationChangedValue) {
        return;
    }

    state_ = state;

    if (availabilityChangedValue) {
        emit availabilityChanged();
    }
    if (configChangedValue) {
        emit configChanged();
    }
    if (runtimeChangedValue) {
        emit runtimeSnapshotChanged();
    }
    if (dataStatusChangedValue) {
        emit dataStatusChanged();
    }
    if (operationChangedValue) {
        emit operationStatusChanged();
    }

    emit settingsChanged();
}

void FanSettingsViewModel::applyRefreshFailure(const UiError& error)
{
    FanSettingsUiState nextState = state_;
    nextState.available = false;
    nextState.latestTemperatureCelsius.reset();
    nextState.latestFanDuty = 0;
    nextState.dataStatusText = "Load failed";
    nextState.operationStatusText = error.userMessage;
    nextState.error = error;
    applyState(nextState);
}

void FanSettingsViewModel::setOperationResult(const QString& message, const UiError& error)
{
    FanSettingsUiState nextState = state_;
    nextState.operationStatusText = message.toStdString();
    nextState.error = error;
    applyState(nextState);
}

bool FanSettingsViewModel::applyFanSettingsCommand(const FanSettingsCommand& command)
{
    try {
        backend_.applyTempServiceConfig(configForCommand(command));
        setOperationResult("Saved", noUiError());
        refresh();
        return true;
    } catch (const std::exception& error) {
        const UiError uiError = toFanSettingsUiError(error);
        setOperationResult(QString::fromStdString(uiError.userMessage), uiError);
    } catch (...) {
        const UiError uiError = toUnknownUiError();
        setOperationResult(QString::fromStdString(uiError.userMessage), uiError);
    }

    return false;
}

mlcp::hmi::service::temp::TempServiceConfig FanSettingsViewModel::configForCommand(
    const FanSettingsCommand& command)
{
    auto config = backend_.tempServiceConfig();
    config.temperatureAutoControl = command.temperatureAutoControl;
    config.targetCelsius = command.targetCelsius;
    config.maxFanDuty = command.maxFanDuty;
    config.manualFanDuty = command.manualFanDuty;
    return config;
}

} // namespace mlcp::hmi::bridge
