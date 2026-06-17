#pragma once

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

#include "service/service.h"

namespace mlcp::hmi::service {

enum class ServiceModuleState {
    registered,
    starting,
    running,
    stopping,
    stopped,
    degraded,
    failed,
};

struct ServiceModuleSnapshot {
    std::string name;
    ServiceModuleState state {ServiceModuleState::registered};
    std::uint64_t startCount {0U};
    std::uint64_t stopCount {0U};
    std::string lastError;
};

struct ServiceHostSnapshot {
    std::size_t serviceCount {0U};
    std::uint64_t startCount {0U};
    std::uint64_t stopCount {0U};
    std::string lastError;
    std::vector<ServiceModuleSnapshot> modules;
};

class ServiceHost {
public:
    void add(IService& service);
    void remove(IService& service);
    void clear();

    void startAll();
    void stopAll();

    ServiceHostSnapshot snapshot() const;

private:
    struct ServiceRecord {
        IService* service {nullptr};
        ServiceModuleSnapshot snapshot;
    };

    std::vector<IService*> servicesLocked() const;
    ServiceRecord* findRecordLocked(IService& service);
    const ServiceRecord* findRecordLocked(const IService& service) const;
    void setModuleStateLocked(IService& service, ServiceModuleState state);
    void recordErrorLocked(const std::string& serviceName,
                           const std::string& operation,
                           const std::string& error);
    void recordModuleErrorLocked(IService& service,
                                 const std::string& operation,
                                 const std::string& error,
                                 ServiceModuleState state);

    mutable std::mutex mutex_;
    std::vector<ServiceRecord> services_;
    std::uint64_t startCount_ {0U};
    std::uint64_t stopCount_ {0U};
    std::string lastError_;
};

const char* serviceModuleStateName(ServiceModuleState state);

} // namespace mlcp::hmi::service
