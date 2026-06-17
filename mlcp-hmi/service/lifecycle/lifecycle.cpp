#include "service/lifecycle/lifecycle.h"

#include "service/log/logger.h"

namespace mlcp::hmi::service {
namespace {

using SystemState = mlcp::hmi::core::state_machine::SystemState;
using SystemStateEvent = mlcp::hmi::core::state_machine::SystemStateEvent;

bool isActiveOperation(LifecycleComponentState componentState)
{
    return componentState == LifecycleComponentState::running;
}

bool isSelftestActive(LifecycleComponentState componentState)
{
    return componentState == LifecycleComponentState::starting
        || componentState == LifecycleComponentState::running;
}

} // namespace

Lifecycle::Lifecycle()
    : stateMachine_()
{
}

void Lifecycle::enterStartup()
{
    applyEvent(SystemStateEvent::powerOn);
}

void Lifecycle::enterSelftest()
{
    reportComponentState(LifecycleComponent::selftest, LifecycleComponentState::running);
}

void Lifecycle::enterStandby()
{
    reportComponentState(LifecycleComponent::selftest, LifecycleComponentState::ready);
    reportComponentState(LifecycleComponent::operation, LifecycleComponentState::ready);
}

void Lifecycle::enterRunning()
{
    reportComponentState(LifecycleComponent::operation, LifecycleComponentState::running);
}

void Lifecycle::enterAlarm()
{
    reportComponentState(LifecycleComponent::alarm, LifecycleComponentState::fault);
}

void Lifecycle::enterDegraded()
{
    reportComponentState(LifecycleComponent::alarm, LifecycleComponentState::degraded);
}

void Lifecycle::enterStopped()
{
    reportComponentState(LifecycleComponent::runtime, LifecycleComponentState::stopped);
}

Lifecycle::State Lifecycle::state() const
{
    return systemState();
}

Lifecycle::State Lifecycle::systemState() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stateMachine_.state();
}

void Lifecycle::reportComponentState(
    LifecycleComponent component,
    LifecycleComponentState componentState)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto previous = componentStates_.find(component);
    if (previous != componentStates_.end() && previous->second == componentState) {
        return;
    }

    componentStates_[component] = componentState;
    LOG_I("lifecycle component state changed: component={}, state={}",
          lifecycleComponentName(component),
          lifecycleComponentStateName(componentState));
    evaluateSystemStateLocked();
}

std::optional<LifecycleComponentState> Lifecycle::componentState(
    LifecycleComponent component) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iter = componentStates_.find(component);
    if (iter == componentStates_.end()) {
        return std::nullopt;
    }

    return iter->second;
}

LifecycleSnapshot Lifecycle::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    LifecycleSnapshot currentSnapshot;
    currentSnapshot.systemState = stateMachine_.state();
    currentSnapshot.componentStates = componentStates_;
    return currentSnapshot;
}

void Lifecycle::applyEvent(SystemStateEvent event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    applyEventLocked(event);
}

bool Lifecycle::applyEventLocked(SystemStateEvent event)
{
    const auto previousState = stateMachine_.state();
    if (!stateMachine_.applyEvent(event)) {
        LOG_W("lifecycle transition rejected: state={}, event={}",
              mlcp::hmi::core::state_machine::systemStateName(previousState),
              mlcp::hmi::core::state_machine::systemStateEventName(event));
        return false;
    }

    if (previousState == stateMachine_.state()) {
        LOG_D("lifecycle state unchanged: state={}, event={}",
              mlcp::hmi::core::state_machine::systemStateName(previousState),
              mlcp::hmi::core::state_machine::systemStateEventName(event));
        return true;
    }

    LOG_I("lifecycle state changed: {} -> {}",
          mlcp::hmi::core::state_machine::systemStateName(previousState),
          mlcp::hmi::core::state_machine::systemStateName(stateMachine_.state()));
    return true;
}

void Lifecycle::evaluateSystemStateLocked()
{
    constexpr int kMaxEvaluateCount = 3;

    for (int index = 0; index < kMaxEvaluateCount; ++index) {
        const auto event = resolveSystemEventLocked();
        if (!event.has_value()) {
            return;
        }

        const SystemState previousState = stateMachine_.state();
        if (!applyEventLocked(*event)) {
            return;
        }

        if (previousState == stateMachine_.state()) {
            return;
        }
    }
}

