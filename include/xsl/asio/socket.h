/**
 * @file socket.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket type
 * @version 0.13
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_SOCKET
#  define XSL_ASIO_SOCKET
#  include "xsl/asio/def.h"
#  include "xsl/asio/dev.h"
#  include "xsl/asio/io.h"
#  include "xsl/net.h"
#  include "xsl/sys.h"

#  include <sys/socket.h>

XSL_ASIO_NB
using namespace xsl::net;
using io::IOM_EVENTS;

template <typename Traits>
struct AsyncConnectionUtils : public sys::net::ConnectionUtils<Traits> {};

template <class Traits>
class AsyncSocket : public Traits,
                    public AsyncDevice<IOM_EVENTS::IN, IOM_EVENTS::OUT>,
                    public AsyncConnectionUtils<Traits>,
                    public NetAsyncRx,
                    public NetAsyncTx {
public:
  using Base = AsyncDevice;
  using Base::Base;

  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;

  using value_type = byte;
  AsyncSocket(Poller &poller, sys::net::Socket<Traits> &&sock)
      : Base(std::move(sock).into_raw(), poller, typename Traits::poll_traits_type{}) {}

  explicit AsyncSocket(Poller &poller, SocketAttribute attr = SocketAttribute::NonBlocking
                                                              | SocketAttribute::CloseOnExec)
      : AsyncSocket(poller, sys::net::Socket<Traits>(attr)) {}
};

template <class... Flags>
using AsyncSocketCompose = AsyncSocket<sys::net::SocketTraits<Flags...>>;

template <sys::net::ConnectionBasedSocketTraits Traits>
struct AsyncConnectionUtils<Traits> : public sys::net::ConnectionUtils<Traits> {
  using Base = sys::net::ConnectionUtils<Traits>;

  /// @brief Accept a connection
  Task<std::expected<net::Socket<Traits>, errc>> accept(this auto &self,
                                                        sys::net::SockAddr<Traits> *addr
                                                        = nullptr) {
    while (true) {
      auto res = Base::accept(self.raw(), addr);
      if (res) {
        co_return std::move(*res);
      } else if (res.error() == errc::resource_unavailable_try_again
                 || res.error() == errc::operation_would_block) {
        if (!co_await self.read_signal()) {
          co_return std::unexpected{errc::not_connected};
        }
      } else {
        co_return std::unexpected{res.error()};
      }
    }
  }
};

XSL_ASIO_NE
#endif
