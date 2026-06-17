#pragma once

#include "bridge/lifecycleUiState.h"
#include "service/lifecycle/lifecycle.h"

namespace mlcp::hmi::bridge {

LifecycleUiState toLifecycleUiState(
    const mlcp::hmi::service::LifecycleSnapshot& snapshot);

} // namespace mlcp::hmi::bridge
