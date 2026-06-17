#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "service/lifecycle/lifecycle.h"

namespace mlcp::hmi::service::selftest {

enum class SelfTestReason {
    startup,
    uiCommand,
};

enum class SelfTestSeverity {
    required,
    optional,
};

struct SelfTestCheckResult {
    std::string name;
    LifecycleComponent component {LifecycleComponent::selftest};
    SelfTestSeverity severity {SelfTestSeverity::required};
    bool passed {false};
    std::string message;
};

struct SelfTestReport {
    SelfTestReason reason {SelfTestReason::startup};
    std::uint64_t sequence {0U};
    bool passed {false};
    bool hasRequiredFailure {false};
    std::vector<SelfTestCheckResult> results;
};

struct SelfTestCheck {
    std::string name;
    LifecycleComponent component {LifecycleComponent::selftest};
    SelfTestSeverity severity {SelfTestSeverity::required};
    std::function<SelfTestCheckResult()> run;
};

class SelfTestFlow {
public:
    using ComponentReporter = std::function<void(LifecycleComponent, LifecycleComponentState)>;

    explicit SelfTestFlow(ComponentReporter componentReporter);

    void clearChecks();
    void addCheck(SelfTestCheck check);
    SelfTestReport run(SelfTestReason reason);
    SelfTestReport lastReport() const;

private:
    SelfTestCheckResult runCheck(const SelfTestCheck& check) const;
    void reportComponentState(LifecycleComponent component, LifecycleComponentState state) const;
    static LifecycleComponentState componentStateForResult(
        const SelfTestCheckResult& result);
    static LifecycleComponentState selfTestStateForReport(const SelfTestReport& report);

    ComponentReporter componentReporter_;
    std::vector<SelfTestCheck> checks_;
    SelfTestReport lastReport_;
    std::uint64_t nextSequence_ {1U};
};

const char* selfTestReasonName(SelfTestReason reason);
const char* selfTestSeverityName(SelfTestSeverity severity);

} // namespace mlcp::hmi::service::selftest
