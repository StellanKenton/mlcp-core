#include "bridge/lifecycleMapper.h"

#include <string>

#include "core/state_machine/system_state/systemStateMachine.h"

namespace mlcp::hmi::bridge {
namespace {

std::string toUpperAscii(std::string text)
{
    for (char& character : text) {
        if (character >= 'a' && character <= 'z') {
            character = static_cast<char>(character - 'a' + 'A');
        }
    }

    return text;
}

} // namespace

LifecycleUiState toLifecycleUiState(
    const mlcp::hmi::service::LifecycleSnapshot& snapshot)
{
    using mlcp::hmi::core::state_machine::SystemState;
    using mlcp::hmi::core::state_machine::systemStateName;

    LifecycleUiState state;
    state.stateCode = static_cast<int>(snapshot.systemState);
    state.stateText = toUpperAscii(systemStateName(snapshot.systemState));
    state.alarmActive = snapshot.systemState == SystemState::alarm;
    state.degraded = snapshot.systemState == SystemState::degraded;

    if (state.alarmActive) {
        state.severityText = "alarm";
    } else if (state.degraded) {
        state.severityText = "degraded";
    } else {
        state.severityText = "normal";
    }

    return state;
}

} // namespace mlcp::hmi::bridge
