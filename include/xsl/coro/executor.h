#pragma once
#ifndef XSL_CORO_EXECUTOR
#  define XSL_CORO_EXECUTOR

#  include "xsl/coro/def.h"

#  include <functional>
XSL_CORO_NB

template <class T>
concept Executor = requires(T t, std::move_only_function<void()> func) {
  { t.schedule(std::move(func)) };
};

class NoopExecutor {
public:
  void schedule(std::move_only_function<void()> &&func);
};

class NewThreadExecutor {
public:
  void schedule(std::move_only_function<void()> &&func);
};

XSL_CORO_NE

#endif  // XSL_CORO_EXECUTOR
