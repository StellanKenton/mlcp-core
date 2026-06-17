#include "service/selftest/selfTestFlow.h"

#include <exception>
#include <stdexcept>
#include <utility>

#include "service/log/logger.h"

namespace mlcp::hmi::service::selftest {

SelfTestFlow::SelfTestFlow(ComponentReporter componentReporter)
    : componentReporter_(std::move(componentReporter))
{
    if (!componentReporter_) {
        throw std::invalid_argument("selftest component reporter is required");
    }
}

void SelfTestFlow::clearChecks()
{
    checks_.clear();
}

void SelfTestFlow::addCheck(SelfTestCheck check)
{
    if (check.name.empty()) {
        throw std::invalid_argument("selftest check name is required");
    }

    if (!check.run) {
        throw std::invalid_argument("selftest check handler is required");
    }

    checks_.push_back(std::move(check));
}

SelfTestReport SelfTestFlow::run(SelfTestReason reason)
{
    SelfTestReport report;
    report.reason = reason;
    report.sequence = nextSequence_++;
    report.passed = true;

    LOG_I("selftest started, reason={}, sequence={}",
          selfTestReasonName(reason),
          report.sequence);
    reportComponentState(LifecycleComponent::selftest, LifecycleComponentState::running);

    for (const SelfTestCheck& check : checks_) {
        SelfTestCheckResult result = runCheck(check);
        report.passed = report.passed && result.passed;
        report.hasRequiredFailure = report.hasRequiredFailure ||
            (!result.passed && result.severity == SelfTestSeverity::required);

        reportComponentState(result.component, componentStateForResult(result));
        LOG_I("selftest check result: name={}, severity={}, passed={}, message={}",
              result.name,
              selfTestSeverityName(result.severity),
              result.passed,
              result.message);
        report.results.push_back(std::move(result));
    }

    reportComponentState(LifecycleComponent::selftest, selfTestStateForReport(report));
    lastReport_ = report;

    LOG_I("selftest finished, reason={}, sequence={}, passed={}, requiredFailure={}",
          selfTestReasonName(report.reason),
          report.sequence,
          report.passed,
          report.hasRequiredFailure);
    return report;
}

SelfTestReport SelfTestFlow::lastReport() const
{
    return lastReport_;
}

SelfTestCheckResult SelfTestFlow::runCheck(const SelfTestCheck& check) const
{
    try {
        SelfTestCheckResult result = check.run();
        result.name = check.name;
        result.component = check.component;
        result.severity = check.severity;
        if (result.message.empty()) {
            result.message = result.passed ? "ok" : "failed";
        }
        return result;
    } catch (const std::exception& error) {
        return SelfTestCheckResult{
            check.name,
            check.component,
            check.severity,
            false,
            error.what(),
        };
    } catch (...) {
        return SelfTestCheckResult{
            check.name,
            check.component,
            check.severity,
            false,
            "unknown exception",
        };
    }
}

void SelfTestFlow::reportComponentState(
    LifecycleComponent component,
    LifecycleComponentState state) const
{
    componentReporter_(component, state);
}

LifecycleComponentState SelfTestFlow::componentStateForResult(
    const SelfTestCheckResult& result)
{
    if (result.passed) {
        return LifecycleComponentState::ready;
    }

    if (result.severity == SelfTestSeverity::required) {
        return LifecycleComponentState::fault;
    }

    return LifecycleComponentState::degraded;
}

LifecycleComponentState SelfTestFlow::selfTestStateForReport(const SelfTestReport& report)
{
    if (report.passed) {
        return LifecycleComponentState::ready;
    }

    if (report.hasRequiredFailure) {
        return LifecycleComponentState::fault;
    }

    return LifecycleComponentState::degraded;
}

const char* selfTestReasonName(SelfTestReason reason)
{
    switch (reason) {
    case SelfTestReason::startup:
        return "startup";
    case SelfTestReason::uiCommand:
        return "uiCommand";
    }

    return "unknown";
}

const char* selfTestSeverityName(SelfTestSeverity severity)
{
    switch (severity) {
    case SelfTestSeverity::required:
        return "required";
    case SelfTestSeverity::optional:
        return "optional";
    }

    return "unknown";
}

} // namespace mlcp::hmi::service::selftest
