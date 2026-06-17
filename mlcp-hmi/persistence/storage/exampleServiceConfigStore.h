#pragma once

#include <string>

#include "service/example/exampleService.h"

namespace mlcp::hmi::persistence::storage {

class ExampleServiceConfigStore {
public:
    explicit ExampleServiceConfigStore(std::string configPath = defaultConfigPath());

    mlcp::hmi::service::example::ExampleServiceConfig load(
        const mlcp::hmi::service::example::ExampleServiceConfig& defaultConfig) const;
    void save(const mlcp::hmi::service::example::ExampleServiceConfig& config) const;

    const std::string& configPath() const;

    static std::string defaultConfigPath();

private:
    std::string configPath_;
};

} // namespace mlcp::hmi::persistence::storage
