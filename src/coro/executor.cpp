#include "xsl/coro/def.h"
#include "xsl/coro/executor.h"

#include <thread>
XSL_CORO_NAMESPACE_BEGIN
void NoopExecutor::schedule(std::move_only_function<void()> &&func) { func(); }

void NewThreadExecutor::schedule(std::move_only_function<void()> &&func) {
  std::thread(std::move(func)).detach();
}

XSL_CORO_NAMESPACE_END