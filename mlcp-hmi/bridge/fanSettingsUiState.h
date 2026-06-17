#pragma once

#include <optional>
#include <string>

#include "bridge/uiError.h"

namespace mlcp::hmi::bridge {

struct FanSettingsUiState {
    bool available {false};
    bool temperatureAutoControl {true};
    double targetCelsius {37.0};
    double toleranceCelsius {0.5};
    int manualFanDuty {0};
    int minFanDuty {0};
    int maxFanDuty {255};
    std::optional<double> latestTemperatureCelsius;
    int latestFanDuty {0};
    std::string dataStatusText {"Not loaded"};
    std::string operationStatusText;
    UiError error;
};

struct FanSettingsCommand {
    bool temperatureAutoControl {true};
    double targetCelsius {37.0};
    int manualFanDuty {0};
    int maxFanDuty {255};
};

} // namespace mlcp::hmi::bridge
