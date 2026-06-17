#pragma once

#include <vector>

#include "service/selftest/selfTestFlow.h"
#include "service/service.h"
#include "service/serviceHost.h"
#include "system/appcontrol/runtimeTaskRegistry.h"
#include "system/runtime/runtime.h"

namespace mlcp::hmi::system {

struct ApplicationServiceRegistration {
    mlcp::hmi::service::IService* service {nullptr};
    mlcp::hmi::service::selftest::SelfTestCheck selfTestCheck;
};

class ApplicationServiceRegistry {
public:
    void add(ApplicationServiceRegistration registration);
    void installServices(mlcp::hmi::service::ServiceHost& serviceHost,
                         Runtime& runtime,
                         RuntimeTaskRegistry& taskRegistry);
    void configureSelfTestChecks(
        mlcp::hmi::service::selftest::SelfTestFlow& selfTestFlow) const;
    void clear();

private:
    std::vector<ApplicationServiceRegistration> registrations_;
};

} // namespace mlcp::hmi::system
