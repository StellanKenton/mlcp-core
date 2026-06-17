#include "device/sysinfo/systemInfoReader.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace mlcp::hmi::device::sysinfo {
namespace {

struct CpuTimes {
    std::uint64_t user {0U};
    std::uint64_t nice {0U};
    std::uint64_t system {0U};
    std::uint64_t idle {0U};
    std::uint64_t iowait {0U};
    std::uint64_t irq {0U};
    std::uint64_t softirq {0U};
    std::uint64_t steal {0U};
};

std::string readTextFile(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open for read: " + path + ": " +
                                 std::strerror(errno));
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        throw std::runtime_error("failed to read: " + path);
    }
    return buffer.str();
}

std::uint64_t parseUnsigned(const std::string& text, const std::string& name)
{
    char* end = nullptr;
    errno = 0;
    const unsigned long long value = std::strtoull(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str()) {
        throw std::runtime_error("invalid unsigned integer for " + name + ": " + text);
    }
    return static_cast<std::uint64_t>(value);
}

CpuTimes parseCpuTimes(const std::string& statText)
{
    std::istringstream lines(statText);
    std::string line;

    while (std::getline(lines, line)) {
        std::istringstream parser(line);
        std::string label;
        parser >> label;
        if (label != "cpu") {
            continue;
        }

        CpuTimes times;
        if (!(parser >> times.user >> times.nice >> times.system >> times.idle >>
              times.iowait >> times.irq >> times.softirq >> times.steal)) {
            throw std::runtime_error("invalid cpu line in proc stat");
        }
        return times;
    }

    throw std::runtime_error("missing cpu line in proc stat");
}

std::array<std::uint64_t, 8U> toRawCpuTimes(const CpuTimes& times)
{
    return {times.user, times.nice,    times.system,  times.idle,
            times.iowait, times.irq,   times.softirq, times.steal};
}

CpuTimes fromRawCpuTimes(const std::array<std::uint64_t, 8U>& values)
{
    CpuTimes times;
    times.user = values[0U];
    times.nice = values[1U];
    times.system = values[2U];
    times.idle = values[3U];
    times.iowait = values[4U];
    times.irq = values[5U];
    times.softirq = values[6U];
    times.steal = values[7U];
    return times;
}

std::uint64_t idleTicks(const CpuTimes& times)
{
    return times.idle + times.iowait;
}

std::uint64_t totalTicks(const CpuTimes& times)
{
    return times.user + times.nice + times.system + times.idle + times.iowait +
           times.irq + times.softirq + times.steal;
}

double calculateCpuUsagePercent(const CpuTimes& before, const CpuTimes& after)
{
    const std::uint64_t beforeTotal = totalTicks(before);
    const std::uint64_t afterTotal = totalTicks(after);
    const std::uint64_t beforeIdle = idleTicks(before);
    const std::uint64_t afterIdle = idleTicks(after);

    if (afterTotal <= beforeTotal || afterIdle < beforeIdle) {
        return 0.0;
    }

    const std::uint64_t totalDelta = afterTotal - beforeTotal;
    const std::uint64_t idleDelta = afterIdle - beforeIdle;
    if (totalDelta == 0U || idleDelta > totalDelta) {
        return 0.0;
    }

    return static_cast<double>(totalDelta - idleDelta) * 100.0 /
           static_cast<double>(totalDelta);
}

MemoryInfo parseMemoryInfo(const std::string& meminfoText)
{
    std::istringstream lines(meminfoText);
    std::string key;
    std::string valueText;
    std::string unit;
    MemoryInfo result;

    while (lines >> key >> valueText >> unit) {
        if (key == "MemTotal:") {
            result.totalKb = parseUnsigned(valueText, "MemTotal");
        } else if (key == "MemAvailable:") {
            result.availableKb = parseUnsigned(valueText, "MemAvailable");
        }
    }

    if (result.totalKb == 0U) {
        throw std::runtime_error("missing MemTotal in proc meminfo");
    }
    if (result.availableKb > result.totalKb) {
        throw std::runtime_error("MemAvailable exceeds MemTotal in proc meminfo");
    }

    result.usedKb = result.totalKb - result.availableKb;
    result.usagePercent = static_cast<double>(result.usedKb) * 100.0 /
                          static_cast<double>(result.totalKb);
    return result;
}

} // namespace

SystemInfoReader::SystemInfoReader()
    : SystemInfoReader("/proc/stat", "/proc/meminfo")
{
}

SystemInfoReader::SystemInfoReader(std::string statPath, std::string meminfoPath)
    : statPath_(std::move(statPath)),
      meminfoPath_(std::move(meminfoPath))
{
    if (statPath_.empty()) {
        throw std::invalid_argument("stat path cannot be empty");
    }
    if (meminfoPath_.empty()) {
        throw std::invalid_argument("meminfo path cannot be empty");
    }
}

MemoryInfo SystemInfoReader::readMemoryInfo() const
{
    return parseMemoryInfo(readTextFile(meminfoPath_));
}

double SystemInfoReader::readCpuUsagePercent(std::chrono::milliseconds sampleInterval) const
{
    if (sampleInterval.count() < 0) {
        throw std::invalid_argument("cpu sample interval cannot be negative");
    }

    const CpuTimes before = parseCpuTimes(readTextFile(statPath_));
    if (sampleInterval.count() > 0) {
        std::this_thread::sleep_for(sampleInterval);
    }
    const CpuTimes after = parseCpuTimes(readTextFile(statPath_));
    return calculateCpuUsagePercent(before, after);
}

double SystemInfoReader::readCpuUsagePercentSinceLastSample() const
{
    const CpuTimes current = parseCpuTimes(readTextFile(statPath_));
    if (!previousCpuTimes_.has_value()) {
        previousCpuTimes_ = toRawCpuTimes(current);
        return 0.0;
    }

    const double usagePercent =
        calculateCpuUsagePercent(fromRawCpuTimes(*previousCpuTimes_), current);
    previousCpuTimes_ = toRawCpuTimes(current);
    return usagePercent;
}

SystemInfoSnapshot SystemInfoReader::readSnapshot(
    std::chrono::milliseconds cpuSampleInterval) const
{
    SystemInfoSnapshot snapshot;
    snapshot.memory = readMemoryInfo();
    snapshot.cpuUsagePercent = readCpuUsagePercent(cpuSampleInterval);
    return snapshot;
}

SystemInfoSnapshot SystemInfoReader::readSnapshotSinceLastSample() const
{
    SystemInfoSnapshot snapshot;
    snapshot.memory = readMemoryInfo();
    snapshot.cpuUsagePercent = readCpuUsagePercentSinceLastSample();
    return snapshot;
}

const std::string& SystemInfoReader::statPath() const
{
    return statPath_;
}

const std::string& SystemInfoReader::meminfoPath() const
{
    return meminfoPath_;
}

} // namespace mlcp::hmi::device::sysinfo
