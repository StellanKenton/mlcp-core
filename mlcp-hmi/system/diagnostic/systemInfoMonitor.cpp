#include "system/diagnostic/systemInfoMonitor.h"

#include <exception>
#include <stdexcept>
#include <utility>

namespace mlcp::hmi::system::diagnostic {

SystemInfoMonitor::SystemInfoMonitor(SystemInfoReader systemInfoReader)
    : systemInfoReader_(std::move(systemInfoReader))
{
    if (!systemInfoReader_) {
        throw std::invalid_argument("system info reader is required");
    }
}

void SystemInfoMonitor::sample()
{
    try {
        const auto systemInfo = systemInfoReader_();
        std::lock_guard<std::mutex> lock(mutex_);
        latestSystemInfo_ = systemInfo;
        ++sampleCount_;
        lastError_.clear();
    } catch (const std::exception& error) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = error.what();
        }
        throw;
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lastError_ = "unknown exception";
        }
        throw;
    }
}

mlcp::hmi::device::sysinfo::SystemInfoSnapshot SystemInfoMonitor::latestSystemInfoOrThrow() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!lastError_.empty()) {
        throw std::runtime_error(lastError_);
    }
    if (latestSystemInfo_.has_value()) {
        return *latestSystemInfo_;
    }

    throw std::runtime_error("system info sample is not ready");
}

SystemInfoMonitorSnapshot SystemInfoMonitor::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    SystemInfoMonitorSnapshot result;
    result.latestSystemInfo = latestSystemInfo_;
    result.sampleCount = sampleCount_;
    result.lastError = lastError_;
    return result;
}

} // namespace mlcp::hmi::system::diagnostic
