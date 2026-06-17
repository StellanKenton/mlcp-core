#include "persistence/storage/tempServiceConfigStore.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

namespace fs = std::filesystem;

using mlcp::hmi::persistence::storage::TempServiceConfigStore;
using mlcp::hmi::service::temp::TempServiceConfig;

void testSavesAndLoadsFanConfig()
{
    const fs::path tempDirectory = fs::temp_directory_path() / "mlcp_temp_config_store_test";
    fs::remove_all(tempDirectory);

    const TempServiceConfigStore store((tempDirectory / "temp_service.conf").string());
    TempServiceConfig config;
    config.temperatureAutoControl = false;
    config.targetCelsius = 38.5;
    config.manualFanDuty = 90;
    config.maxFanDuty = 210;
    config.controlInterval = std::chrono::milliseconds(250);

    store.save(config);

    const TempServiceConfig loaded = store.load(TempServiceConfig {});
    assert(!loaded.temperatureAutoControl);
    assert(loaded.targetCelsius == 38.5);
    assert(loaded.manualFanDuty == 90);
    assert(loaded.maxFanDuty == 210);
    assert(loaded.controlInterval == std::chrono::milliseconds(250));

    fs::remove_all(tempDirectory);
}

void testMissingFileUsesDefaultConfig()
{
    const fs::path tempDirectory = fs::temp_directory_path() / "mlcp_temp_config_missing_test";
    fs::remove_all(tempDirectory);

    const TempServiceConfigStore store((tempDirectory / "temp_service.conf").string());
    TempServiceConfig defaultConfig;
    defaultConfig.maxFanDuty = 210;

    const TempServiceConfig loaded = store.load(defaultConfig);
    assert(loaded.maxFanDuty == 210);

    fs::remove_all(tempDirectory);
}

} // namespace

int main()
{
    testSavesAndLoadsFanConfig();
    testMissingFileUsesDefaultConfig();

    return 0;
}
