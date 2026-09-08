/**
 * @file log.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/Utility.h>
#include <quill/sinks/ConsoleSink.h>
#include <xsl/log.h>

#include <cstdlib>
#include <ranges>
XSL_NB

LogCtl logger_xsl("xsl");

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL
LogCtl::LogCtl(std::string const& logger_name, std::string const& sink_name) : logger(nullptr) {
  quill::Backend::start();
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(sink_name);
  logger = quill::Frontend::create_or_get_logger(
      logger_name, std::move(console_sink),
      quill::PatternFormatterOptions{
          "[%(time)][%(thread_id)] %(short_source_location:<20) %(log_level:<8) "
          "%(message)",
          "%H:%M:%S.%Qus"});
  auto env = std::getenv("CPP_LOG");
  if (!env) {
    set_log_level(LogLevel::Error);
    return;
  }
  auto env_view = std::string_view{env};
  std::string env_str = "error";
  for (const auto& e : std::views::split(env_view, ',')) {
    auto pos = std::find(e.begin(), e.end(), '=');
    if (pos == e.end()) {
      env_str = std::ranges::to<std::string>(e);
      continue;
    }
    auto key = std::string_view(&*e.begin(), pos - e.begin());
    if (key == logger_name) {
      env_str = std::ranges::to<std::string>(std::ranges::subrange(++pos, e.end()));
      break;
    }
  }
  if (env_str == "trace") {
    env_str += "l1";
  }
  logger->set_log_level(quill::loglevel_from_string(env_str));
  LOG_INFO(logger, "{} Log level: {}", logger_name, log_level_to_string(get_log_level()));
}
LogCtl::~LogCtl() {
  if (logger) {
    quill::Frontend::remove_logger(logger);
  }
}
#else
LogCtl::LogCtl(std::string const&, std::string const&) : logger(nullptr) {}
LogCtl::~LogCtl() {}
#endif
XSL_NE
