#pragma once

namespace mlcp::hmi::core::state_machine {

enum class SystemState {
    booting,
    selftest,
    standby,
    running,
    alarm,
    degraded,
    stopped,
};

enum class SystemStateEvent {
    powerOn,
    selftestStart,
    selftestPassed,
    startRequested,
    alarmRaised,
    degradeRequested,
    faultRecovered,
    stopRequested,
};

class SystemStateMachine {
public:
    SystemStateMachine();

    bool applyEvent(SystemStateEvent event);
    SystemState state() const;

private:
    bool canTransit(SystemState nextState) const;
    SystemState resolveNextState(SystemStateEvent event) const;

    SystemState state_;
};

const char* systemStateName(SystemState state);
const char* systemStateEventName(SystemStateEvent event);

} // namespace mlcp::hmi::core::state_machine
