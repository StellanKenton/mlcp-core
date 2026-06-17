#include "persistence/storage/tempServiceConfigStore.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace fs = std::filesystem;

namespace mlcp::hmi::persistence::storage {
namespace {

std::string trim(const std::string& text)
{
    const std::string whitespace = " \t\r\n";
    const std::size_t begin = text.find_first_not_of(whitespace);
    if (begin == std::string::npos) {
        return "";
    }

    const std::size_t end = text.find_last_not_of(whitespace);
    return text.substr(begin, end - begin + 1U);
}

std::map<std::string, std::string> readKeyValues(const fs::path& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open config for read: " + path.string() +
                                 ": " + std::strerror(errno));
    }

    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmedLine = trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }

        const std::size_t separator = trimmedLine.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("invalid config line: " + trimmedLine);
        }

        values[trim(trimmedLine.substr(0U, separator))] =
            trim(trimmedLine.substr(separator + 1U));
    }

    return values;
}

bool readBool(const std::map<std::string, std::string>& values,
              const std::string& key,
              bool fallback)
{
    const auto iterator = values.find(key);
    if (iterator == values.end()) {
        return fallback;
    }

    if (iterator->second == "true" || iterator->second == "1") {
        return true;
    }
    if (iterator->second == "false" || iterator->second == "0") {
        return false;
    }

    throw std::runtime_error("invalid bool config value for " + key + ": " + iterator->second);
}

double readDouble(const std::map<std::string, std::string>& values,
                  const std::string& key,
                  double fallback)
{
    const auto iterator = values.find(key);
    if (iterator == values.end()) {
        return fallback;
    }

    try {
        std::size_t parsedSize = 0U;
        const double result = std::stod(iterator->second, &parsedSize);
        if (parsedSize != iterator->second.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid double config value for " + key + ": " +
                                 iterator->second);
    }
}

int readInt(const std::map<std::string, std::string>& values,
            const std::string& key,
            int fallback)
{
    const auto iterator = values.find(key);
    if (iterator == values.end()) {
        return fallback;
    }

    try {
        std::size_t parsedSize = 0U;
        const int result = std::stoi(iterator->second, &parsedSize);
        if (parsedSize != iterator->second.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid integer config value for " + key + ": " +
                                 iterator->second);
    }
}

} // namespace

TempServiceConfigStore::TempServiceConfigStore(std::string configPath)
    : configPath_(std::move(configPath))
{
    if (configPath_.empty()) {
        throw std::invalid_argument("temp service config path is required");
    }
}

mlcp::hmi::service::temp::TempServiceConfig TempServiceConfigStore::load(
    const mlcp::hmi::service::temp::TempServiceConfig& defaultConfig) const
{
    const fs::path path = configPath_;
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return defaultConfig;
    }

    const std::map<std::string, std::string> values = readKeyValues(path);
    auto config = defaultConfig;
    config.temperatureAutoControl =
        readBool(values, "temperatureAutoControl", config.temperatureAutoControl);
    config.targetCelsius = readDouble(values, "targetCelsius", config.targetCelsius);
    config.toleranceCelsius = readDouble(values, "toleranceCelsius", config.toleranceCelsius);
    config.proportionalGain = readDouble(values, "proportionalGain", config.proportionalGain);
    config.manualFanDuty = readInt(values, "manualFanDuty", config.manualFanDuty);
    config.minFanDuty = readInt(values, "minFanDuty", config.minFanDuty);
    config.maxFanDuty = readInt(values, "maxFanDuty", config.maxFanDuty);
    config.controlInterval =
        std::chrono::milliseconds(readInt(values, "controlIntervalMs",
                                          static_cast<int>(config.controlInterval.count())));
    return config;
}

void TempServiceConfigStore::save(
    const mlcp::hmi::service::temp::TempServiceConfig& config) const
{
    const fs::path path = configPath_;
    const fs::path directory = path.parent_path();
    if (!directory.empty()) {
        fs::create_directories(directory);
    }

    const fs::path temporaryPath = path.string() + ".tmp";
    {
        std::ofstream output(temporaryPath);
        if (!output) {
            throw std::runtime_error("failed to open config for write: " +
                                     temporaryPath.string() + ": " + std::strerror(errno));
        }

        output << "temperatureAutoControl="
               << (config.temperatureAutoControl ? "true" : "false") << '\n';
        output << "targetCelsius=" << config.targetCelsius << '\n';
        output << "toleranceCelsius=" << config.toleranceCelsius << '\n';
        output << "proportionalGain=" << config.proportionalGain << '\n';
        output << "manualFanDuty=" << config.manualFanDuty << '\n';
        output << "minFanDuty=" << config.minFanDuty << '\n';
        output << "maxFanDuty=" << config.maxFanDuty << '\n';
        output << "controlIntervalMs=" << config.controlInterval.count() << '\n';
        if (!output) {
            throw std::runtime_error("failed to write config: " + temporaryPath.string());
        }
    }

    fs::rename(temporaryPath, path);
}

const std::string& TempServiceConfigStore::configPath() const
{
    return configPath_;
}

std::string TempServiceConfigStore::defaultConfigPath()
{
    if (const char* configDirectory = std::getenv("MLCP_HMI_CONFIG_DIR")) {
        return (fs::path(configDirectory) / "temp_service.conf").string();
    }

    if (const char* homeDirectory = std::getenv("HOME")) {
        return (fs::path(homeDirectory) / ".config" / "mlcp-hmi" /
                "temp_service.conf").string();
    }

    return (fs::temp_directory_path() / "mlcp-hmi" / "temp_service.conf").string();
}

} // namespace mlcp::hmi::persistence::storage
