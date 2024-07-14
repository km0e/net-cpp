#pragma once
#ifndef XSL_CORO_DETACH
#  define XSL_CORO_DETACH
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <cassert>
#  include <coroutine>
#  include <optional>
#  include <utility>
XSL_CORO_NB

template <class Awaiter>
  requires Awaitable<Awaiter>
void detach(Awaiter &&awaiter) {
  class Detach {
    class DetachPromise {
    public:
      using result_type = void;
      using executor_type = no_executor;
      DetachPromise() : _result(std::nullopt) {}
      decltype(auto) get_return_object() {
        return Detach{std::coroutine_handle<DetachPromise>::from_promise(*this)};
      }

      std::suspend_never initial_suspend() { return {}; }

      std::suspend_never final_suspend() noexcept { return {}; }

      void unhandled_exception() { this->_result = std::unexpected{std::current_exception()}; }

      std::nullptr_t executor() const noexcept { return nullptr; }

      void resume(std::coroutine_handle<> handle) noexcept {
        DEBUG("detach resume");
        handle();
        DEBUG("detach resume done");
      }

      void return_void() { this->_result = std::expected<void, std::exception_ptr>{}; }

    protected:
      std::optional<Result<result_type>> _result;
    };

  public:
    using promise_type = DetachPromise;

    explicit Detach(std::coroutine_handle<promise_type> handle) noexcept : _handle(handle) {}

  private:
    std::coroutine_handle<promise_type> _handle;
  };

  [[maybe_unused]] auto final = [](Awaiter awaiter) -> Detach {
    DEBUG("detach");
    co_await awaiter;
  }(std::move(awaiter));
}

XSL_CORO_NE
#endif
