#pragma once

#ifndef XSL_NET_TRANSPORT_TCP_ACCEPT
#  define XSL_NET_TRANSPORT_TCP_ACCEPT
#  include "xsl/coro/semaphore.h"
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync.h"
#  include "xsl/sys.h"

#  include <expected>
#  include <memory>
#  include <utility>
TCP_NB
using namespace xsl::sync;

namespace impl_tcp_acceptor {
  class Callback {
  public:
    Callback(const std::shared_ptr<coro::CountingSemaphore<1>> &sem) : sem(sem) {}
    sync::PollHandleHintTag operator()([[maybe_unused]] int fd, IOM_EVENTS events) {
      DEBUG("acceptor");
      if (this->sem.unique()) {
        return PollHandleHintTag::DELETE;
      }
      if (!!(events & IOM_EVENTS::IN)) {
        sem->release();
      } else if (!events) {
        ERROR("Timeout");
        return PollHandleHintTag::DELETE;
      }
      // TODO: handle other events
      return PollHandleHintTag::NONE;
    }

  private:
    std::shared_ptr<coro::CountingSemaphore<1>> sem;
  };
}  // namespace impl_tcp_acceptor

/**
 * @brief Acceptor is a coroutine that can be used to accept a connection
 *
 */
class Acceptor {
public:
  Acceptor(Socket &&skt, std::shared_ptr<Poller> poller)
      : sock(std::move(skt)), sem(std::make_shared<coro::CountingSemaphore<1>>(false)) {
    poller->add(this->sock.raw_fd(), IOM_EVENTS::IN, impl_tcp_acceptor::Callback{this->sem});
  }
  Acceptor(Acceptor &&rhs) noexcept = default;
  ~Acceptor() = default;

  coro::Task<std::tuple<Socket, IpAddr>> accept() noexcept {
    DEBUG("acceptor accept");
    while (true) {
      co_await *this->sem;
      auto res = xsl::accept(this->sock);
      if (res) {
        co_return std::move(*res);
      }
    }
  }

private:
  Socket sock;
  std::shared_ptr<coro::CountingSemaphore<1>> sem;
};

TCP_NE
#endif
