#pragma once

#include <string>
#include <vector>

#include "service/service.h"
#include "service/temp/tempService.h"
#include "service/temp/temperatureSampler.h"
#include "system/diagnostic/systemInfoMonitor.h"
#include "system/runtime/runtime.h"

namespace mlcp::hmi::system {

class TaskRegistry {
public:
    explicit TaskRegistry(Runtime& runtime);

    void registerTemperatureSampler(mlcp::hmi::service::temp::TemperatureSampler& sampler);
    void unregisterTemperatureSampler();
    void registerSystemInfoMonitor(
        mlcp::hmi::system::diagnostic::SystemInfoMonitor& monitor);
    void unregisterSystemInfoMonitor();
    void registerTempService(mlcp::hmi::service::temp::TempService& service);
    void unregisterTempService();
    void registerServiceRuntimeTasks(mlcp::hmi::service::IService& service);
    void unregisterServiceRuntimeTasks(mlcp::hmi::service::IService& service);
    void unregisterAll();

private:
    Runtime& runtime_;
    std::vector<std::string> serviceRuntimeTaskNames_;
};

} // namespace mlcp::hmi::system
