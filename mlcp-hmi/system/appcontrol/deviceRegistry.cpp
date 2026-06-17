#include "system/appcontrol/deviceRegistry.h"

#include <exception>
#include <string>
#include <vector>

#include "service/log/logger.h"

namespace mlcp::hmi::system {
namespace {

constexpr int kFanStopDuty = 0;

} // namespace

void DeviceRegistry::startup(Runtime& runtime,
                             TaskRegistry& taskRegistry,
                             const LifecycleReporter& reporter)
{
    initializeFan(runtime, reporter);
    initializeTemperatureSensor(runtime, taskRegistry, reporter);
    initializeSystemInfoReader(runtime, taskRegistry, reporter);
}

void DeviceRegistry::initializeFan(Runtime& runtime, const LifecycleReporter& reporter)
{
    try {
        const std::string fanDirectory = mlcp::hmi::device::fan::findRumiFanDirectory();
        fanController_.emplace(fanDirectory, std::vector<int>{0, 1});
        runtime.runIoTaskAndWait([this]() {
            fanController_->setDuty(kFanStopDuty);
        });
        reporter(mlcp::hmi::service::LifecycleComponent::fan,
                 mlcp::hmi::service::LifecycleComponentState::ready);
        LOG_I("fan initialized, directory={}", fanController_->fanDirectory());
    } catch (const std::exception& error) {
        fanController_.reset();
        reporter(mlcp::hmi::service::LifecycleComponent::fan,
                 mlcp::hmi::service::LifecycleComponentState::degraded);
        LOG_W("fan initialization skipped: {}", error.what());
    }
}

void DeviceRegistry::initializeTemperatureSensor(Runtime& runtime,
                                                 TaskRegistry& taskRegistry,
                                                 const LifecycleReporter& reporter)
{
    try {
        const std::string temperaturePath =
            mlcp::hmi::device::temp::findLm75bdTemperaturePath();
        temperatureSensor_.emplace(temperaturePath);
        double currentTemperature = 0.0;
        runtime.runIoTaskAndWait([this, &currentTemperature]() {
            currentTemperature = temperatureSensor_->readCelsius();
        });
        LOG_I("temperature sensor initialized, path={}, current={:.2f}C",
              temperatureSensor_->temperaturePath(),
              currentTemperature);
        temperatureSampler_.emplace([this]() {
            return temperatureSensor_->readCelsius();
        });
        runtime.runIoTaskAndWait([this]() {
            temperatureSampler_->sample();
        });
        taskRegistry.registerTemperatureSampler(*temperatureSampler_);
        reporter(mlcp::hmi::service::LifecycleComponent::temperatureSensor,
                 mlcp::hmi::service::LifecycleComponentState::ready);
    } catch (const std::exception& error) {
        taskRegistry.unregisterTemperatureSampler();
        temperatureSampler_.reset();
        temperatureSensor_.reset();
        reporter(mlcp::hmi::service::LifecycleComponent::temperatureSensor,
                 mlcp::hmi::service::LifecycleComponentState::degraded);
        LOG_W("temperature sensor initialization skipped: {}", error.what());
    }
}

void DeviceRegistry::initializeSystemInfoReader(Runtime& runtime,
                                                TaskRegistry& taskRegistry,
                                                const LifecycleReporter& reporter)
{
    try {
        systemInfoReader_.emplace();
        systemInfoMonitor_.emplace([this]() {
            return systemInfoReader_->readSnapshotSinceLastSample();
        });
        mlcp::hmi::device::sysinfo::SystemInfoSnapshot snapshot;
        runtime.runIoTaskAndWait([this, &snapshot]() {
            systemInfoMonitor_->sample();
            snapshot = systemInfoMonitor_->latestSystemInfoOrThrow();
        });
        LOG_I("system info initialized, statPath={}, meminfoPath={}, memoryUsed={:.2f}%, "
              "cpuUsed={:.2f}%",
              systemInfoReader_->statPath(),
              systemInfoReader_->meminfoPath(),
              snapshot.memory.usagePercent,
              snapshot.cpuUsagePercent);
        taskRegistry.registerSystemInfoMonitor(*systemInfoMonitor_);
        reporter(mlcp::hmi::service::LifecycleComponent::systemInfoReader,
                 mlcp::hmi::service::LifecycleComponentState::ready);
    } catch (const std::exception& error) {
        taskRegistry.unregisterSystemInfoMonitor();
        systemInfoMonitor_.reset();
        systemInfoReader_.reset();
        reporter(mlcp::hmi::service::LifecycleComponent::systemInfoReader,
                 mlcp::hmi::service::LifecycleComponentState::degraded);
        LOG_W("system info initialization skipped: {}", error.what());
    }
}

void DeviceRegistry::stopFan(Runtime& runtime)
{
    if (!fanController_.has_value()) {
        return;
    }

    try {
        runtime.runIoTaskAndWait([this]() {
            fanController_->setDuty(kFanStopDuty);
        });
        LOG_I("fan stopped");
    } catch (const std::exception& error) {
        LOG_W("failed to stop fan: {}", error.what());
    }
}

void DeviceRegistry::clear()
{
    systemInfoMonitor_.reset();
    systemInfoReader_.reset();
    temperatureSampler_.reset();
    temperatureSensor_.reset();
    fanController_.reset();
}

mlcp::hmi::device::fan::FanController* DeviceRegistry::fanController()
{
    return fanController_.has_value() ? &*fanController_ : nullptr;
}

mlcp::hmi::device::temp::TemperatureSensor* DeviceRegistry::temperatureSensor()
{
    return temperatureSensor_.has_value() ? &*temperatureSensor_ : nullptr;
}

mlcp::hmi::service::temp::TemperatureSampler* DeviceRegistry::temperatureSampler()
{
    return temperatureSampler_.has_value() ? &*temperatureSampler_ : nullptr;
}

mlcp::hmi::system::diagnostic::SystemInfoMonitor* DeviceRegistry::systemInfoMonitor()
{
    return systemInfoMonitor_.has_value() ? &*systemInfoMonitor_ : nullptr;
}

} // namespace mlcp::hmi::system
