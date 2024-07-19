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

/**
 @brief Acceptor is a coroutine that can be used to accept a connection
 */
class Acceptor {
public:
  Acceptor(Socket &&skt, std::shared_ptr<Poller> poller)
      : sock(std::move(skt)), sem(std::make_shared<coro::CountingSemaphore<1>>(false)) {
    using InCallback = sync::PollCallback<IOM_EVENTS::IN>;
    poller->add(this->sock.raw_fd(), IOM_EVENTS::IN, InCallback{this->sem});
  }
  Acceptor(Acceptor &&rhs) noexcept = default;
  ~Acceptor() = default;

  coro::Task<AcceptResult> accept() noexcept {
    DEBUG("acceptor accept");
    while (true) {
      auto res = xsl::accept(this->sock);
      if (res) {
        co_return std::move(*res);
      } else if (res.error() == std::errc::resource_unavailable_try_again
                 || res.error() == std::errc::operation_would_block) {
        co_await *this->sem;
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }

private:
  Socket sock;
  std::shared_ptr<coro::CountingSemaphore<1>> sem;
};

TCP_NE
#endif
