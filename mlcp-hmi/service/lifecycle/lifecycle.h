#pragma once

#include <map>
#include <mutex>
#include <optional>

#include "core/state_machine/system_state/systemStateMachine.h"

namespace mlcp::hmi::service {

enum class LifecycleComponent {
    runtime,
    fan,
    temperatureSensor,
    tempService,
    systemInfoReader,
    selftest,
    operation,
    alarm,
};

enum class LifecycleComponentState {
    unknown,
    starting,
    ready,
    running,
    degraded,
    fault,
    stopped,
};

struct LifecycleSnapshot {
    mlcp::hmi::core::state_machine::SystemState systemState {
        mlcp::hmi::core::state_machine::SystemState::booting};
    std::map<LifecycleComponent, LifecycleComponentState> componentStates;
};

class Lifecycle {
public:
    using State = mlcp::hmi::core::state_machine::SystemState;

    Lifecycle();

    void enterStartup();
    void enterSelftest();
    void enterStandby();
    void enterRunning();
    void enterAlarm();
    void enterDegraded();
    void enterStopped();

    State state() const;
    State systemState() const;
    void reportComponentState(LifecycleComponent component, LifecycleComponentState componentState);
    std::optional<LifecycleComponentState> componentState(LifecycleComponent component) const;
    LifecycleSnapshot snapshot() const;

private:
    void applyEvent(mlcp::hmi::core::state_machine::SystemStateEvent event);
    bool applyEventLocked(mlcp::hmi::core::state_machine::SystemStateEvent event);
    void evaluateSystemStateLocked();
    std::optional<mlcp::hmi::core::state_machine::SystemStateEvent>
    resolveSystemEventLocked() const;
    std::optional<mlcp::hmi::core::state_machine::SystemStateEvent>
    resolveStopOrFaultEventLocked() const;
    bool isRuntimeStoppedLocked() const;
    bool hasComponentStateLocked(LifecycleComponentState componentState) const;

    mutable std::mutex mutex_;
    mlcp::hmi::core::state_machine::SystemStateMachine stateMachine_;
    std::map<LifecycleComponent, LifecycleComponentState> componentStates_;
};

const char* lifecycleComponentName(LifecycleComponent component);
const char* lifecycleComponentStateName(LifecycleComponentState componentState);

} // namespace mlcp::hmi::service
