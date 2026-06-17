#pragma once

#include <string>

#include "service/lifecycle/lifecycle.h"
#include "service/temp/tempService.h"

namespace mlcp::hmi::bridge {

class IHmiBackend {
public:
    virtual ~IHmiBackend() = default;

    virtual std::string lifecycleStateName() const = 0;
    virtual mlcp::hmi::service::LifecycleSnapshot lifecycleSnapshot() const = 0;
    virtual mlcp::hmi::service::temp::TempServiceConfig tempServiceConfig() = 0;
    virtual mlcp::hmi::service::temp::TempServiceSnapshot tempServiceSnapshot() = 0;
    virtual void applyTempServiceConfig(
        const mlcp::hmi::service::temp::TempServiceConfig& config) = 0;
};

} // namespace mlcp::hmi::bridge
