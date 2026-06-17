#include "device/temp/temperatureSensor.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace mlcp::hmi::device::temp {
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

long parseLong(const std::string& text, const std::string& name)
{
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(text.c_str(), &end, 0);
    if (errno != 0 || end == text.c_str()) {
        throw std::runtime_error("invalid integer for " + name + ": " + text);
    }
    return value;
}

} // namespace

TemperatureSensor::TemperatureSensor(std::string temperaturePath)
    : temperaturePath_(std::move(temperaturePath))
{
}

double TemperatureSensor::readCelsius() const
{
    return static_cast<double>(parseLong(readTextFile(temperaturePath_), "temperature")) /
           1000.0;
}

const std::string& TemperatureSensor::temperaturePath() const
{
    return temperaturePath_;
}

std::string findLm75bdTemperaturePath()
{
    const std::vector<fs::path> candidates{
        "/sys/bus/i2c/devices/0-0048/temperature",
        "/sys/bus/i2c/drivers/astroceta-lm75bd/0-0048/temperature",
    };

    for (const auto& candidate : candidates) {
        if (pathExists(candidate)) {
            return candidate.string();
        }
    }

    const fs::path i2cDevices = "/sys/bus/i2c/devices";
    if (pathExists(i2cDevices)) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(i2cDevices, ec)) {
            const fs::path temperaturePath = entry.path() / "temperature";
            const std::string deviceName = entry.path().filename().string();
            if (deviceName.find("-0048") != std::string::npos &&
                pathExists(temperaturePath)) {
                return temperaturePath.string();
            }
        }
    }

    throw std::runtime_error("could not find LM75BD temperature sysfs file");
}

} // namespace mlcp::hmi::device::temp
