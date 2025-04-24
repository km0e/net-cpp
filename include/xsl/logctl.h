/**
 * @file logctl.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_LOGCTL
#  define XSL_LOGCTL
#  include "xsl/def.h"

#  include <quill/LogMacros.h>
#  include <quill/Logger.h>
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

class LogCtl {
private:
  LogCtl();
  constexpr LogCtl(const LogCtl&) = delete;
  constexpr LogCtl& operator=(const LogCtl&) = delete;

public:
  ~LogCtl();
  quill::Logger* logger;
#  if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL
  static LogCtl instance;

  static constexpr void no_log() { set_log_level(LogLevel::None); }

  static void flush_log() { instance.logger->flush_log(); }

  static constexpr void set_log_level(LogLevel level) {
    switch (level) {
      case LogLevel::None:
        instance.logger->set_log_level(quill::LogLevel::None);
        break;
      case LogLevel::Error:
        instance.logger->set_log_level(quill::LogLevel::Error);
        break;
      case LogLevel::Warning:
        instance.logger->set_log_level(quill::LogLevel::Warning);
        break;
      case LogLevel::Info:
        instance.logger->set_log_level(quill::LogLevel::Info);
        break;
      case LogLevel::Debug:
        instance.logger->set_log_level(quill::LogLevel::Debug);
        break;
      case LogLevel::Trace:
        instance.logger->set_log_level(quill::LogLevel::TraceL1);
        break;
    }
  }

  static constexpr LogLevel get_log_level() {
    switch (instance.logger->get_log_level()) {
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

  static constexpr std::string_view log_level_to_string(LogLevel level) {
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
#  else
  static constexpr void no_log() {}

  static constexpr void flush_log() {}

  static constexpr void set_log_level(LogLevel) {}
#  endif
};

#  define log_trace(fmt, ...) LOG_TRACE_L1(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define log_debug(fmt, ...) LOG_DEBUG(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define log_info(fmt, ...) LOG_INFO(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define log_warning(fmt, ...) LOG_WARNING(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define log_error(fmt, ...) LOG_ERROR(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define log_critical(fmt, ...) LOG_CRITICAL(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

constexpr void set_log_level(LogLevel level) { xsl::LogCtl::set_log_level(level); }

constexpr void no_log() { set_log_level(xsl::LogLevel::None); }

inline void flush_log() { xsl::LogCtl::flush_log(); }

XSL_NE
#endif
