#pragma once
#ifndef XSL_CORO_EXECUTOR
#  define XSL_CORO_EXECUTOR

#  include "xsl/coro/def.h"

#  include <functional>
XSL_CORO_NB

class ExecutorBase {
public:
  virtual ~ExecutorBase() = default;
  virtual void schedule(std::move_only_function<void()> &&func) = 0;
};

template <class T>
concept Executor = requires(T t, std::move_only_function<void()> func) {
  { t.schedule(std::move(func)) };
};

class NoopExecutor : public ExecutorBase {
public:
  void schedule(std::move_only_function<void()> &&func);
};

class NewThreadExecutor : public ExecutorBase {
public:
  void schedule(std::move_only_function<void()> &&func);
};

XSL_CORO_NE

#endif  // XSL_CORO_EXECUTOR
