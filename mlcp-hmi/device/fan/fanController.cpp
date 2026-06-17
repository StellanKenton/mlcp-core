#include "device/fan/fanController.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace mlcp::hmi::device::fan {
namespace {

bool pathExists(const fs::path& path)
{
    std::error_code ec;
    return fs::exists(path, ec);
}

void writeTextFile(const fs::path& path, const std::string& value, bool dryRun)
{
    if (dryRun) {
        std::cout << "dry-run write " << path << " <- " << value;
        return;
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("failed to open for write: " + path.string() +
                                 ": " + std::strerror(errno));
    }

    output << value;
    if (!output) {
        throw std::runtime_error("failed to write: " + path.string());
    }
}

bool looksLikeFanDirectory(const fs::path& directory)
{
    return pathExists(directory / "pwm0") &&
           pathExists(directory / "pwm1") &&
           pathExists(directory / "enable0") &&
           pathExists(directory / "enable1");
}

void validateChannels(const std::vector<int>& channels)
{
    if (channels.empty()) {
        throw std::invalid_argument("fan channels cannot be empty");
    }

    for (const int channel : channels) {
        if (!isValidFanChannel(channel)) {
            throw std::invalid_argument("fan channel only supports 0 and 1");
        }
    }
}

} // namespace

FanController::FanController(std::string fanDirectory, std::vector<int> channels)
    : fanDirectory_(std::move(fanDirectory)),
      channels_(std::move(channels))
{
    validateChannels(channels_);
}

void FanController::setDuty(int duty) const
{
    writeDuty(duty, false);
}

void FanController::setDutyForDryRun(int duty) const
{
    writeDuty(duty, true);
}

const std::string& FanController::fanDirectory() const
{
    return fanDirectory_;
}

const std::vector<int>& FanController::channels() const
{
    return channels_;
}

void FanController::writeDuty(int duty, bool dryRun) const
{
    duty = std::clamp(duty, kPwmMin, kPwmMax);

    for (const int channel : channels_) {
        const fs::path base = fanDirectory_;
        const fs::path enablePath = base / ("enable" + std::to_string(channel));
        const fs::path pwmPath = base / ("pwm" + std::to_string(channel));

        if (duty > 0) {
            writeTextFile(enablePath, "1\n", dryRun);
            writeTextFile(pwmPath, std::to_string(duty) + "\n", dryRun);
        } else {
            writeTextFile(pwmPath, "0\n", dryRun);
            writeTextFile(enablePath, "0\n", dryRun);
        }
    }
}

std::string findRumiFanDirectory()
{
    const std::vector<fs::path> candidates{
        "/sys/bus/platform/devices/rumifan",
        "/sys/devices/platform/rumifan",
    };

    for (const auto& candidate : candidates) {
        if (looksLikeFanDirectory(candidate)) {
            return candidate.string();
        }
    }

    const fs::path platformDevices = "/sys/bus/platform/devices";
    if (pathExists(platformDevices)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(platformDevices, ec)) {
            if (looksLikeFanDirectory(entry.path())) {
                return entry.path().string();
            }
        }
    }

    throw std::runtime_error("could not find rumi-fan sysfs directory");
}

bool isValidFanChannel(int channel)
{
    return channel == 0 || channel == 1;
}

} // namespace mlcp::hmi::device::fan
