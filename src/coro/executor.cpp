#include "xsl/coro/def.h"
#include "xsl/logctl.h"
#include "xsl/coro/executor.h"

#include <thread>
XSL_CORO_NB
void NoopExecutor::schedule(move_only_function<void()> &&func) { func(); }

void NewThreadExecutor::schedule(move_only_function<void()> &&func) {
  LOG5("new task scheduled");
  std::thread(std::move(func)).detach();
}

XSL_CORO_NE
