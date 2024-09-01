/**
 * @file executor.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Executor for coroutines
 * @version 0.11
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_EXECUTOR
#  define XSL_CORO_EXECUTOR

#  include "xsl/coro/def.h"
#  include "xsl/def.h"

XSL_CORO_NB

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
  void schedule(move_only_function<void()> &&func) override;
};

class NewThreadExecutor : public ExecutorBase {
public:
  void schedule(move_only_function<void()> &&func) override;
};

XSL_CORO_NE

#endif  // XSL_CORO_EXECUTOR
