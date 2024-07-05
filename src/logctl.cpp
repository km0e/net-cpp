#include "xsl/def.h"
#include "xsl/logctl.h"

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
XSL_NAMESPACE_BEGIN
LogCtl LogCtl::instance{};
LogCtl::LogCtl() : logger(nullptr) {
  quill::Backend::start();
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  logger = quill::Frontend::create_or_get_logger(
      "root", std::move(console_sink),
      "[%(thread_id)] %(short_source_location:<28) %(log_level:<6) "
      "%(message)");
}
LogCtl::~LogCtl() {
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();
}

XSL_NAMESPACE_END
