#include "xsl/def.h"
#include "xsl/logctl.h"

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/Utility.h>
#include <quill/sinks/ConsoleSink.h>

#include <cstdlib>
XSL_NB
#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL
LogCtl LogCtl::instance{};
LogCtl::LogCtl() : logger(nullptr) {
  quill::Backend::start();
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  logger = quill::Frontend::create_or_get_logger(
      "root", std::move(console_sink),
      "[%(time)][%(thread_id)] %(short_source_location:<28) %(log_level:<8) "
      "%(message)",
      "%H:%M:%S.%Qus");
  auto env = std::getenv("CPP_LOG");
  if (env) {
    instance.logger->set_log_level(quill::loglevel_from_string(std::string(env)));
  } else {
    set_log_level(LogLevel::LOG8);
  }
}
LogCtl::~LogCtl() {
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();
}
#else
LogCtl::LogCtl() : logger(nullptr) {}
LogCtl::~LogCtl() {}
#endif
XSL_NE
