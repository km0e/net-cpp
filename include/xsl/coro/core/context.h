/**
 * @file context.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Coroutine context
 * @version 0.2.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_CORO_CONTEXT
#  define XSL_CORO_CONTEXT
#  include <xsl/coro/core/executor.h>
#  include <xsl/coro/def.h>
#  include <xsl/coro/log.h>

#  include <cassert>
#  include <memory>
#  include <stop_token>

XSL_CORO_NB

/// @brief Coroutine context: executor + reserved object + cancellation domain
///
/// Cancellation is a std::stop_source: copies of a CoroContext share the stop
/// state (atomic refcount inside), so all chains detached with copies of one
/// context form ONE cancellation domain with multi-callback support.
class CoroContext {
  std::shared_ptr<ExecutorBase> _e = noop_executor();  // shared stateless instance
  std::shared_ptr<void> _reserved;
  std::stop_source _stop;

  struct share_t {
    explicit share_t() = default;
  };
  CoroContext(share_t, std::shared_ptr<ExecutorBase> e, std::shared_ptr<void> reserved,
              std::stop_source stop)
      : _e(std::move(e)), _reserved(std::move(reserved)), _stop(std::move(stop)) {}

public:
  CoroContext() = default;
  template <Executor E>
  CoroContext(E&& executor, void* reserved = nullptr, void (*deleter)(void*) = nullptr)
      : _e(std::make_shared<E>(std::forward<E>(executor)))
      , _reserved(reserved, deleter ? deleter : [](void*) {})
      , _stop() {}
  /// @brief construct from a shared executor — the entry point for executors
  ///        that are not move-constructible (e.g. ThreadPoolExecutor, which
  ///        holds a mutex)
  CoroContext(std::shared_ptr<ExecutorBase> executor, void* reserved = nullptr,
              void (*deleter)(void*) = nullptr)
      : _e(std::move(executor))
      , _reserved(reserved, deleter ? deleter : [](void*) {})
      , _stop() {}
  /// @brief bind this context to an EXISTING stop source (e.g. a poller's):
  ///        requesting stop on that source cancels every chain in this domain
  template <Executor E>
  CoroContext(std::stop_source stop, E&& executor, void* reserved = nullptr,
              void (*deleter)(void*) = nullptr)
      : _e(std::make_shared<E>(std::forward<E>(executor)))
      , _reserved(reserved, deleter ? deleter : [](void*) {})
      , _stop(std::move(stop)) {}
  /// @brief stop-source + shared-executor overload (see the notes above)
  CoroContext(std::stop_source stop, std::shared_ptr<ExecutorBase> executor,
              void* reserved = nullptr, void (*deleter)(void*) = nullptr)
      : _e(std::move(executor))
      , _reserved(reserved, deleter ? deleter : [](void*) {})
      , _stop(std::move(stop)) {}
  ~CoroContext() = default;

  void dispatch(move_only_function<void()>&& func) {
    co_trace("Context dispatching");
    _e->schedule(std::move(func));
  }
  /// @brief child context sharing executor, reserved object AND cancellation
  ///        domain — co_yield fan-out uses this, so cancelling the parent
  ///        also cancels fanned-out children (e.g. per-connection tasks)
  CoroContext new_child_context() { return CoroContext(share_t{}, _e, _reserved, _stop); }
  /// @brief child context with an INDEPENDENT cancellation domain
  CoroContext new_independent_context() { return CoroContext(share_t{}, _e, _reserved, {}); }

  constexpr auto get_reserved() { return _reserved.get(); }

  /// @brief request cancellation of every chain in this domain: all
  ///        stop_callbacks registered by cancellable awaits fire
  ///        synchronously on the calling thread
  void cancel() { _stop.request_stop(); }
  bool stop_requested() const noexcept { return _stop.stop_requested(); }
  std::stop_token stop_token() const noexcept { return _stop.get_token(); }
};

/// @brief awaiter yielding the reserved object of the current coroutine context
/// @note never suspends; only valid inside coroutines whose await_transform
///       supplies the CoroContext (Task). Using it elsewhere has no viable
///       await_resume — a compile error rather than a silent null dereference.
template <class T>
struct Reserved {
  constexpr bool await_ready() const noexcept { return true; }
  /// @brief resume with the reserved object of the current coroutine context
  constexpr T& await_resume(CoroContext& ctx) const noexcept {
    auto* p = ctx.get_reserved();
    assert(p != nullptr && "co_await Reserved<T>: context has no reserved object "
                           "(create the context with asio_ctx(...) or pass a "
                           "reserved pointer to CoroContext)");
    return *static_cast<T*>(p);
  }
};

XSL_CORO_NE
#endif  // XSL_CORO_CONTEXT
