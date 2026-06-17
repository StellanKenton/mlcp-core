#include "system/appcontrol/runtimeTaskRegistry.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace mlcp::hmi::system {
namespace {

constexpr std::chrono::milliseconds kTemperatureSampleInterval {50};
constexpr std::chrono::milliseconds kSystemInfoSampleInterval {200};
constexpr const char* kTempServiceTaskName = "service.temp.tick";
constexpr const char* kTemperatureSamplerTaskName = "io.temperature.sample";
constexpr const char* kSystemInfoMonitorTaskName = "io.system.sample";

} // namespace

RuntimeTaskRegistry::RuntimeTaskRegistry(Runtime& runtime)
    : runtime_(runtime)
{
}

void RuntimeTaskRegistry::registerTemperatureSampler(
    mlcp::hmi::service::temp::TemperatureSampler& sampler)
{
    runtime_.registerPeriodicTask(RuntimePeriodicTask{
        kTemperatureSamplerTaskName,
        RuntimeThreadRole::io,
        kTemperatureSampleInterval,
        [&sampler]() {
            sampler.sample();
        },
    });
}

void RuntimeTaskRegistry::unregisterTemperatureSampler()
{
    runtime_.unregisterPeriodicTask(kTemperatureSamplerTaskName);
}

void RuntimeTaskRegistry::registerSystemInfoMonitor(
    mlcp::hmi::system::diagnostic::SystemInfoMonitor& monitor)
{
    runtime_.registerPeriodicTask(RuntimePeriodicTask{
        kSystemInfoMonitorTaskName,
        RuntimeThreadRole::io,
        kSystemInfoSampleInterval,
        [&monitor]() {
            monitor.sample();
        },
    });
}

void RuntimeTaskRegistry::unregisterSystemInfoMonitor()
{
    runtime_.unregisterPeriodicTask(kSystemInfoMonitorTaskName);
}

void RuntimeTaskRegistry::registerTempService(
    mlcp::hmi::service::temp::TempService& service)
{
    registerServiceRuntimeTasks(service);
}

void RuntimeTaskRegistry::unregisterTempService()
{
    runtime_.unregisterPeriodicTask(kTempServiceTaskName);
    const auto end = std::remove(serviceRuntimeTaskNames_.begin(),
                                 serviceRuntimeTaskNames_.end(),
                                 kTempServiceTaskName);
    serviceRuntimeTaskNames_.erase(end, serviceRuntimeTaskNames_.end());
}

void RuntimeTaskRegistry::registerServiceRuntimeTasks(mlcp::hmi::service::IService& service)
{
    for (auto task : service.runtimeTasks()) {
        const std::string taskName = task.name;
        runtime_.registerPeriodicTask(std::move(task));
        serviceRuntimeTaskNames_.push_back(taskName);
    }
}

void RuntimeTaskRegistry::unregisterServiceRuntimeTasks(mlcp::hmi::service::IService& service)
{
    for (const auto& task : service.runtimeTasks()) {
        runtime_.unregisterPeriodicTask(task.name);
        const auto end = std::remove(serviceRuntimeTaskNames_.begin(),
                                     serviceRuntimeTaskNames_.end(),
                                     task.name);
        serviceRuntimeTaskNames_.erase(end, serviceRuntimeTaskNames_.end());
    }
}

void RuntimeTaskRegistry::unregisterAll()
{
    for (const auto& taskName : serviceRuntimeTaskNames_) {
        runtime_.unregisterPeriodicTask(taskName);
    }
    serviceRuntimeTaskNames_.clear();
    unregisterTemperatureSampler();
    unregisterSystemInfoMonitor();
}

} // namespace mlcp::hmi::system
