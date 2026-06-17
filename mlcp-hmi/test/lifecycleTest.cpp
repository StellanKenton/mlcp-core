#include "service/lifecycle/lifecycle.h"

#include <cassert>

namespace {

using mlcp::hmi::core::state_machine::SystemState;
using mlcp::hmi::service::Lifecycle;
using mlcp::hmi::service::LifecycleComponent;
using mlcp::hmi::service::LifecycleComponentState;

void reportStartupReady(Lifecycle& lifecycle)
{
    lifecycle.reportComponentState(
        LifecycleComponent::runtime,
        LifecycleComponentState::running);
    lifecycle.reportComponentState(
        LifecycleComponent::selftest,
        LifecycleComponentState::ready);
}

} // namespace

int main()
{
    Lifecycle lifecycle;
    assert(lifecycle.systemState() == SystemState::booting);

    lifecycle.enterSelftest();
    assert(lifecycle.systemState() == SystemState::selftest);

    lifecycle.reportComponentState(
        LifecycleComponent::fan,
        LifecycleComponentState::fault);
    assert(lifecycle.systemState() == SystemState::alarm);

    lifecycle.reportComponentState(
        LifecycleComponent::fan,
        LifecycleComponentState::ready);
    lifecycle.enterStandby();
    assert(lifecycle.systemState() == SystemState::standby);

    lifecycle.enterRunning();
    assert(lifecycle.systemState() == SystemState::running);

    lifecycle.reportComponentState(
        LifecycleComponent::temperatureSensor,
        LifecycleComponentState::degraded);
    assert(lifecycle.systemState() == SystemState::degraded);

    lifecycle.reportComponentState(
        LifecycleComponent::temperatureSensor,
        LifecycleComponentState::ready);
    assert(lifecycle.systemState() == SystemState::running);

    lifecycle.enterStopped();
    assert(lifecycle.systemState() == SystemState::stopped);

    Lifecycle bootLifecycle;
    reportStartupReady(bootLifecycle);
    assert(bootLifecycle.systemState() == SystemState::standby);

    const auto snapshot = bootLifecycle.snapshot();
    assert(snapshot.systemState == SystemState::standby);
    assert(snapshot.componentStates.at(LifecycleComponent::runtime)
        == LifecycleComponentState::running);

    return 0;
}
