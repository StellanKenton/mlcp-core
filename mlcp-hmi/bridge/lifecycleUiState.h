#pragma once

#include <string>

namespace mlcp::hmi::bridge {

struct LifecycleUiState {
    int stateCode {0};
    std::string stateText {"BOOTING"};
    std::string severityText {"normal"};
    bool alarmActive {false};
    bool degraded {false};
};

} // namespace mlcp::hmi::bridge
