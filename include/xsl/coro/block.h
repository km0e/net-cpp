#pragma once
#ifndef XSL_CORO_BLOCK
#  define XSL_CORO_BLOCK
#  include "xsl/coro/def.h"
#  include "xsl/coro/result.h"
#  include "xsl/logctl.h"

#  include <condition_variable>
#  include <coroutine>
#  include <cstddef>
#  include <mutex>
#  include <utility>
XSL_CORO_NAMESPACE_BEGIN
template <class ResultType>
class BlockPromise;
template <class ResultType>
class Block {
public:
  using promise_type = BlockPromise<ResultType>;

  explicit Block(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Block(Block &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  Block(Block &) = delete;

  Block &operator=(Block &) = delete;

  ~Block() {
    DEBUG( "~Block");
    if (_handle) _handle.destroy();
  }

  ResultType operator*() { return *_handle.promise(); }

private:
  std::coroutine_handle<promise_type> _handle;
};

namespace detail {
  template <class ResultType>
  class BlockPromiseBase {
  public:
    using result_type = ResultType;
    using executor_type = no_executor;
    BlockPromiseBase() : _result(std::nullopt), _fin_lock(), _fin_cv() {}
    std::suspend_never initial_suspend() {
      DEBUG( "initial_suspend");
      return {};
    }

    std::suspend_always final_suspend() noexcept {
      DEBUG( "final_suspend");
      _fin_cv.notify_all();
      return {};
    }

    void unhandled_exception() { _result = Result<ResultType>(std::current_exception()); }

    ResultType operator*() {
      DEBUG( "operator*");
      std::unique_lock<std::mutex> lock(_fin_lock);
      if (!_result.has_value()) _fin_cv.wait(lock);
      return _result->get_or_throw();
    }

    std::nullptr_t executor() const noexcept { return nullptr; }

  protected:
    std::optional<Result<ResultType>> _result;

    std::mutex _fin_lock;
    std::condition_variable _fin_cv;
  };
}  // namespace detail
template <class ResultType>
class BlockPromise : public detail::BlockPromiseBase<ResultType> {
public:
  using Base = detail::BlockPromiseBase<ResultType>;
  using Base::_result;

  Block<ResultType> get_return_object() {
    return Block{std::coroutine_handle<BlockPromise>::from_promise(*this)};
  }

  void return_value(ResultType value) {
    DEBUG( "return_value");
    _result = Result<ResultType>(std::move(value));
  }
};
template <>
class BlockPromise<void> : public detail::BlockPromiseBase<void> {
public:
  using Base = detail::BlockPromiseBase<void>;
  using Base::_result;

  Block<void> get_return_object() {
    return Block{std::coroutine_handle<BlockPromise>::from_promise(*this)};
  }
  void return_void() {
    DEBUG( "return_value");
    _result = Result<void>();
  }
};
/**
 * @brief block
 * @tparam Awaiter
 * @param task
 * @return Block<typename awaiter_traits<to_awaiter_t<Awaiter>>::result_type>
 * @note after wrapping the task into a block, you need to call operator* to block the current
 * thread until the task is finished
 */
template <ToAwaiter Awaiter>
Block<typename awaiter_traits<to_awaiter_t<Awaiter>>::result_type> block(Awaiter &&task) {
  co_return co_await std::move(task);
}

XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_BLOCK
