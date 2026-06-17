#pragma once

#include <string>
#include <vector>

namespace mlcp::hmi::device::fan {

constexpr int kPwmMin = 0;
constexpr int kPwmMax = 255;

class FanController {
public:
    FanController(std::string fanDirectory, std::vector<int> channels);

    void setDuty(int duty) const;
    void setDutyForDryRun(int duty) const;

    const std::string& fanDirectory() const;
    const std::vector<int>& channels() const;

private:
    void writeDuty(int duty, bool dryRun) const;

    std::string fanDirectory_;
    std::vector<int> channels_;
};

std::string findRumiFanDirectory();
bool isValidFanChannel(int channel);

} // namespace mlcp::hmi::device::fan
