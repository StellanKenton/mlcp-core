#pragma once

#include <string>

namespace mlcp::hmi::device::temp {

class TemperatureSensor {
public:
    explicit TemperatureSensor(std::string temperaturePath);

    double readCelsius() const;
    const std::string& temperaturePath() const;

private:
    std::string temperaturePath_;
};

std::string findLm75bdTemperaturePath();

} // namespace mlcp::hmi::device::temp
