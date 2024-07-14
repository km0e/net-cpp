#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_ACCEPT
#  define XSL_NET_TRANSPORT_TCP_ACCEPT
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync.h"
#  include "xsl/sys.h"

#  include <coroutine>
#  include <expected>
#  include <optional>
#  include <queue>
#  include <system_error>
#  include <utility>
TCP_NB
using namespace xsl::sync;
/**
 * @brief Acceptor is a coroutine that can be used to accept a connection
 *
 */
class Acceptor {
  class AcceptorImpl {
  public:
    AcceptorImpl(Socket &&skt) : skt(std::move(skt)), cb(std::nullopt), mtx_(), events() {}
    sync::PollHandleHintTag operator()([[maybe_unused]] int fd, IOM_EVENTS events) {
      DEBUG("acceptor");
      this->mtx_.lock();
      this->events.push(events);
      if (this->cb) {
        DEBUG("dispatch");
        auto cb = std::exchange(this->cb, std::nullopt);
        (*cb)();
        /**
         * @brief why move cb before calling it? not calling it directly and then reset it?
         * if we call it directly, it may call acceptor again, and reset cb will loss the callback
         *
         */
      } else {
        DEBUG("no cb");
        this->mtx_.unlock();
      }
      DEBUG("return");
      return PollHandleHintTag::NONE;
    }

    Socket skt;

    std::optional<std::function<void()>> cb;

    std::mutex mtx_;
    std::queue<IOM_EVENTS> events;
  };

public:
  using result_type = AcceptResult;
  Acceptor(Socket &&skt, std::shared_ptr<Poller> poller)
      : impl(std::make_unique<AcceptorImpl>(std::move(skt))) {
    poller->add(impl->skt.raw_fd(), IOM_EVENTS::IN,
                [impl = impl.get()](int fd, IOM_EVENTS events) { return (*impl)(fd, events); });
  }
  ~Acceptor() = default;
  bool await_ready() noexcept {
    DEBUG("");
    this->impl->mtx_.lock();
    return !this->impl->events.empty();
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    DEBUG("need to suspend");
    this->impl->cb = [handle]() { handle.promise().dispatch([handle]() { handle.resume(); }); };
    this->impl->mtx_.unlock();
  }
  AcceptResult await_resume() noexcept {
    DEBUG("return result");
    this->impl->mtx_.unlock();
    auto events = this->impl->events.front();
    this->impl->events.pop();
    if (!!(events & IOM_EVENTS::IN)) {
      DEBUG("Fd: {} is readable", this->impl->skt.raw_fd());
      return sys::accept(this->impl->skt);
    } else if (!events) {
      ERROR("Timeout");
      return AcceptResult{std::unexpect, std::errc(ETIMEDOUT)};
    }
    return AcceptResult{std::unexpect, std::errc(ECONNREFUSED)};
  }

private:
  std::unique_ptr<AcceptorImpl> impl;
};
static_assert(coro::Awaitable<Acceptor>, "Acceptor must be an Awaitable");

TCP_NE
#endif
