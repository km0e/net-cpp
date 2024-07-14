#pragma once

#ifndef XSL_CORO_BLOCK
#  define XSL_CORO_BLOCK
#  include "xsl/coro/def.h"

#  include <coroutine>
#  include <expected>
#  include <optional>
#  include <semaphore>
XSL_CORO_NB

template <class ResultType>
class BlockPromise;

template <class ResultType>
class Block {
  class BlockPromiseBase {
  public:
    using result_type = ResultType;
    using executor_type = no_executor;
    BlockPromiseBase() : _result(std::nullopt) {}
    auto get_return_object(this auto &&self) {
      return Block{std::coroutine_handle<std::remove_cvref_t<decltype(self)>>::from_promise(self)};
    }

    std::suspend_never initial_suspend() { return {}; }

    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() { this->_result = std::unexpected{std::current_exception()}; }

    std::nullptr_t executor() const noexcept { return nullptr; }

    template <class Promise>
    void resume(std::coroutine_handle<Promise> handle) noexcept {
      handle();
    }
    template <class F>
    void dispatch(F &&f) {
      f();
    }

    result_type operator*() {
      if (*_result) {
        if constexpr (std::is_same_v<result_type, void>) {
          return **_result;
        } else {
          return std::move(**_result);
        }
      } else {
        std::rethrow_exception(_result->error());
      }
    }

  protected:
    std::optional<Result<result_type>> _result;
  };

public:
  using PromiseBase = BlockPromiseBase;

  using promise_type = BlockPromise<ResultType>;

  explicit Block(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Block(Block &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  Block(const Block &) = delete;

  ResultType operator*() { return *_handle.promise(); }

  ~Block() {
    assert(!_handle || _handle.done());
    if (_handle) {
      _handle.destroy();
    }
  }

private:
  std::coroutine_handle<promise_type> _handle;
};

template <class ResultType>
class BlockPromise : public Block<ResultType>::PromiseBase {
  using Base = Block<ResultType>::PromiseBase;
  using Base::_result;

public:
  using result_type = typename Base::result_type;

  void return_value(result_type &&value) { this->_result = std::move(value); }
};

template <>
class BlockPromise<void> : public Block<void>::PromiseBase {
  using Base = Block<void>::PromiseBase;
  using Base::_result;

public:
  using result_type = typename Base::result_type;

  void return_void() { this->_result = std::expected<void, std::exception_ptr>{}; }
};

template <class Awaiter>
  requires Awaitable<Awaiter, Block<typename awaiter_traits<Awaiter>::result_type>>
auto block(Awaiter &awaiter) -> typename awaiter_traits<Awaiter>::result_type {
  using result_type = typename awaiter_traits<Awaiter>::result_type;
  std::binary_semaphore sem{0};
  auto final = [&sem](Awaiter &awaiter) -> Block<result_type> {
    struct Release {
      std::binary_semaphore &sem;
      ~Release() { sem.release(); }
    } release{sem};

    co_return co_await awaiter;
  }(awaiter);

  sem.acquire();
  return *final;
}

XSL_CORO_NE
#endif  // XSL_CORO_BLOCK
