#pragma once

#include <functional>
#include <optional>
#include <string>

#include "device/fan/fanController.h"
#include "device/sysinfo/systemInfoReader.h"
#include "device/temp/temperatureSensor.h"
#include "service/lifecycle/lifecycle.h"
#include "service/temp/temperatureSampler.h"
#include "system/appcontrol/taskRegistry.h"
#include "system/diagnostic/systemInfoMonitor.h"
#include "system/runtime/runtime.h"

namespace mlcp::hmi::system {

class DeviceRegistry {
public:
    using LifecycleReporter =
        std::function<void(mlcp::hmi::service::LifecycleComponent,
                           mlcp::hmi::service::LifecycleComponentState)>;

    void startup(Runtime& runtime,
                 TaskRegistry& taskRegistry,
                 const LifecycleReporter& reporter);
    void stopFan(Runtime& runtime);
    void clear();

    mlcp::hmi::device::fan::FanController* fanController();
    mlcp::hmi::device::temp::TemperatureSensor* temperatureSensor();
    mlcp::hmi::service::temp::TemperatureSampler* temperatureSampler();
    mlcp::hmi::system::diagnostic::SystemInfoMonitor* systemInfoMonitor();

private:
    void initializeFan(Runtime& runtime, const LifecycleReporter& reporter);
    void initializeTemperatureSensor(Runtime& runtime,
                                     TaskRegistry& taskRegistry,
                                     const LifecycleReporter& reporter);
    void initializeSystemInfoReader(Runtime& runtime,
                                    TaskRegistry& taskRegistry,
                                    const LifecycleReporter& reporter);

    std::optional<mlcp::hmi::device::fan::FanController> fanController_;
    std::optional<mlcp::hmi::device::temp::TemperatureSensor> temperatureSensor_;
    std::optional<mlcp::hmi::service::temp::TemperatureSampler> temperatureSampler_;
    std::optional<mlcp::hmi::device::sysinfo::SystemInfoReader> systemInfoReader_;
    std::optional<mlcp::hmi::system::diagnostic::SystemInfoMonitor> systemInfoMonitor_;
};

} // namespace mlcp::hmi::system
