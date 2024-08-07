#pragma once
#ifndef XSL_NET_TRANSPORT_ACCEPT
#  define XSL_NET_TRANSPORT_ACCEPT
#  include "xsl/feature.h"
#  include "xsl/net/transport/def.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys/net.h"
#  include "xsl/sys/net/accept.h"
#  include "xsl/sys/net/dev.h"
#  include "xsl/sys/net/socket.h"
TRANSPORT_NB
class Acceptor {
public:
  static std::expected<Acceptor, std::error_condition> create(sync::Poller &poller,
                                                              sys::Socket &&socket) {
    auto async = std::move(socket).async(poller);
    return Acceptor{std::move(async)};
  }
  Acceptor(sys::net::AsyncDevice<feature::InOut<std::byte>> &&dev) : dev(std::move(dev)) {}
  Acceptor(Acceptor &&) = default;
  Acceptor &operator=(Acceptor &&) = default;
  ~Acceptor() {}
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::expected<sys::Socket, std::errc>, Executor> accept(
      sys::net::SockAddr *addr) noexcept {
    while (true) {
      auto res = sys::net::accept(this->dev.inner(), addr);
      if (res) {
        auto sock = std::move(*res);
        co_return sock;
      } else if (res.error() == std::errc::resource_unavailable_try_again
                 || res.error() == std::errc::operation_would_block) {
        if (!co_await this->dev.read_sem()) {
          co_return std::unexpected{std::errc::operation_canceled};
        }
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }

private:
  sys::net::AsyncDevice<feature::InOut<std::byte>> dev;
};

TRANSPORT_NE
#endif
