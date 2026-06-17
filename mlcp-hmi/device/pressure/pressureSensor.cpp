#include "device/pressure/pressureSensor.h"

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace mlcp::hmi::device::pressure {
namespace {

bool pathExists(const fs::path& path)
{
    std::error_code ec;
    return fs::exists(path, ec);
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open for read: " + path.string() +
                                 ": " + std::strerror(errno));
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        throw std::runtime_error("failed to read: " + path.string());
    }
    return buffer.str();
}

bool hasOnlyTrailingSpace(const char* cursor)
{
    while (*cursor != '\0') {
        if (std::isspace(static_cast<unsigned char>(*cursor)) == 0) {
            return false;
        }
        ++cursor;
    }
    return true;
}

long long parseSignedInteger(const std::string& text, const std::string& name)
{
    char* end = nullptr;
    errno = 0;
    const long long value = std::strtoll(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || !hasOnlyTrailingSpace(end)) {
        throw std::runtime_error("invalid integer for " + name + ": " + text);
    }
    return value;
}

int parseInt(const std::string& text, const std::string& name)
{
    const long long value = parseSignedInteger(text, name);
    if (value < std::numeric_limits<int>::min() ||
        value > std::numeric_limits<int>::max()) {
        throw std::runtime_error("integer out of range for " + name + ": " + text);
    }
    return static_cast<int>(value);
}

std::uint32_t parseUnsigned32(const std::string& text, const std::string& name)
{
    const long long value = parseSignedInteger(text, name);
    if (value < 0 || value > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("unsigned integer out of range for " + name + ": " + text);
    }
    return static_cast<std::uint32_t>(value);
}

bool looksLikePressureDirectory(const fs::path& directory)
{
    return pathExists(directory / "raw") &&
           pathExists(directory / "voltage_uv") &&
           pathExists(directory / "voltage_mv") &&
           pathExists(directory / "pressure_kpa");
}

} // namespace

PressureSensor::PressureSensor(std::string pressureDirectory)
    : pressureDirectory_(std::move(pressureDirectory))
{
    if (pressureDirectory_.empty()) {
        throw std::invalid_argument("pressure directory cannot be empty");
    }
}

int PressureSensor::readRaw() const
{
    return parseInt(readTextFile(attributePath("raw")), "pressure raw");
}

std::uint32_t PressureSensor::readVoltageMicrovolt() const
{
    return parseUnsigned32(readTextFile(attributePath("voltage_uv")), "pressure voltage_uv");
}

std::uint32_t PressureSensor::readVoltageMillivolt() const
{
    return parseUnsigned32(readTextFile(attributePath("voltage_mv")), "pressure voltage_mv");
}

std::uint32_t PressureSensor::readPressureKpa() const
{
    return parseUnsigned32(readTextFile(attributePath("pressure_kpa")), "pressure_kpa");
}

std::string PressureSensor::readPressureRange() const
{
    return readTextFile(attributePath("pressure_range"));
}

PressureSnapshot PressureSensor::readSnapshot() const
{
    PressureSnapshot snapshot;
    snapshot.raw = readRaw();
    snapshot.voltageMicrovolt = readVoltageMicrovolt();
    snapshot.voltageMillivolt = readVoltageMillivolt();
    snapshot.pressureKpa = readPressureKpa();
    return snapshot;
}

const std::string& PressureSensor::pressureDirectory() const
{
    return pressureDirectory_;
}

std::string PressureSensor::attributePath(const std::string& attributeName) const
{
    const fs::path base = pressureDirectory_;
    return (base / attributeName).string();
}

std::string findRumiPressureDirectory()
{
    const std::vector<fs::path> candidates{
        "/sys/bus/platform/devices/rumipressure",
        "/sys/devices/platform/rumipressure",
    };

    for (const auto& candidate : candidates) {
        if (looksLikePressureDirectory(candidate)) {
            return candidate.string();
        }
    }

    return findRumiPressureDirectoryUnder("/sys/bus/platform/devices");
}

std::string findRumiPressureDirectoryUnder(const std::string& platformDevicesDirectory)
{
    const fs::path platformDevices = platformDevicesDirectory;
    if (pathExists(platformDevices)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(platformDevices, ec)) {
            if (looksLikePressureDirectory(entry.path())) {
                return entry.path().string();
            }
        }
    }

    throw std::runtime_error("could not find rumi-pressure sysfs directory");
}

} // namespace mlcp::hmi::device::pressure
