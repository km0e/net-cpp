#pragma once
#ifndef XSL_CORO_EXECUTOR
#  define XSL_CORO_EXECUTOR

#  include "xsl/coro/def.h"

#  include <functional>
XSL_CORO_NB

#  ifndef __cpp_lib_move_only_function
template <class F>
using move_only_function = std::function<F>;
#  else
using std::move_only_function;
#  endif  // __cpp_lib_move_only_function

class ExecutorBase {
public:
  virtual ~ExecutorBase() = default;
  virtual void schedule(move_only_function<void()> &&func) = 0;
};

template <class T>
concept Executor = requires(T t, move_only_function<void()> func) {
  { t.schedule(std::move(func)) };
};

class NoopExecutor : public ExecutorBase {
public:
  void schedule(move_only_function<void()> &&func);
};

class NewThreadExecutor : public ExecutorBase {
public:
  void schedule(move_only_function<void()> &&func);
};

XSL_CORO_NE

#endif  // XSL_CORO_EXECUTOR
