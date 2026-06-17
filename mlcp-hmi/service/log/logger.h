#pragma once

#include <string>

#include <spdlog/spdlog.h>

namespace mlcp::common::log {

void initConsoleLogger();
void initConsoleAndFileLogger(const std::string& logFilePath);

} // namespace mlcp::common::log

#define LOG_T(...) ::spdlog::trace(__VA_ARGS__)
#define LOG_D(...) ::spdlog::debug(__VA_ARGS__)
#define LOG_I(...) ::spdlog::info(__VA_ARGS__)
#define LOG_W(...) ::spdlog::warn(__VA_ARGS__)
#define LOG_E(...) ::spdlog::error(__VA_ARGS__)
#define LOG_C(...) ::spdlog::critical(__VA_ARGS__)
