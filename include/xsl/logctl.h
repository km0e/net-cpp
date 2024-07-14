#pragma once
#ifndef XSL_LOGCTL
#  define XSL_LOGCTL
#  include "xsl/def.h"

#  include <quill/LogMacros.h>
#  include <quill/Logger.h>
XSL_NB

enum class LogLevel { NONE, TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL };

class LogCtl {
private:
  LogCtl();
  LogCtl(const LogCtl&) = delete;
  LogCtl& operator=(const LogCtl&) = delete;

public:
  ~LogCtl();
  quill::Logger* logger;
  static LogCtl instance;

  static constexpr void no_log() { set_log_level(LogLevel::NONE); }
  static constexpr void set_log_level(LogLevel level) {
    switch (level) {
      case LogLevel::TRACE:
        instance.logger->set_log_level(quill::LogLevel::TraceL3);
        break;
      case LogLevel::DEBUG:
        instance.logger->set_log_level(quill::LogLevel::Debug);
        break;
      case LogLevel::INFO:
        instance.logger->set_log_level(quill::LogLevel::Info);
        break;
      case LogLevel::WARNING:
        instance.logger->set_log_level(quill::LogLevel::Warning);
        break;
      case LogLevel::ERROR:
        instance.logger->set_log_level(quill::LogLevel::Error);
        break;
      case LogLevel::CRITICAL:
        instance.logger->set_log_level(quill::LogLevel::Critical);
        break;
      case LogLevel::NONE:
        instance.logger->set_log_level(quill::LogLevel::None);
        break;
    }
  }
};

#  define TRACE(fmt, ...) LOG_TRACE_L1(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define DEBUG(fmt, ...) LOG_DEBUG(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define INFO(fmt, ...) LOG_INFO(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define WARNING(fmt, ...) LOG_WARNING(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define ERROR(fmt, ...) LOG_ERROR(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define CRITICAL(fmt, ...) LOG_CRITICAL(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

constexpr void set_log_level(LogLevel level) { xsl::LogCtl::set_log_level(level); }

constexpr void no_log() { set_log_level(xsl::LogLevel::NONE); }

inline void flush_log() { xsl::LogCtl::instance.logger->flush_log(); }

XSL_NE
#endif
