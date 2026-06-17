#pragma once

#include <vector>

#include "system/runtime/runtime.h"

namespace mlcp::hmi::service {

class IService {
public:
    virtual ~IService() = default;

    virtual const char* name() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void tick() = 0;
    virtual std::vector<mlcp::hmi::system::RuntimePeriodicTask> runtimeTasks()
    {
        return {};
    }
};

} // namespace mlcp::hmi::service
