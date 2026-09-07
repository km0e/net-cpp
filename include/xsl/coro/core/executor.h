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
#  include <memory>
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

/// @brief shared instance: NoopExecutor is stateless, one per program is enough
inline const std::shared_ptr<ExecutorBase> &noop_executor() noexcept {
  static const std::shared_ptr<ExecutorBase> instance = std::make_shared<NoopExecutor>();
  return instance;
}

class NewThreadExecutor : public ExecutorBase {
public:
  void schedule(move_only_function<void()> &&func);
};

/**
 * @brief Thread pool executor — a HANDLE to a shared pool state
 *
 * The pool state outlives this handle (workers hold their own reference):
 * destroying the handle only signals stop and never joins, so it is safe to
 * drop the last handle (or the last shared_ptr<ExecutorBase> reference held
 * by a completing coroutine) from ANY thread — including a pool worker.
 * Without this split, a coroutine finishing on a worker could drop the last
 * executor reference and run ~ThreadPoolExecutor on that worker, where
 * join()-ing itself throws EDEADLK and terminates the process.
 *
 * The handle is move-only: copies would make the stopping destructor
 * ambiguous (destroying one copy must not stop the pool others still use);
 * sharing goes through shared_ptr<ExecutorBase> instead. Remaining tasks are
 * still drained (workers exit only when stop is set AND the queue is empty),
 * just asynchronously with respect to the handle's destruction.
 */
class ThreadPoolExecutor : public ExecutorBase {
public:
  explicit ThreadPoolExecutor(size_t n = std::thread::hardware_concurrency());
  ThreadPoolExecutor(ThreadPoolExecutor &&) noexcept = default;
  ThreadPoolExecutor &operator=(ThreadPoolExecutor &&ano) noexcept {
    if (this != &ano) {
      _signal_stop();  // the outgoing pool must not leak (its handle is gone)
      _state = std::move(ano._state);
    }
    return *this;
  }
  ThreadPoolExecutor(const ThreadPoolExecutor &) = delete;
  ThreadPoolExecutor &operator=(const ThreadPoolExecutor &) = delete;
  ~ThreadPoolExecutor();
  void schedule(move_only_function<void()> &&func) override;

private:
  struct State;
  std::shared_ptr<State> _state;
  void _signal_stop() noexcept;
};

XSL_CORO_NE

#endif  // XSL_CORO_EXECUTOR
