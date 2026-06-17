#include "service/log/logger.h"

#include <memory>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace mlcp::common::log {

namespace {

constexpr char kLoggerName[] = "mlcp";
constexpr char kLoggerPattern[] = "%Y-%m-%d %H:%M:%S.%e [%^%l%$] [%n] [tid=%t] %v";
constexpr std::size_t kMaxFileSize = 1024U * 1024U * 5U;
constexpr std::size_t kMaxFileCount = 3U;

void applyDefaultLoggerConfig(const std::shared_ptr<spdlog::logger>& logger)
{
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::err);
    logger->set_pattern(kLoggerPattern);

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::err);
}

void replaceDefaultLogger(const std::shared_ptr<spdlog::logger>& logger)
{
    spdlog::drop(kLoggerName);
    spdlog::register_logger(logger);
    applyDefaultLoggerConfig(logger);
}

} // namespace

void initConsoleLogger()
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    auto logger = std::make_shared<spdlog::logger>(kLoggerName, sinks.begin(), sinks.end());
    replaceDefaultLogger(logger);
}

void initConsoleAndFileLogger(const std::string& logFilePath)
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logFilePath,
        kMaxFileSize,
        kMaxFileCount));

    auto logger = std::make_shared<spdlog::logger>(kLoggerName, sinks.begin(), sinks.end());
    replaceDefaultLogger(logger);
}

} // namespace mlcp::common::log
