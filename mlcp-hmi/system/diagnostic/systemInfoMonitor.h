#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

#include "device/sysinfo/systemInfoReader.h"

namespace mlcp::hmi::system::diagnostic {

struct SystemInfoMonitorSnapshot {
    std::optional<mlcp::hmi::device::sysinfo::SystemInfoSnapshot> latestSystemInfo;
    std::uint64_t sampleCount {0U};
    std::string lastError;
};

class SystemInfoMonitor {
public:
    using SystemInfoReader =
        std::function<mlcp::hmi::device::sysinfo::SystemInfoSnapshot()>;

    explicit SystemInfoMonitor(SystemInfoReader systemInfoReader);

    void sample();
    mlcp::hmi::device::sysinfo::SystemInfoSnapshot latestSystemInfoOrThrow() const;
    SystemInfoMonitorSnapshot snapshot() const;

private:
    SystemInfoReader systemInfoReader_;
    mutable std::mutex mutex_;
    std::optional<mlcp::hmi::device::sysinfo::SystemInfoSnapshot> latestSystemInfo_;
    std::uint64_t sampleCount_ {0U};
    std::string lastError_;
};

} // namespace mlcp::hmi::system::diagnostic
