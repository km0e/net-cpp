/**
 * @file log.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_LOGCTL
#  define XSL_LOGCTL

#  include <quill/LogMacros.h>
#  include <quill/Logger.h>
#  include <xsl/def.h>
XSL_NB

enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  None,
};

consteval LogLevel compile_active_log_level() {
#  if QUILL_COMPILE_ACTIVE_LOG_LEVEL == QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1
  return LogLevel::Trace;
#  elif QUILL_COMPILE_ACTIVE_LOG_LEVEL == QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG
  return LogLevel::Debug;
#  elif QUILL_COMPILE_ACTIVE_LOG_LEVEL == QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO
  return LogLevel::Info;
#  elif QUILL_COMPILE_ACTIVE_LOG_LEVEL == QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING
  return LogLevel::Warning;
#  elif QUILL_COMPILE_ACTIVE_LOG_LEVEL == QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR
  return LogLevel::Error;
#  else
  return LogLevel::None;
#  endif
}

constexpr std::string_view log_level_to_string(LogLevel level) {
  switch (level) {
    case LogLevel::None:
      return "none";
    case LogLevel::Error:
      return "error";
    case LogLevel::Warning:
      return "warning";
    case LogLevel::Info:
      return "info";
    case LogLevel::Debug:
      return "debug";
    case LogLevel::Trace:
      return "trace";
    default:
      return "none";
  }
}
#  define log_trace(fmt, ...) LOG_TRACE_L1(xsl::logger_xsl.logger, fmt, ##__VA_ARGS__)

#  define log_debug(fmt, ...) LOG_DEBUG(xsl::logger_xsl.logger, fmt, ##__VA_ARGS__)

#  define log_info(fmt, ...) LOG_INFO(xsl::logger_xsl.logger, fmt, ##__VA_ARGS__)

#  define log_warning(fmt, ...) LOG_WARNING(xsl::logger_xsl.logger, fmt, ##__VA_ARGS__)

#  define log_error(fmt, ...) LOG_ERROR(xsl::logger_xsl.logger, fmt, ##__VA_ARGS__)

#  define log_critical(fmt, ...) LOG_CRITICAL(xsl::logger_xsl.logger, fmt, ##__VA_ARGS__)

class LogCtl {
private:
  constexpr LogCtl(const LogCtl&) = delete;
  constexpr LogCtl& operator=(const LogCtl&) = delete;

public:
  quill::Logger* logger;

  LogCtl(std::string const& logger_name, std::string const& sink_name = "console");
  ~LogCtl();
#  if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL

  void flush_log() { logger->flush_log(); }

  constexpr void set_log_level(LogLevel level) {
    switch (level) {
      case LogLevel::None:
        logger->set_log_level(quill::LogLevel::None);
        break;
      case LogLevel::Error:
        logger->set_log_level(quill::LogLevel::Error);
        break;
      case LogLevel::Warning:
        logger->set_log_level(quill::LogLevel::Warning);
        break;
      case LogLevel::Info:
        logger->set_log_level(quill::LogLevel::Info);
        break;
      case LogLevel::Debug:
        logger->set_log_level(quill::LogLevel::Debug);
        break;
      case LogLevel::Trace:
        logger->set_log_level(quill::LogLevel::TraceL1);
        break;
    }
  }

  constexpr LogLevel get_log_level() {
    switch (logger->get_log_level()) {
      case quill::LogLevel::None:
        return LogLevel::None;
      case quill::LogLevel::Error:
        return LogLevel::Error;
      case quill::LogLevel::Warning:
        return LogLevel::Warning;
      case quill::LogLevel::Info:
        return LogLevel::Info;
      case quill::LogLevel::Debug:
        return LogLevel::Debug;
      case quill::LogLevel::TraceL1:
        return LogLevel::Trace;
      default:
        return LogLevel::None;
    }
  }

#  else
  static constexpr void no_log() {}

  static constexpr void flush_log() {}

  static constexpr void set_log_level(LogLevel) {}
#  endif
};

extern LogCtl logger_xsl;

XSL_NE
#  include <quill/DeferredFormatCodec.h>

#  define LOG_FMT_FOR_IMPL_TO_STRING_VIEW(type)                               \
    template <>                                                               \
    struct fmtquill::formatter<type> {                                        \
      constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); } \
      auto format(type const& user, format_context& ctx) const {              \
        return fmtquill::format_to(ctx.out(), "{}", user.to_string_view());   \
      }                                                                       \
    };                                                                        \
    template <>                                                               \
    struct quill::Codec<type> : quill::DeferredFormatCodec<type> {};

#endif