std::optional<SystemStateEvent> Lifecycle::resolveSystemEventLocked() const
{
    const SystemState systemState = stateMachine_.state();
    const auto selftest = componentStates_.find(LifecycleComponent::selftest);
    const auto operation = componentStates_.find(LifecycleComponent::operation);
    const bool selftestReady = selftest != componentStates_.end()
        && selftest->second == LifecycleComponentState::ready;
    const bool selftestActive = selftest != componentStates_.end()
        && isSelftestActive(selftest->second);
    const bool operationActive = operation != componentStates_.end()
        && isActiveOperation(operation->second);

    switch (systemState) {
    case SystemState::booting:
        if (isRuntimeStoppedLocked()) {
            return SystemStateEvent::stopRequested;
        }
        if (resolveStopOrFaultEventLocked().has_value() || selftestActive || selftestReady
            || operationActive) {
            return SystemStateEvent::selftestStart;
        }
        break;
    case SystemState::selftest:
        if (const auto event = resolveStopOrFaultEventLocked(); event.has_value()) {
            return event;
        }
        if (selftestReady) {
            return SystemStateEvent::selftestPassed;
        }
        break;
    case SystemState::standby:
        if (const auto event = resolveStopOrFaultEventLocked(); event.has_value()) {
            return event;
        }
        if (selftestActive) {
            return SystemStateEvent::selftestStart;
        }
        if (operationActive) {
            return SystemStateEvent::startRequested;
        }
        break;
    case SystemState::running:
        if (const auto event = resolveStopOrFaultEventLocked(); event.has_value()) {
            return event;
        }
        if (!operationActive) {
            return SystemStateEvent::faultRecovered;
        }
        break;
    case SystemState::alarm:
    case SystemState::degraded:
        if (const auto event = resolveStopOrFaultEventLocked(); event.has_value()) {
            return event;
        }
        if (operationActive) {
            return SystemStateEvent::startRequested;
        }
        if (selftestReady) {
            return SystemStateEvent::faultRecovered;
        }
        break;
    case SystemState::stopped:
        if (!isRuntimeStoppedLocked()) {
            return SystemStateEvent::powerOn;
        }
        break;
    }

    return std::nullopt;
}

std::optional<SystemStateEvent> Lifecycle::resolveStopOrFaultEventLocked() const
{
    if (isRuntimeStoppedLocked()) {
        return SystemStateEvent::stopRequested;
    }

    if (hasComponentStateLocked(LifecycleComponentState::fault)) {
        return SystemStateEvent::alarmRaised;
    }

    if (hasComponentStateLocked(LifecycleComponentState::degraded)) {
        return SystemStateEvent::degradeRequested;
    }

    return std::nullopt;
}

bool Lifecycle::isRuntimeStoppedLocked() const
{
    const auto runtime = componentStates_.find(LifecycleComponent::runtime);
    return runtime != componentStates_.end()
        && runtime->second == LifecycleComponentState::stopped;
}

bool Lifecycle::hasComponentStateLocked(LifecycleComponentState componentState) const
{
    for (const auto& item : componentStates_) {
        if (item.second == componentState) {
            return true;
        }
    }

    return false;
}

const char* lifecycleComponentName(LifecycleComponent component)
{
    switch (component) {
    case LifecycleComponent::runtime:
        return "runtime";
    case LifecycleComponent::fan:
        return "fan";
    case LifecycleComponent::temperatureSensor:
        return "temperatureSensor";
    case LifecycleComponent::tempService:
        return "tempService";
    case LifecycleComponent::systemInfoReader:
        return "systemInfoReader";
    case LifecycleComponent::selftest:
        return "selftest";
    case LifecycleComponent::operation:
        return "operation";
    case LifecycleComponent::alarm:
        return "alarm";
    }

    return "unknown";
}

const char* lifecycleComponentStateName(LifecycleComponentState componentState)
{
    switch (componentState) {
    case LifecycleComponentState::unknown:
        return "unknown";
    case LifecycleComponentState::starting:
        return "starting";
    case LifecycleComponentState::ready:
        return "ready";
    case LifecycleComponentState::running:
        return "running";
    case LifecycleComponentState::degraded:
        return "degraded";
    case LifecycleComponentState::fault:
        return "fault";
    case LifecycleComponentState::stopped:
        return "stopped";
    }

    return "unknown";
}

} // namespace mlcp::hmi::service
