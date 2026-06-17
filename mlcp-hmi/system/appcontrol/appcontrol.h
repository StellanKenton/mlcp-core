#pragma once

#include <atomic>
#include <string>

#include "bridge/hmiBackend.h"
#include "service/lifecycle/lifecycle.h"
#include "service/selftest/selfTestFlow.h"
#include "service/temp/tempService.h"
#include "system/appcontrol/applicationWiring.h"
#include "system/appcontrol/deviceRegistry.h"
#include "system/appcontrol/runtimeTaskRegistry.h"
#include "system/runtime/runtime.h"

namespace mlcp::hmi::system {

class AppControl : public mlcp::hmi::bridge::IHmiBackend {
public:
    AppControl();

    void startup();
    void shutdown();
    std::string lifecycleStateName() const override;
    mlcp::hmi::service::LifecycleSnapshot lifecycleSnapshot() const override;
    mlcp::hmi::service::selftest::SelfTestReport requestSelfTest();
    mlcp::hmi::service::temp::TempServiceConfig tempServiceConfig() override;
    mlcp::hmi::service::temp::TempServiceSnapshot tempServiceSnapshot() override;
    void applyTempServiceConfig(
        const mlcp::hmi::service::temp::TempServiceConfig& config) override;

private:
    mlcp::hmi::service::selftest::SelfTestReport executeSelfTest(
        mlcp::hmi::service::selftest::SelfTestReason reason);
    void reportLifecycleComponentStateDirect(
        mlcp::hmi::service::LifecycleComponent component,
        mlcp::hmi::service::LifecycleComponentState componentState);
    void reportLifecycleComponentState(
        mlcp::hmi::service::LifecycleComponent component,
        mlcp::hmi::service::LifecycleComponentState componentState);

    mlcp::hmi::service::Lifecycle lifecycle_;
    Runtime runtime_;
    RuntimeTaskRegistry runtimeTasks_;
    DeviceRegistry devices_;
    ApplicationWiring wiring_;
    mlcp::hmi::service::selftest::SelfTestFlow selfTestFlow_;

    std::atomic<bool> requestStop_ {false};
};

} // namespace mlcp::hmi::system
