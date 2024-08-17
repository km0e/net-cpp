#pragma once
#ifndef XSL_NET_TRANSPORT_ACCEPT
#  define XSL_NET_TRANSPORT_ACCEPT
#  include "xsl/net/transport/def.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys/net/accept.h"
#  include "xsl/sys/net/socket.h"
TRANSPORT_NB

template <class LowerLayer>
class Acceptor {
public:
  using lower_layer_type = LowerLayer;
  using layer_type = sys::net::TcpSocket<lower_layer_type>;
  using async_layer_type = sys::net::AsyncTcpSocket<lower_layer_type>;

  static std::expected<Acceptor, std::error_condition> create(sync::Poller &poller,
                                                              layer_type &&socket) {
    auto async = std::move(socket).async(poller);
    return Acceptor{std::move(async)};
  }
  Acceptor(async_layer_type &&dev) : _dev(std::move(dev)) {}
  Acceptor(Acceptor &&) = default;
  Acceptor &operator=(Acceptor &&) = default;
  ~Acceptor() {}
  template <class Executor = coro::ExecutorBase>
  Task<std::expected<layer_type, std::errc>, Executor> accept(
      sys::net::SockAddr *addr) noexcept {
    while (true) {
      auto res = sys::net::accept(this->_dev.inner(), addr);
      if (res) {
        auto sock = std::move(*res);
        co_return sock;
      } else if (res.error() == std::errc::resource_unavailable_try_again
                 || res.error() == std::errc::operation_would_block) {
        if (!co_await this->_dev.read_sem()) {
          co_return std::unexpected{std::errc::operation_canceled};
        }
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }

private:
  async_layer_type _dev;
};

TRANSPORT_NE
#endif
