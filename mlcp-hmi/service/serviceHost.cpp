#include "service/serviceHost.h"

#include <algorithm>
#include <exception>

#include "service/log/logger.h"

namespace mlcp::hmi::service {

namespace {

const char* fallbackServiceName(const IService* service)
{
    if (service == nullptr || service->name() == nullptr) {
        return "unknownService";
    }

    return service->name();
}

} // namespace

void ServiceHost::add(IService& service)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = std::find_if(services_.begin(), services_.end(),
                                       [&service](const ServiceRecord& record) {
                                           return record.service == &service;
                                       });
    if (iterator != services_.end()) {
        return;
    }

    ServiceRecord record;
    record.service = &service;
    record.snapshot.name = fallbackServiceName(&service);
    record.snapshot.state = ServiceModuleState::registered;
    services_.push_back(record);
    LOG_I("service added, name={}", fallbackServiceName(&service));
}

void ServiceHost::remove(IService& service)
{
    std::lock_guard<std::mutex> lock(mutex_);
    services_.erase(std::remove_if(services_.begin(), services_.end(),
                                   [&service](const ServiceRecord& record) {
                                       return record.service == &service;
                                   }),
                    services_.end());
    LOG_I("service removed, name={}", fallbackServiceName(&service));
}

void ServiceHost::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    services_.clear();
}

void ServiceHost::startAll()
{
    const std::vector<IService*> services = servicesLocked();
    for (IService* service : services) {
        try {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                setModuleStateLocked(*service, ServiceModuleState::starting);
            }
            service->start();
            std::lock_guard<std::mutex> lock(mutex_);
            ++startCount_;
            if (ServiceRecord* record = findRecordLocked(*service)) {
                ++record->snapshot.startCount;
                record->snapshot.lastError.clear();
            }
            setModuleStateLocked(*service, ServiceModuleState::running);
        } catch (const std::exception& error) {
            std::lock_guard<std::mutex> lock(mutex_);
            recordModuleErrorLocked(*service, "start", error.what(), ServiceModuleState::failed);
        } catch (...) {
            std::lock_guard<std::mutex> lock(mutex_);
            recordModuleErrorLocked(
                *service, "start", "unknown exception", ServiceModuleState::failed);
        }
    }
}

void ServiceHost::stopAll()
{
    const std::vector<IService*> services = servicesLocked();
    for (auto iterator = services.rbegin(); iterator != services.rend(); ++iterator) {
        IService* service = *iterator;
        try {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                setModuleStateLocked(*service, ServiceModuleState::stopping);
            }
            service->stop();
            std::lock_guard<std::mutex> lock(mutex_);
            ++stopCount_;
            if (ServiceRecord* record = findRecordLocked(*service)) {
                ++record->snapshot.stopCount;
                record->snapshot.lastError.clear();
            }
            setModuleStateLocked(*service, ServiceModuleState::stopped);
        } catch (const std::exception& error) {
            std::lock_guard<std::mutex> lock(mutex_);
            recordModuleErrorLocked(*service, "stop", error.what(), ServiceModuleState::failed);
        } catch (...) {
            std::lock_guard<std::mutex> lock(mutex_);
            recordModuleErrorLocked(
                *service, "stop", "unknown exception", ServiceModuleState::failed);
        }
    }
}

ServiceHostSnapshot ServiceHost::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    ServiceHostSnapshot result;
    result.serviceCount = services_.size();
    result.startCount = startCount_;
    result.stopCount = stopCount_;
    result.lastError = lastError_;
    result.modules.reserve(services_.size());
    for (const ServiceRecord& record : services_) {
        result.modules.push_back(record.snapshot);
    }
    return result;
}

std::vector<IService*> ServiceHost::servicesLocked() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IService*> services;
    services.reserve(services_.size());
    for (const ServiceRecord& record : services_) {
        services.push_back(record.service);
    }
    return services;
}

ServiceHost::ServiceRecord* ServiceHost::findRecordLocked(IService& service)
{
    const auto iterator = std::find_if(services_.begin(), services_.end(),
                                       [&service](const ServiceRecord& record) {
                                           return record.service == &service;
                                       });
    if (iterator == services_.end()) {
        return nullptr;
    }

    return &(*iterator);
}

const ServiceHost::ServiceRecord* ServiceHost::findRecordLocked(const IService& service) const
{
    const auto iterator = std::find_if(services_.begin(), services_.end(),
                                       [&service](const ServiceRecord& record) {
                                           return record.service == &service;
                                       });
    if (iterator == services_.end()) {
        return nullptr;
    }

    return &(*iterator);
}

void ServiceHost::setModuleStateLocked(IService& service, ServiceModuleState state)
{
    ServiceRecord* record = findRecordLocked(service);
    if (record == nullptr || record->snapshot.state == state) {
        return;
    }

    record->snapshot.state = state;
    LOG_I("service module state changed: name={}, state={}",
          record->snapshot.name,
          serviceModuleStateName(state));
}

void ServiceHost::recordErrorLocked(const std::string& serviceName,
                                    const std::string& operation,
                                    const std::string& error)
{
    lastError_ = serviceName + " " + operation + " failed: " + error;
    LOG_W("service {} failed, name={}, error={}", operation, serviceName, error);
}

void ServiceHost::recordModuleErrorLocked(IService& service,
                                          const std::string& operation,
                                          const std::string& error,
                                          ServiceModuleState state)
{
    recordErrorLocked(fallbackServiceName(&service), operation, error);
    ServiceRecord* record = findRecordLocked(service);
    if (record == nullptr) {
        return;
    }

    record->snapshot.lastError = record->snapshot.name + " " + operation + " failed: " + error;
    setModuleStateLocked(service, state);
}

const char* serviceModuleStateName(ServiceModuleState state)
{
    switch (state) {
    case ServiceModuleState::registered:
        return "registered";
    case ServiceModuleState::starting:
        return "starting";
    case ServiceModuleState::running:
        return "running";
    case ServiceModuleState::stopping:
        return "stopping";
    case ServiceModuleState::stopped:
        return "stopped";
    case ServiceModuleState::degraded:
        return "degraded";
    case ServiceModuleState::failed:
        return "failed";
    }

    return "unknown";
}

} // namespace mlcp::hmi::service
