#pragma once

#include <functional>
#include <optional>

#include "persistence/storage/tempServiceConfigStore.h"
#include "service/lifecycle/lifecycle.h"
#include "service/selftest/selfTestFlow.h"
#include "service/serviceHost.h"
#include "service/temp/tempService.h"
#include "system/appcontrol/applicationServiceRegistry.h"
#include "system/appcontrol/deviceRegistry.h"
#include "system/appcontrol/runtimeTaskRegistry.h"
#include "system/runtime/runtime.h"

namespace mlcp::hmi::system {

class ApplicationWiring {
public:
    using LifecycleReporter =
        std::function<void(mlcp::hmi::service::LifecycleComponent,
                           mlcp::hmi::service::LifecycleComponentState)>;

    void initializeServices(DeviceRegistry& devices,
                            Runtime& runtime,
                            RuntimeTaskRegistry& taskRegistry,
                            const LifecycleReporter& scheduledReporter,
                            const LifecycleReporter& serviceThreadReporter);
    void configureSelfTestChecks(mlcp::hmi::service::selftest::SelfTestFlow& selfTestFlow,
                                 DeviceRegistry& devices,
                                 Runtime& runtime);
    void startServices();
    void stopServices();
    void clearServices();

    mlcp::hmi::service::temp::TempServiceConfig tempServiceConfig();
    mlcp::hmi::service::temp::TempServiceSnapshot tempServiceSnapshot();
    void applyTempServiceConfig(const mlcp::hmi::service::temp::TempServiceConfig& config);

private:
    void reportTempServiceState(mlcp::hmi::service::temp::TempServiceState state,
                                const LifecycleReporter& reporter) const;

    mlcp::hmi::service::ServiceHost serviceHost_;
    ApplicationServiceRegistry serviceRegistry_;
    mlcp::hmi::persistence::storage::TempServiceConfigStore tempServiceConfigStore_;
    std::optional<mlcp::hmi::service::temp::TempService> tempService_;
};

} // namespace mlcp::hmi::system
