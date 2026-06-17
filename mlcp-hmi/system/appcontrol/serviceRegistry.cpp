#include "system/appcontrol/serviceRegistry.h"

#include <stdexcept>
#include <utility>

namespace mlcp::hmi::system {

void ServiceRegistry::add(ServiceRegistration registration)
{
    if (registration.service == nullptr) {
        throw std::invalid_argument("service registration requires service");
    }
    if (!registration.selfTestCheck.run) {
        throw std::invalid_argument("service registration requires selftest check");
    }

    registrations_.push_back(std::move(registration));
}

void ServiceRegistry::installServices(mlcp::hmi::service::ServiceHost& serviceHost,
                                      Runtime& runtime,
                                      TaskRegistry& taskRegistry)
{
    for (const auto& registration : registrations_) {
        auto* service = registration.service;
        runtime.runServiceTaskAndWait([&serviceHost, service]() {
            serviceHost.add(*service);
        });
        taskRegistry.registerServiceRuntimeTasks(*service);
    }
}

void ServiceRegistry::configureSelfTestChecks(
    mlcp::hmi::service::selftest::SelfTestFlow& selfTestFlow) const
{
    for (const auto& registration : registrations_) {
        selfTestFlow.addCheck(registration.selfTestCheck);
    }
}

void ServiceRegistry::clear()
{
    registrations_.clear();
}

} // namespace mlcp::hmi::system
