#include "core/state_machine/system_state/systemStateMachine.h"

#include <cassert>

int main()
{
    mlcp::hmi::core::state_machine::SystemStateMachine stateMachine;
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::booting);

    assert(stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::selftestStart));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::selftest);

    assert(stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::selftestPassed));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::standby);

    assert(stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::startRequested));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::running);

    assert(stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::alarmRaised));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::alarm);

    assert(stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::faultRecovered));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::standby);

    assert(!stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::powerOn));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::standby);

    assert(stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::stopRequested));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::stopped);

    assert(stateMachine.applyEvent(
        mlcp::hmi::core::state_machine::SystemStateEvent::powerOn));
    assert(stateMachine.state() == mlcp::hmi::core::state_machine::SystemState::booting);

    return 0;
}
