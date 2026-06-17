#include "system/appcontrol/appcontrol.h"

#include <optional>
#include <stdexcept>
#include <string>

#include "service/log/logger.h"

namespace mlcp::hmi::system {

AppControl::AppControl()
    : lifecycle_(),
      runtime_(),
      runtimeTasks_(runtime_),
      devices_(),
      wiring_(),
      selfTestFlow_([this](mlcp::hmi::service::LifecycleComponent component,
                           mlcp::hmi::service::LifecycleComponentState componentState) {
          reportLifecycleComponentStateDirect(component, componentState);
      })
{
    mlcp::common::log::initConsoleLogger();
    requestStop_.store(false);
    LOG_I("appcontrol logger initialized");
}

void AppControl::startup()
{
    LOG_I("appcontrol startup start");
    runtime_.start();
    runtime_.runServiceTaskAndWait([this]() {
        lifecycle_.enterStartup();
        lifecycle_.reportComponentState(
            mlcp::hmi::service::LifecycleComponent::runtime,
            mlcp::hmi::service::LifecycleComponentState::running);
    });
    devices_.initializeFan(
        runtime_,
        [this](mlcp::hmi::service::LifecycleComponent component,
               mlcp::hmi::service::LifecycleComponentState componentState) {
            reportLifecycleComponentState(component, componentState);
        });
    devices_.initializeTemperatureSensor(
        runtime_,
        runtimeTasks_,
        [this](mlcp::hmi::service::LifecycleComponent component,
               mlcp::hmi::service::LifecycleComponentState componentState) {
            reportLifecycleComponentState(component, componentState);
        });
    wiring_.initializeServices(
        devices_,
        runtime_,
        runtimeTasks_,
        [this](mlcp::hmi::service::LifecycleComponent component,
               mlcp::hmi::service::LifecycleComponentState componentState) {
            reportLifecycleComponentState(component, componentState);
        },
        [this](mlcp::hmi::service::LifecycleComponent component,
               mlcp::hmi::service::LifecycleComponentState componentState) {
            reportLifecycleComponentStateDirect(component, componentState);
        });
    devices_.initializeSystemInfoReader(
        runtime_,
        runtimeTasks_,
        [this](mlcp::hmi::service::LifecycleComponent component,
               mlcp::hmi::service::LifecycleComponentState componentState) {
            reportLifecycleComponentState(component, componentState);
        });
    wiring_.configureSelfTestChecks(selfTestFlow_, devices_, runtime_);
    executeSelfTest(mlcp::hmi::service::selftest::SelfTestReason::startup);
    runtime_.runServiceTaskAndWait([this]() {
        wiring_.startServices();
        LOG_I("wiring startup complete");
    });
    LOG_I("appcontrol startup complete");
}

void AppControl::shutdown()
{
    LOG_I("appcontrol shutdown start");
    if (runtime_.isRunning()) {
        runtimeTasks_.unregisterAll();
        runtime_.runServiceTaskAndWait([this]() {
            wiring_.stopServices();
        });
        runtime_.runServiceTaskAndWait([this]() {
            wiring_.clearServices();
        });
        runtime_.runServiceTaskAndWait([this]() {
            lifecycle_.enterStopped();
        });
        devices_.stopFan(runtime_);
        devices_.clear();
    }
    runtime_.stop();
    LOG_I("runtime stopped");
    LOG_I("appcontrol shutdown complete");
}

mlcp::hmi::service::selftest::SelfTestReport AppControl::requestSelfTest()
{
    return executeSelfTest(mlcp::hmi::service::selftest::SelfTestReason::uiCommand);
}

std::string AppControl::lifecycleStateName() const
{
    return mlcp::hmi::core::state_machine::systemStateName(lifecycle_.systemState());
}

mlcp::hmi::service::LifecycleSnapshot AppControl::lifecycleSnapshot() const
{
    return lifecycle_.snapshot();
}

mlcp::hmi::service::temp::TempServiceConfig AppControl::tempServiceConfig()
{
    std::optional<mlcp::hmi::service::temp::TempServiceConfig> config;
    runtime_.runServiceTaskAndWait([this, &config]() {
        config = wiring_.tempServiceConfig();
    });

    return *config;
}

mlcp::hmi::service::temp::TempServiceSnapshot AppControl::tempServiceSnapshot()
{
    std::optional<mlcp::hmi::service::temp::TempServiceSnapshot> snapshot;
    runtime_.runServiceTaskAndWait([this, &snapshot]() {
        snapshot = wiring_.tempServiceSnapshot();
    });

    return *snapshot;
}

void AppControl::applyTempServiceConfig(
    const mlcp::hmi::service::temp::TempServiceConfig& config)
{
    runtime_.runServiceTaskAndWait([this, config]() {
        wiring_.applyTempServiceConfig(config);
    });
}

mlcp::hmi::service::selftest::SelfTestReport AppControl::executeSelfTest(
    mlcp::hmi::service::selftest::SelfTestReason reason)
{
    using mlcp::hmi::core::state_machine::SystemState;
    using mlcp::hmi::service::selftest::SelfTestReport;

    std::optional<SelfTestReport> report;
    runtime_.runServiceTaskAndWait([this, reason, &report]() {
        if (reason == mlcp::hmi::service::selftest::SelfTestReason::uiCommand
            && lifecycle_.systemState() == SystemState::running) {
            throw std::runtime_error("selftest is not allowed while system is running");
        }

        report = selfTestFlow_.run(reason);
    });

    return *report;
}

void AppControl::reportLifecycleComponentStateDirect(
    mlcp::hmi::service::LifecycleComponent component,
    mlcp::hmi::service::LifecycleComponentState componentState)
{
    lifecycle_.reportComponentState(component, componentState);
}

void AppControl::reportLifecycleComponentState(
    mlcp::hmi::service::LifecycleComponent component,
    mlcp::hmi::service::LifecycleComponentState componentState)
{
    runtime_.runServiceTaskAndWait([this, component, componentState]() {
        lifecycle_.reportComponentState(component, componentState);
    });
}

} // namespace mlcp::hmi::system
