#pragma once
#ifndef XSL_CORO_NEXT
#  define XSL_CORO_NEXT
#  include "xsl/coro/base.h"
#  include "xsl/coro/block.h"
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"

#  include <functional>
#  include <memory>
#  include <optional>
XSL_CORO_NB

template <typename ResultType, typename Executor>
class Next : public Coro<ResultType> {
private:
  using Base = Coro<ResultType>;

protected:
  using Base::Promise;
  using typename Base::Friend;

  class NextPromiseBase : public Base::PromiseBase {
    using PromiseBase = Base::PromiseBase;

  public:
    using result_type = PromiseBase::result_type;
    using executor_type = Executor;

    NextPromiseBase() : PromiseBase(), _next_resume(std::nullopt), _executor(nullptr) {}

    template <class Promise>
    void resume(std::coroutine_handle<Promise> handle) {
      this->dispatch([handle, this]() mutable {
        DEBUG("task resume {}", (uint64_t)handle.address());
        handle();
        DEBUG("task resume {} done", (uint64_t)handle.address());
        if (handle.done()) {
          DEBUG("task resume handle done");
          if (this->_next_resume) {
            DEBUG("task resume next_resume");
            (*this->_next_resume)();
          }
        }
      });
    }

    template <typename F>
      requires std::invocable<F>
    void dispatch(F &&f) {
      if (this->executor()) {
        this->_executor->schedule(std::move(f));
      } else {
        f();
      }
    }

    const std::shared_ptr<Executor> &executor() const noexcept { return _executor; }

    template <class Promise>
    void next(std::coroutine_handle<Promise> handle) {
      _next_resume = [handle]() mutable { handle.promise().resume(handle); };
    }

    auto &&by(this auto &&self, const std::shared_ptr<Executor> &executor) {
      self._executor = executor;
      return std::forward<decltype(self)>(self);
    }

  protected:
    std::optional<std::function<void()>> _next_resume;

    std::shared_ptr<Executor> _executor;
  };
  using PromiseBase = NextPromiseBase;

public:
  using result_type = NextPromiseBase::result_type;
  using executor_type = NextPromiseBase::executor_type;

  auto &&by(this auto &&self, std::shared_ptr<executor_type> &executor) {
    self.get_handle().promise().by(executor);
    return std::forward<decltype(self)>(self);
  }

  result_type block(this auto &&self) { return coro::block(self._co_await()); }

  void detach(this auto &&self) {
    DEBUG("task detach");
    self._co_await();
  }

  void detach(std::shared_ptr<executor_type> &executor) { this->by(executor).detach(); }
};
XSL_CORO_NE
#endif
