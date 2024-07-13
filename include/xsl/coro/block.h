#pragma once

#ifndef XSL_CORO_BLOCK
#  define XSL_CORO_BLOCK
#  include "xsl/coro/def.h"
#  include "xsl/coro/final.h"

#  include <expected>
#  include <semaphore>
#  include <utility>
XSL_CORO_NB

template <ToAwaiter Awaiter>
auto block(Awaiter &&awaiter) -> typename awaiter_traits<to_awaiter_t<Awaiter>>::result_type {
  using result_type = typename awaiter_traits<to_awaiter_t<Awaiter>>::result_type;
  std::binary_semaphore sem{0};
  auto final = [&sem](Awaiter awaiter) -> Final<result_type> {
    struct Release {
      std::binary_semaphore &sem;
      ~Release() { sem.release(); }
    } release{sem};

    co_return co_await awaiter;
  }(std::move(awaiter));

  sem.acquire();
  return *final;
}

XSL_CORO_NE
#endif  // XSL_CORO_BLOCK
