#pragma once
#ifndef XSL_CORO_FINAL
#  define XSL_CORO_FINAL
#  include "xsl/coro/def.h"

#  include <coroutine>
#  include <cstddef>
#  include <exception>
#  include <utility>
XSL_CORO_NAMESPACE_BEGIN
class Final {
public:
  struct FinalPromise {
    using result_type = void;
    using executor_type = no_executor;
    std::suspend_never initial_suspend() { return {}; }

    std::suspend_never final_suspend() noexcept { return {}; }

    void return_void() {}

    void unhandled_exception() { std::rethrow_exception(std::current_exception()); }

    Final get_return_object() {
      return Final{std::coroutine_handle<FinalPromise>::from_promise(*this)};
    }

    std::nullptr_t executor() const noexcept { return nullptr; }
  };

  using promise_type = FinalPromise;

  explicit Final(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  Final(Final &&task) noexcept : _handle(std::exchange(task._handle, {})) {}

  Final(Final &) = delete;

  void operator()() const noexcept { _handle(); }

  ~Final() {}

private:
  std::coroutine_handle<promise_type> _handle;
};

template <ToAwaiter Awaiter>
decltype(auto) final(Awaiter &&awaiter) {
  class FinalWrapper {
  public:
    using promise_type [[maybe_unused]] = typename Awaiter::promise_type;

    FinalWrapper(Awaiter &&awaiter) : awaiter(std::move(awaiter)) {}
    FinalWrapper(FinalWrapper &&wrapper) noexcept : awaiter(std::move(wrapper.awaiter)) {}
    FinalWrapper &operator=(FinalWrapper &&wrapper) noexcept {
      awaiter = std::move(wrapper.awaiter);
      return *this;
    }
    void operator()() {
      [[maybe_unused]] auto detached
          = [](Awaiter awaiter) -> Final { co_await awaiter; }(std::move(awaiter));
    }

  private:
    Awaiter awaiter;
  };

  return FinalWrapper{std::forward<Awaiter>(awaiter)};
}

XSL_CORO_NAMESPACE_END
#endif  // XSL_CORO_FINAL
