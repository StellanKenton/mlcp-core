#pragma once

#include <exception>

#include "bridge/fanSettingsUiState.h"
#include "service/temp/tempService.h"

namespace mlcp::hmi::bridge {

FanSettingsUiState toFanSettingsUiState(
    const mlcp::hmi::service::temp::TempServiceSnapshot& snapshot);

UiError toFanSettingsUiError(const std::exception& error);
UiError toUnknownUiError();

} // namespace mlcp::hmi::bridge
