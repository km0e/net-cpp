#pragma once
#ifndef XSL_LOGCTL
#  define XSL_LOGCTL
#  include "xsl/def.h"

#  include <quill/LogMacros.h>
#  include <quill/Logger.h>
XSL_NB

enum class LogLevel { NONE, LOG1, LOG2, LOG3, LOG4, LOG5, LOG6, LOG7, LOG8 };

class LogCtl {
private:
  LogCtl();
  LogCtl(const LogCtl&) = delete;
  LogCtl& operator=(const LogCtl&) = delete;

public:
  ~LogCtl();
  quill::Logger* logger;
#  if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_CRITICAL
  static LogCtl instance;

  static constexpr void no_log() { set_log_level(LogLevel::NONE); }

  static inline void flush_log() { instance.logger->flush_log(); }

  static constexpr void set_log_level(LogLevel level) {
    switch (level) {
      case LogLevel::NONE:
        instance.logger->set_log_level(quill::LogLevel::None);
        break;
      case LogLevel::LOG1:
        instance.logger->set_log_level(quill::LogLevel::Critical);
        break;
      case LogLevel::LOG2:
        instance.logger->set_log_level(quill::LogLevel::Error);
        break;
      case LogLevel::LOG3:
        instance.logger->set_log_level(quill::LogLevel::Warning);
        break;
      case LogLevel::LOG4:
        instance.logger->set_log_level(quill::LogLevel::Info);
        break;
      case LogLevel::LOG5:
        instance.logger->set_log_level(quill::LogLevel::Debug);
        break;
      case LogLevel::LOG6:
        instance.logger->set_log_level(quill::LogLevel::TraceL1);
        break;
      case LogLevel::LOG7:
        instance.logger->set_log_level(quill::LogLevel::TraceL2);
        break;
      case LogLevel::LOG8:
        instance.logger->set_log_level(quill::LogLevel::TraceL3);
        break;
    }
  }
#  else
  static constexpr void no_log() {}

  static constexpr void flush_log() {}

  static constexpr void set_log_level(LogLevel) {}
#  endif
};

#  define LOG1(fmt, ...) LOG_CRITICAL(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define LOG2(fmt, ...) LOG_ERROR(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define LOG3(fmt, ...) LOG_WARNING(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define LOG4(fmt, ...) LOG_INFO(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define LOG5(fmt, ...) LOG_DEBUG(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define LOG6(fmt, ...) LOG_TRACE_L1(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define LOG7(fmt, ...) LOG_TRACE_L2(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define LOG8(fmt, ...) LOG_TRACE_L3(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define TRACE(fmt, ...) LOG_TRACE_L1(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define DEBUG(fmt, ...) LOG_DEBUG(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define INFO(fmt, ...) LOG_INFO(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define WARN(fmt, ...) LOG_WARNING(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define ERROR(fmt, ...) LOG_ERROR(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

#  define CRITICAL(fmt, ...) LOG_CRITICAL(xsl::LogCtl::instance.logger, fmt, ##__VA_ARGS__)

constexpr void set_log_level(LogLevel level) { xsl::LogCtl::set_log_level(level); }

constexpr void no_log() { set_log_level(xsl::LogLevel::NONE); }

inline void flush_log() { xsl::LogCtl::flush_log(); }

XSL_NE
#endif
