#include "xsl/def.h"
#include "xsl/logctl.h"

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
XSL_NB
#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_CRITICAL
LogCtl LogCtl::instance{};
#endif
LogCtl::LogCtl() : logger(nullptr) {
  quill::Backend::start();
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  logger = quill::Frontend::create_or_get_logger(
      "root", std::move(console_sink),
      "[%(time)][%(thread_id)] %(short_source_location:<28) %(log_level:<6) "
      "%(message)",
      "%H:%M:%S.%Qus");
  set_log_level(LogLevel::LOG7);
}
LogCtl::~LogCtl() {
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();
}

XSL_NE
