#include "core/state_machine/system_state/systemStateMachine.h"

namespace mlcp::hmi::core::state_machine {

namespace {

bool isFaultState(SystemState state)
{
    return state == SystemState::alarm || state == SystemState::degraded;
}

} // namespace

SystemStateMachine::SystemStateMachine()
    : state_(SystemState::booting)
{
}

bool SystemStateMachine::applyEvent(SystemStateEvent event)
{
    const SystemState nextState = resolveNextState(event);
    if (nextState == state_) {
        return true;
    }

    if (!canTransit(nextState)) {
        return false;
    }

    state_ = nextState;
    return true;
}

SystemState SystemStateMachine::state() const
{
    return state_;
}

bool SystemStateMachine::canTransit(SystemState nextState) const
{
    if (nextState == SystemState::stopped) {
        return true;
    }

    switch (state_) {
    case SystemState::booting:
        return nextState == SystemState::selftest;
    case SystemState::selftest:
        return nextState == SystemState::standby || isFaultState(nextState);
    case SystemState::standby:
        return nextState == SystemState::running || nextState == SystemState::selftest
            || isFaultState(nextState);
    case SystemState::running:
        return nextState == SystemState::standby || isFaultState(nextState);
    case SystemState::alarm:
    case SystemState::degraded:
        return nextState == SystemState::standby || nextState == SystemState::running
            || isFaultState(nextState);
    case SystemState::stopped:
        return nextState == SystemState::booting;
    }

    return false;
}

SystemState SystemStateMachine::resolveNextState(SystemStateEvent event) const
{
    switch (event) {
    case SystemStateEvent::powerOn:
        return SystemState::booting;
    case SystemStateEvent::selftestStart:
        return SystemState::selftest;
    case SystemStateEvent::selftestPassed:
    case SystemStateEvent::faultRecovered:
        return SystemState::standby;
    case SystemStateEvent::startRequested:
        return SystemState::running;
    case SystemStateEvent::alarmRaised:
        return SystemState::alarm;
    case SystemStateEvent::degradeRequested:
        return SystemState::degraded;
    case SystemStateEvent::stopRequested:
        return SystemState::stopped;
    }

    return state_;
}

const char* systemStateName(SystemState state)
{
    switch (state) {
    case SystemState::booting:
        return "booting";
    case SystemState::selftest:
        return "selftest";
    case SystemState::standby:
        return "standby";
    case SystemState::running:
        return "running";
    case SystemState::alarm:
        return "alarm";
    case SystemState::degraded:
        return "degraded";
    case SystemState::stopped:
        return "stopped";
    }

    return "unknown";
}

const char* systemStateEventName(SystemStateEvent event)
{
    switch (event) {
    case SystemStateEvent::powerOn:
        return "powerOn";
    case SystemStateEvent::selftestStart:
        return "selftestStart";
    case SystemStateEvent::selftestPassed:
        return "selftestPassed";
    case SystemStateEvent::startRequested:
        return "startRequested";
    case SystemStateEvent::alarmRaised:
        return "alarmRaised";
    case SystemStateEvent::degradeRequested:
        return "degradeRequested";
    case SystemStateEvent::faultRecovered:
        return "faultRecovered";
    case SystemStateEvent::stopRequested:
        return "stopRequested";
    }

    return "unknown";
}

} // namespace mlcp::hmi::core::state_machine
