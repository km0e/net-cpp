#pragma once

#ifndef XSL_CORO_BLOCK
#  define XSL_CORO_BLOCK
#  include "xsl/coro/base.h"
#  include "xsl/coro/def.h"

#  include <cassert>
#  include <coroutine>
#  include <expected>
#  include <semaphore>
XSL_CORO_NB
template <class ResultType>
class Block;

template <class ResultType>
class BlockPromiseBase : public PromiseBase<ResultType> {
private:
  using Base = PromiseBase<ResultType>;

public:
  using coro_type = Block<ResultType>;
  using typename Base::result_type;
};

template <class ResultType>
class Block : public Coro<ResultType> {
private:
  using Base = Coro<ResultType>;

protected:
  friend typename Base::Friend;

public:
  using promise_type = Promise<BlockPromiseBase<ResultType>>;
  using typename Base::result_type;

  explicit Block(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Block(Block &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  result_type operator*() { return *_handle.promise(); }

  ~Block() {
    assert(!_handle || _handle.done());
    if (_handle) {
      _handle.destroy();
    }
  }

protected:
  std::coroutine_handle<promise_type> _handle;
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
template <class Awaiter>
  requires(!std::is_lvalue_reference_v<Awaiter>)
decltype(auto) block(Awaiter &&awaiter) {
  Awaiter tmp = std::forward<Awaiter>(awaiter);
  return block(tmp);
}

XSL_CORO_NE
#endif  // XSL_CORO_BLOCK
