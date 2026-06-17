#include "system/appcontrol/applicationServiceRegistry.h"

#include <stdexcept>
#include <utility>

namespace mlcp::hmi::system {

void ApplicationServiceRegistry::add(ApplicationServiceRegistration registration)
{
    if (registration.service == nullptr) {
        throw std::invalid_argument("service registration requires service");
    }
    if (!registration.selfTestCheck.run) {
        throw std::invalid_argument("service registration requires selftest check");
    }

    registrations_.push_back(std::move(registration));
}

void ApplicationServiceRegistry::installServices(
    mlcp::hmi::service::ServiceHost& serviceHost,
    Runtime& runtime,
    RuntimeTaskRegistry& taskRegistry)
{
    for (const auto& registration : registrations_) {
        auto* service = registration.service;
        runtime.runServiceTaskAndWait([&serviceHost, service]() {
            serviceHost.add(*service);
        });
        taskRegistry.registerServiceRuntimeTasks(*service);
    }
}

void ApplicationServiceRegistry::configureSelfTestChecks(
    mlcp::hmi::service::selftest::SelfTestFlow& selfTestFlow) const
{
    for (const auto& registration : registrations_) {
        selfTestFlow.addCheck(registration.selfTestCheck);
    }
}

void ApplicationServiceRegistry::clear()
{
    registrations_.clear();
}

} // namespace mlcp::hmi::system
