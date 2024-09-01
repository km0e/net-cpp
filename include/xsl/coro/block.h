/**
 * @file block.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Block coroutine until the awaited coroutine finishes
 * @version 0.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_CORO_BLOCK
#  define XSL_CORO_BLOCK
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <cstdio>
#  include <expected>
#  include <optional>
#  include <semaphore>
#  include <type_traits>
#  include <utility>
XSL_CORO_NB
class Block {
protected:
  class BlockPromise {
  public:
    constexpr auto get_return_object() noexcept {
      return Block{std::coroutine_handle<BlockPromise>::from_promise(*this)};
    }

    inline void unhandled_exception() { std::rethrow_exception(std::current_exception()); }

    using executor_type = void;
    using coro_type = Block;

    constexpr std::suspend_never initial_suspend() const noexcept { return {}; }

    constexpr std::suspend_never final_suspend() const noexcept { return {}; }
  };

public:
  using promise_type = BlockPromise;

  constexpr explicit Block(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  constexpr Block(Block &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  constexpr ~Block() {}

protected:
  std::coroutine_handle<promise_type> _handle;
};

/**
 * @brief Block coroutine until the awaited coroutine finishes
 *
 * @tparam Awaiter
 * @param awaiter
 * @return decltype(auto)
 */
template <class Awaiter>
constexpr decltype(auto) block(Awaiter &&awaiter) {
  using awaiter_type = std::remove_reference_t<Awaiter>;
  using result_type = typename awaiter_traits<awaiter_type>::result_type;
  std::binary_semaphore sem{0};
  if constexpr (!std::is_same_v<result_type, void>) {
    return [&sem](Awaiter &&awaiter) -> result_type {
      std::optional<result_type> result{};
      auto _ = [&result, &sem](Awaiter &&awaiter) -> Block {
        result = co_await std::forward<Awaiter>(awaiter);
        sem.release();
      }(std::forward<Awaiter>(awaiter));
      sem.acquire();
      assert(result.has_value());
      return std::move(*result);
    }(std::forward<Awaiter>(awaiter));
  } else {
    return [&sem](Awaiter &&awaiter) -> void {
      auto _ = [&sem](Awaiter &&awaiter) -> Block {
        co_await std::forward<Awaiter>(awaiter);
        sem.release();
        LOG6("block: resume");
      }(std::forward<Awaiter>(awaiter));
      sem.acquire();
      DEBUG("block: final");
    }(std::forward<Awaiter>(awaiter));
  }
}
XSL_CORO_NE
#endif  // XSL_CORO_BLOCK
