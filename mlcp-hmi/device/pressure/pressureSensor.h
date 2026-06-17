#pragma once

#include <cstdint>
#include <string>

namespace mlcp::hmi::device::pressure {

struct PressureSnapshot {
    int raw {0};
    std::uint32_t voltageMicrovolt {0U};
    std::uint32_t voltageMillivolt {0U};
    std::uint32_t pressureKpa {0U};
};

class PressureSensor {
public:
    explicit PressureSensor(std::string pressureDirectory);

    int readRaw() const;
    std::uint32_t readVoltageMicrovolt() const;
    std::uint32_t readVoltageMillivolt() const;
    std::uint32_t readPressureKpa() const;
    std::string readPressureRange() const;
    PressureSnapshot readSnapshot() const;

    const std::string& pressureDirectory() const;

private:
    std::string attributePath(const std::string& attributeName) const;

    std::string pressureDirectory_;
};

std::string findRumiPressureDirectory();
std::string findRumiPressureDirectoryUnder(const std::string& platformDevicesDirectory);

} // namespace mlcp::hmi::device::pressure
