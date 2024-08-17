#pragma once
#ifndef XSL_CORO
#  define XSL_CORO
#  include "xsl/coro/await.h"
#  include "xsl/coro/executor.h"
#  include "xsl/coro/lazy.h"
#  include "xsl/coro/semaphore.h"
#  include "xsl/coro/task.h"
#  include "xsl/def.h"
XSL_NB
namespace coro {
  using _coro::CountingSemaphore;
  using _coro::ExecutorBase;
  using _coro::GetExecutor;
  using _coro::NewThreadExecutor;
  using _coro::NoopExecutor;
}  // namespace coro
using _coro::Lazy;
using _coro::Task;
XSL_NE
#endif
