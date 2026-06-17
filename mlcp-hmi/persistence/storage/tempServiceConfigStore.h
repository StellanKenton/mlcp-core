#pragma once

#include <string>

#include "service/temp/tempService.h"

namespace mlcp::hmi::persistence::storage {

class TempServiceConfigStore {
public:
    explicit TempServiceConfigStore(std::string configPath = defaultConfigPath());

    mlcp::hmi::service::temp::TempServiceConfig load(
        const mlcp::hmi::service::temp::TempServiceConfig& defaultConfig) const;
    void save(const mlcp::hmi::service::temp::TempServiceConfig& config) const;

    const std::string& configPath() const;

    static std::string defaultConfigPath();

private:
    std::string configPath_;
};

} // namespace mlcp::hmi::persistence::storage
