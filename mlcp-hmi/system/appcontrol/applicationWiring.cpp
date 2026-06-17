#include "system/appcontrol/applicationWiring.h"

#include <exception>
#include <stdexcept>

#include "service/log/logger.h"

namespace mlcp::hmi::system {

void ApplicationWiring::initializeServices(
    DeviceRegistry& devices,
    Runtime& runtime,
    RuntimeTaskRegistry& taskRegistry,
    const LifecycleReporter& scheduledReporter,
    const LifecycleReporter& serviceThreadReporter)
{
    serviceRegistry_.clear();
    auto* temperatureSampler = devices.temperatureSampler();
    auto* fanController = devices.fanController();
    if (temperatureSampler == nullptr || fanController == nullptr) {
        scheduledReporter(mlcp::hmi::service::LifecycleComponent::tempService,
                          mlcp::hmi::service::LifecycleComponentState::degraded);
        LOG_W("temp service skipped: temperature sensor or fan is unavailable");
        return;
    }

    try {
        tempService_.emplace(
            [temperatureSampler]() {
                return temperatureSampler->latestTemperatureCelsiusOrThrow();
            },
            [&runtime, fanController](int duty) {
                runtime.postIoTask([fanController, duty]() {
                    fanController->setDuty(duty);
                });
            },
            [this, serviceThreadReporter](mlcp::hmi::service::temp::TempServiceState state) {
                reportTempServiceState(state, serviceThreadReporter);
            });
        serviceRegistry_.add(ApplicationServiceRegistration{
            &*tempService_,
            mlcp::hmi::service::selftest::SelfTestCheck{
                "tempService",
                mlcp::hmi::service::LifecycleComponent::tempService,
                mlcp::hmi::service::selftest::SelfTestSeverity::required,
                [this]() {
                    if (!tempService_.has_value()) {
                        return mlcp::hmi::service::selftest::SelfTestCheckResult {
                            "", {}, {}, false, "temp service is unavailable"};
                    }

                    return mlcp::hmi::service::selftest::SelfTestCheckResult {
                        "", {}, {}, true, "temp service is available"};
                },
            },
        });
        try {
            const auto config = tempServiceConfigStore_.load(tempService_->config());
            tempService_->setConfig(config);
            LOG_I("temp service config loaded, path={}, target={:.2f}C, maxFanDuty={}",
                  tempServiceConfigStore_.configPath(),
                  config.targetCelsius,
                  config.maxFanDuty);
        } catch (const std::exception& error) {
            LOG_W("temp service config load skipped, path={}, error={}",
                  tempServiceConfigStore_.configPath(),
                  error.what());
        }
        serviceRegistry_.installServices(serviceHost_, runtime, taskRegistry);
        LOG_I("temp service initialized");
    } catch (const std::exception& error) {
        if (tempService_.has_value()) {
            taskRegistry.unregisterServiceRuntimeTasks(*tempService_);
        }
        if (tempService_.has_value()) {
            runtime.runServiceTaskAndWait([this]() {
                serviceHost_.remove(*tempService_);
            });
        }
        serviceRegistry_.clear();
        tempService_.reset();
        scheduledReporter(mlcp::hmi::service::LifecycleComponent::tempService,
                          mlcp::hmi::service::LifecycleComponentState::degraded);
        LOG_W("temp service initialization skipped: {}", error.what());
    }
}

void ApplicationWiring::configureSelfTestChecks(
    mlcp::hmi::service::selftest::SelfTestFlow& selfTestFlow,
    DeviceRegistry& devices,
    Runtime& runtime)
{
    using mlcp::hmi::service::LifecycleComponent;
    using mlcp::hmi::service::selftest::SelfTestCheck;
    using mlcp::hmi::service::selftest::SelfTestCheckResult;
    using mlcp::hmi::service::selftest::SelfTestSeverity;

    selfTestFlow.clearChecks();
    selfTestFlow.addCheck(SelfTestCheck{
        "fan",
        LifecycleComponent::fan,
        SelfTestSeverity::required,
        [&devices, &runtime]() {
            auto* fanController = devices.fanController();
            if (fanController == nullptr) {
                return SelfTestCheckResult {"", {}, {}, false, "fan controller is unavailable"};
            }

            runtime.runIoTaskAndWait([fanController]() {
                fanController->setDutyForDryRun(0);
            });
            return SelfTestCheckResult {"", {}, {}, true, "fan dry-run duty write passed"};
        },
    });
    selfTestFlow.addCheck(SelfTestCheck{
        "temperatureSensor",
        LifecycleComponent::temperatureSensor,
        SelfTestSeverity::required,
        [&devices, &runtime]() {
            auto* temperatureSensor = devices.temperatureSensor();
            if (temperatureSensor == nullptr) {
                return SelfTestCheckResult {"", {}, {}, false, "temperature sensor is unavailable"};
            }

            double temperature = 0.0;
            runtime.runIoTaskAndWait([temperatureSensor, &temperature]() {
                temperature = temperatureSensor->readCelsius();
            });
            if (temperature < -40.0 || temperature > 125.0) {
                return SelfTestCheckResult {
                    "", {}, {}, false, "temperature sample is out of range"};
            }

            return SelfTestCheckResult {"", {}, {}, true, "temperature sample passed"};
        },
    });
    serviceRegistry_.configureSelfTestChecks(selfTestFlow);
    selfTestFlow.addCheck(SelfTestCheck{
        "systemInfoReader",
        LifecycleComponent::systemInfoReader,
        SelfTestSeverity::optional,
        [&devices, &runtime]() {
            auto* systemInfoMonitor = devices.systemInfoMonitor();
            if (systemInfoMonitor == nullptr) {
                return SelfTestCheckResult {"", {}, {}, false, "system info reader is unavailable"};
            }

            mlcp::hmi::device::sysinfo::SystemInfoSnapshot snapshot;
            runtime.runIoTaskAndWait([systemInfoMonitor, &snapshot]() {
                systemInfoMonitor->sample();
                snapshot = systemInfoMonitor->latestSystemInfoOrThrow();
            });
            if (snapshot.memory.totalKb == 0U) {
                return SelfTestCheckResult {"", {}, {}, false, "memory total is zero"};
            }

            return SelfTestCheckResult {"", {}, {}, true, "system info sample passed"};
        },
    });
}

void ApplicationWiring::startServices()
{
    serviceHost_.startAll();
}

void ApplicationWiring::stopServices()
{
    serviceHost_.stopAll();
}

void ApplicationWiring::clearServices()
{
    serviceRegistry_.clear();
    serviceHost_.clear();
    tempService_.reset();
}

mlcp::hmi::service::temp::TempServiceConfig ApplicationWiring::tempServiceConfig()
{
    if (!tempService_.has_value()) {
        throw std::runtime_error("temp service is unavailable");
    }

    return tempService_->config();
}

mlcp::hmi::service::temp::TempServiceSnapshot ApplicationWiring::tempServiceSnapshot()
{
    if (!tempService_.has_value()) {
        throw std::runtime_error("temp service is unavailable");
    }

    return tempService_->snapshot();
}

void ApplicationWiring::applyTempServiceConfig(
    const mlcp::hmi::service::temp::TempServiceConfig& config)
{
    if (!tempService_.has_value()) {
        throw std::runtime_error("temp service is unavailable");
    }

    tempService_->setConfig(config);
    tempServiceConfigStore_.save(config);
    LOG_I("temp service config saved, path={}", tempServiceConfigStore_.configPath());
}

void ApplicationWiring::reportTempServiceState(
    mlcp::hmi::service::temp::TempServiceState state,
    const LifecycleReporter& reporter) const
{
    using mlcp::hmi::service::LifecycleComponentState;
    LifecycleComponentState componentState = LifecycleComponentState::unknown;

    switch (state) {
    case mlcp::hmi::service::temp::TempServiceState::stopped:
        componentState = LifecycleComponentState::stopped;
        break;
    case mlcp::hmi::service::temp::TempServiceState::running:
        componentState = LifecycleComponentState::running;
        break;
    case mlcp::hmi::service::temp::TempServiceState::degraded:
        componentState = LifecycleComponentState::degraded;
        break;
    }

    reporter(mlcp::hmi::service::LifecycleComponent::tempService, componentState);
}

} // namespace mlcp::hmi::system
