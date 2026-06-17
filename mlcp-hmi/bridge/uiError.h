#pragma once

#include <string>

namespace mlcp::hmi::bridge {

enum class UiErrorCode {
    none = 0,
    tempServiceUnavailable,
    invalidTempTarget,
    invalidFanDuty,
    invalidConfig,
    backendFailure,
};

struct UiError {
    UiErrorCode code {UiErrorCode::none};
    std::string technicalMessage;
    std::string userMessage;
};

inline UiError noUiError()
{
    return {};
}

inline bool hasUiError(const UiError& error)
{
    return error.code != UiErrorCode::none;
}

} // namespace mlcp::hmi::bridge
