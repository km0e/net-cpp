#pragma once

#ifndef XSL_NET_TRANSPORT_TCP_ACCEPT
#  define XSL_NET_TRANSPORT_TCP_ACCEPT
#  include "xsl/coro/def.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync.h"
#  include "xsl/sys.h"

#  include <atomic>
#  include <coroutine>
#  include <expected>
#  include <memory>
#  include <mutex>
#  include <optional>
#  include <queue>
#  include <system_error>
#  include <utility>
TCP_NB
using namespace xsl::sync;

namespace impl_tcp_acceptor {
  struct InnerHandle {
    InnerHandle() noexcept : closed(), mtx(), cb(), events() {}
    std::atomic_flag closed;
    std::mutex mtx;
    std::optional<std::function<void()>> cb;
    std::queue<IOM_EVENTS> events;
  };
  class Callback {
  public:
    Callback(const std::shared_ptr<InnerHandle> &handle) : handle(handle) {}
    sync::PollHandleHintTag operator()([[maybe_unused]] int fd, IOM_EVENTS events) {
      DEBUG("acceptor");
      if (this->handle->closed.test()) {
        return PollHandleHintTag::DELETE;
      }
      this->handle->mtx.lock();
      this->handle->events.push(events);
      if (this->handle->cb) {
        DEBUG("dispatch");
        auto cb = std::exchange(this->handle->cb, std::nullopt);
        (*cb)();
        /**
         * @brief why move cb before calling it? not calling it directly and then reset it?
         * if we call it directly, it may call acceptor again, and reset cb will loss the callback
         *
         */
      } else {
        DEBUG("no cb");
        this->handle->mtx.unlock();
      }
      DEBUG("return");
      return PollHandleHintTag::NONE;
    }

  private:
    std::shared_ptr<InnerHandle> handle;
  };
}  // namespace impl_tcp_acceptor

/**
 * @brief Acceptor is a coroutine that can be used to accept a connection
 *
 */
class Acceptor {
public:
  using result_type = AcceptResult;
  Acceptor(Socket &&skt, std::shared_ptr<Poller> poller)
      : sock(std::move(skt)), handle(std::make_shared<impl_tcp_acceptor::InnerHandle>()) {
    poller->add(this->sock.raw_fd(), IOM_EVENTS::IN,
                impl_tcp_acceptor::Callback{this->handle});  // register callback
  }
  Acceptor(Acceptor &&rhs) noexcept = default;
  ~Acceptor() = default;
  bool await_ready() noexcept {
    DEBUG("");
    this->handle->mtx.lock();
    return !this->handle->events.empty();
  }
  template <class Promise>
  void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
    DEBUG("need to suspend");
    this->handle->cb = [handle]() { handle.promise().resume(handle); };
    this->handle->mtx.unlock();
  }
  AcceptResult await_resume() noexcept {
    DEBUG("return result");
    this->handle->mtx.unlock();
    auto events = this->handle->events.front();
    this->handle->events.pop();
    if (!!(events & IOM_EVENTS::IN)) {
      DEBUG("Fd: {} is readable", this->sock.raw_fd());
      return sys::accept(this->sock);
    } else if (!events) {
      ERROR("Timeout");
      return AcceptResult{std::unexpect, std::errc(ETIMEDOUT)};
    }
    return AcceptResult{std::unexpect, std::errc(ECONNREFUSED)};
  }

private:
  Socket sock;
  std::shared_ptr<impl_tcp_acceptor::InnerHandle> handle;
};
static_assert(coro::Awaitable<Acceptor>, "Acceptor must be an Awaitable");

TCP_NE
#endif
