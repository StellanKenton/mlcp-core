#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace mlcp::hmi::device::sysinfo {

struct MemoryInfo {
    std::uint64_t totalKb {0U};
    std::uint64_t availableKb {0U};
    std::uint64_t usedKb {0U};
    double usagePercent {0.0};
};

struct SystemInfoSnapshot {
    MemoryInfo memory;
    double cpuUsagePercent {0.0};
};

class SystemInfoReader {
public:
    SystemInfoReader();
    SystemInfoReader(std::string statPath, std::string meminfoPath);

    MemoryInfo readMemoryInfo() const;
    double readCpuUsagePercent(std::chrono::milliseconds sampleInterval) const;
    double readCpuUsagePercentSinceLastSample() const;
    SystemInfoSnapshot readSnapshot(std::chrono::milliseconds cpuSampleInterval) const;
    SystemInfoSnapshot readSnapshotSinceLastSample() const;

    const std::string& statPath() const;
    const std::string& meminfoPath() const;

private:
    std::string statPath_;
    std::string meminfoPath_;
    mutable std::optional<std::array<std::uint64_t, 8U>> previousCpuTimes_;
};

} // namespace mlcp::hmi::device::sysinfo
