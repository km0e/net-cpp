/**
 * @file executor.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Executor for coroutines
 * @version 0.1.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_CORO_EXECUTOR
#  define XSL_CORO_EXECUTOR

#  include <xsl/coro/def.h>
#  include <xsl/def.h>

#  include <condition_variable>
#  include <mutex>
#  include <queue>
#  include <thread>
#  include <vector>

XSL_CORO_NB
template <class T>
concept Executor = requires(T t, move_only_function<void()> func) {
  { t.schedule(std::move(func)) };
};
struct ExecutorBase {
  virtual ~ExecutorBase() = default;
  virtual void schedule(move_only_function<void()> &&func) = 0;
};

class NoopExecutor : public ExecutorBase {
public:
  void schedule(move_only_function<void()> &&func);
};

class NewThreadExecutor : public ExecutorBase {
public:
  void schedule(move_only_function<void()> &&func);
};

class ThreadPoolExecutor : public ExecutorBase {
public:
  explicit ThreadPoolExecutor(size_t n = std::thread::hardware_concurrency());
  ~ThreadPoolExecutor();
  void schedule(move_only_function<void()> &&func) override;

private:
  std::vector<std::thread> _workers;
  std::queue<move_only_function<void()>> _tasks;
  std::mutex _mtx;
  std::condition_variable _cv;
  bool _stop = false;
};

XSL_CORO_NE

#endif  // XSL_CORO_EXECUTOR
