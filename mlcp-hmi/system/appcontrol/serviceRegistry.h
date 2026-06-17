#pragma once

#include <vector>

#include "service/selftest/selfTestFlow.h"
#include "service/service.h"
#include "service/serviceHost.h"
#include "system/appcontrol/taskRegistry.h"
#include "system/runtime/runtime.h"

namespace mlcp::hmi::system {

struct ServiceRegistration {
    mlcp::hmi::service::IService* service {nullptr};
    mlcp::hmi::service::selftest::SelfTestCheck selfTestCheck;
};

class ServiceRegistry {
public:
    void add(ServiceRegistration registration);
    void installServices(mlcp::hmi::service::ServiceHost& serviceHost,
                         Runtime& runtime,
                         TaskRegistry& taskRegistry);
    void configureSelfTestChecks(
        mlcp::hmi::service::selftest::SelfTestFlow& selfTestFlow) const;
    void clear();

private:
    std::vector<ServiceRegistration> registrations_;
};

} // namespace mlcp::hmi::system
