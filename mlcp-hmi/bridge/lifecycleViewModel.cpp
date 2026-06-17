#include "bridge/lifecycleViewModel.h"

#include "bridge/lifecycleMapper.h"

namespace mlcp::hmi::bridge {

LifecycleViewModel::LifecycleViewModel(
    const IHmiBackend& backend,
    QObject* parent)
    : QObject(parent),
      backend_(backend)
{
    refreshLifecycle();

    refreshTimer_.setInterval(200);
    connect(&refreshTimer_, &QTimer::timeout, this, &LifecycleViewModel::refreshLifecycle);
    refreshTimer_.start();
}

int LifecycleViewModel::stateCode() const
{
    return state_.stateCode;
}

QString LifecycleViewModel::stateText() const
{
    return QString::fromStdString(state_.stateText);
}

QString LifecycleViewModel::severityText() const
{
    return QString::fromStdString(state_.severityText);
}

bool LifecycleViewModel::alarmActive() const
{
    return state_.alarmActive;
}

bool LifecycleViewModel::degraded() const
{
    return state_.degraded;
}

void LifecycleViewModel::refreshLifecycle()
{
    applyState(toLifecycleUiState(backend_.lifecycleSnapshot()));
}

void LifecycleViewModel::applyState(const LifecycleUiState& state)
{
    if (state.stateCode == state_.stateCode &&
        state.stateText == state_.stateText &&
        state.severityText == state_.severityText &&
        state.alarmActive == state_.alarmActive &&
        state.degraded == state_.degraded) {
        return;
    }

    const bool stateTextChangedValue = state.stateText != state_.stateText;
    state_ = state;
    emit lifecycleChanged();
    if (!stateTextChangedValue) {
        return;
    }

    emit stateTextChanged();
}

} // namespace mlcp::hmi::bridge
