#include "system/diagnostic/systemInfoMonitor.h"

#include <cassert>
#include <stdexcept>
#include <string>

namespace {

using mlcp::hmi::device::sysinfo::SystemInfoSnapshot;
using mlcp::hmi::system::diagnostic::SystemInfoMonitor;

SystemInfoSnapshot makeSnapshot(std::uint64_t totalKb, double cpuUsagePercent)
{
    SystemInfoSnapshot snapshot;
    snapshot.memory.totalKb = totalKb;
    snapshot.memory.availableKb = totalKb / 2U;
    snapshot.memory.usedKb = totalKb / 2U;
    snapshot.memory.usagePercent = 50.0;
    snapshot.cpuUsagePercent = cpuUsagePercent;
    return snapshot;
}

void testStoresLatestSystemInfo()
{
    SystemInfoSnapshot currentSnapshot = makeSnapshot(1000U, 12.5);
    SystemInfoMonitor monitor([&currentSnapshot]() {
        return currentSnapshot;
    });

    monitor.sample();
    currentSnapshot = makeSnapshot(2000U, 25.0);
    monitor.sample();

    const auto snapshot = monitor.snapshot();
    assert(snapshot.latestSystemInfo.has_value());
    assert(snapshot.latestSystemInfo->memory.totalKb == 2000U);
    assert(snapshot.latestSystemInfo->cpuUsagePercent == 25.0);
    assert(snapshot.sampleCount == 2U);
    assert(snapshot.lastError.empty());
    assert(monitor.latestSystemInfoOrThrow().memory.totalKb == 2000U);
}

void testRecordsReadFailure()
{
    SystemInfoMonitor monitor([]() -> SystemInfoSnapshot {
        throw std::runtime_error("proc read failed");
    });

    bool sampleFailed = false;
    try {
        monitor.sample();
    } catch (const std::runtime_error&) {
        sampleFailed = true;
    }

    assert(sampleFailed);
    const auto snapshot = monitor.snapshot();
    assert(!snapshot.latestSystemInfo.has_value());
    assert(snapshot.sampleCount == 0U);
    assert(snapshot.lastError == "proc read failed");

    bool latestFailed = false;
    try {
        static_cast<void>(monitor.latestSystemInfoOrThrow());
    } catch (const std::runtime_error& error) {
        latestFailed = std::string(error.what()) == "proc read failed";
    }
    assert(latestFailed);
}

} // namespace

int main()
{
    testStoresLatestSystemInfo();
    testRecordsReadFailure();

    return 0;
}
