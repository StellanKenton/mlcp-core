#include "service/selftest/selfTestFlow.h"

#include <cassert>
#include <map>
#include <stdexcept>

namespace {

using mlcp::hmi::service::LifecycleComponent;
using mlcp::hmi::service::LifecycleComponentState;
using mlcp::hmi::service::selftest::SelfTestCheck;
using mlcp::hmi::service::selftest::SelfTestCheckResult;
using mlcp::hmi::service::selftest::SelfTestFlow;
using mlcp::hmi::service::selftest::SelfTestReason;
using mlcp::hmi::service::selftest::SelfTestSeverity;

SelfTestCheck makeCheck(const char* name,
                        LifecycleComponent component,
                        SelfTestSeverity severity,
                        bool passed)
{
    return SelfTestCheck{
        name,
        component,
        severity,
        [passed]() {
            return SelfTestCheckResult {"", {}, {}, passed, passed ? "ok" : "failed"};
        },
    };
}

void testSuccessfulStartupSelfTest()
{
    std::map<LifecycleComponent, LifecycleComponentState> states;
    SelfTestFlow flow([&states](LifecycleComponent component, LifecycleComponentState state) {
        states[component] = state;
    });

    flow.addCheck(makeCheck(
        "fan", LifecycleComponent::fan, SelfTestSeverity::required, true));
    flow.addCheck(makeCheck(
        "systemInfo", LifecycleComponent::systemInfoReader, SelfTestSeverity::optional, true));

    const auto report = flow.run(SelfTestReason::startup);

    assert(report.reason == SelfTestReason::startup);
    assert(report.sequence == 1U);
    assert(report.passed);
    assert(!report.hasRequiredFailure);
    assert(report.results.size() == 2U);
    assert(states.at(LifecycleComponent::selftest) == LifecycleComponentState::ready);
    assert(states.at(LifecycleComponent::fan) == LifecycleComponentState::ready);
}

void testRequiredFailureMarksFault()
{
    std::map<LifecycleComponent, LifecycleComponentState> states;
    SelfTestFlow flow([&states](LifecycleComponent component, LifecycleComponentState state) {
        states[component] = state;
    });

    flow.addCheck(makeCheck(
        "temperature", LifecycleComponent::temperatureSensor, SelfTestSeverity::required, false));

    const auto report = flow.run(SelfTestReason::uiCommand);

    assert(report.reason == SelfTestReason::uiCommand);
    assert(!report.passed);
    assert(report.hasRequiredFailure);
    assert(states.at(LifecycleComponent::temperatureSensor) == LifecycleComponentState::fault);
    assert(states.at(LifecycleComponent::selftest) == LifecycleComponentState::fault);
}

void testOptionalFailureMarksDegraded()
{
    std::map<LifecycleComponent, LifecycleComponentState> states;
    SelfTestFlow flow([&states](LifecycleComponent component, LifecycleComponentState state) {
        states[component] = state;
    });

    flow.addCheck(makeCheck(
        "systemInfo", LifecycleComponent::systemInfoReader, SelfTestSeverity::optional, false));

    const auto report = flow.run(SelfTestReason::uiCommand);

    assert(!report.passed);
    assert(!report.hasRequiredFailure);
    assert(states.at(LifecycleComponent::systemInfoReader) == LifecycleComponentState::degraded);
    assert(states.at(LifecycleComponent::selftest) == LifecycleComponentState::degraded);
}

void testExceptionBecomesFailedResult()
{
    std::map<LifecycleComponent, LifecycleComponentState> states;
    SelfTestFlow flow([&states](LifecycleComponent component, LifecycleComponentState state) {
        states[component] = state;
    });

    flow.addCheck(SelfTestCheck{
        "fan",
        LifecycleComponent::fan,
        SelfTestSeverity::required,
        []() -> SelfTestCheckResult {
            throw std::runtime_error("io timeout");
        },
    });

    const auto report = flow.run(SelfTestReason::uiCommand);

    assert(!report.passed);
    assert(report.results.size() == 1U);
    assert(report.results[0].message == "io timeout");
    assert(states.at(LifecycleComponent::fan) == LifecycleComponentState::fault);
}

} // namespace

int main()
{
    testSuccessfulStartupSelfTest();
    testRequiredFailureMarksFault();
    testOptionalFailureMarksDegraded();
    testExceptionBecomesFailedResult();

    return 0;
}
