/**
 * @file socket.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket type
 * @version 0.2.1
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_SOCKET
#  define XSL_SYS_NET_SOCKET
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <sys/socket.h>
#  include <xsl/def.h>
#  include <xsl/error.h>
#  include <xsl/sys/dev.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/net/sockaddr.h>

XSL_SYS_NET_NB
using namespace xsl::io;
template <class Traits>
class Socket;

template <class Traits>
struct ConnectionUtils {
  /// @brief Connect to a address
  Expected<void, errc> connect(this Socket<Traits>& self, const SockAddr<Traits>& sa) {
    ENSEC(filter_interrupt(::connect, self.raw(), &sa.addr(), sa.len()) == 0);
    return {};
  }
  /// @brief Bind to a address
  constexpr Expected<void, errc> bind(this auto& self, const SockAddr<Traits>& sa) {
    ENSEC(::bind(self.raw(), &sa.addr(), sa.len()) == 0);
    return {};
  }
  consteval bool is_connection_based() const { return Traits::is_connection_based(); }
};

template <class Traits>
struct SocketOptions;

/// @brief Socket
/// @tparam Traits Socket traits
template <class Traits>
class Socket : public Traits,
               public RawDevice,
               public ConnectionUtils<Traits>,
               public SocketOptions<Traits> {
public:
  using Base = RawDevice;
  using Base::Base;

  using traits_type = Traits;

  using value_type = byte;

  constexpr RawOwner into_raw() && noexcept(std::is_nothrow_move_constructible_v<Socket>) {
    return std::move(*this);
  }
};

template <ConnectionBasedSocketTraits Traits>
struct ConnectionUtils<Traits> {
  /// @brief Connect to a address
  Expected<void, errc> connect(this Socket<Traits>& self,
                               const SockAddr<Traits>& sa) {  // @NOTE:This may return INPROGRESS,
                                                              // because the socket is non-blocking
    auto [addr, addrlen] = sa.raw();
    ENSEC(filter_interrupt(::connect, self.raw(), &addr, addrlen) == 0);
    return {};
  }
  /// @brief Bind to a address
  constexpr Expected<void, errc> bind(this auto&& self, const SockAddr<Traits>& sa) noexcept {
    ENSEC(::bind(self.raw(), &sa.addr(), sa.len()) == 0);
    return {};
  }
  /// @brief Accept a connection
  constexpr Expected<Socket<Traits>, errc> accept(this auto&& self,
                                                  SockAddr<Traits>* addr = nullptr) {
    return ConnectionUtils::accept(self.raw(), addr);
  }
  /// @brief Start listening
  constexpr Expected<void, errc> listen(this auto&& self, int max_connections = 128) noexcept {
    ENSEC(::listen(self.raw(), max_connections) == 0);
    return {};
  }

  consteval bool is_connection_based() const { return Traits::is_connection_based(); }

protected:
  static constexpr Expected<Socket<Traits>, errc> accept(RawHandle _raw, SockAddr<Traits>* addr) {
    auto tmp_fd = [_raw, addr] {
      if (addr == nullptr) {
        return ::accept4(_raw, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
      } else {
        return ::accept4(_raw, &addr->addr(), &addr->len(), SOCK_NONBLOCK | SOCK_CLOEXEC);
      }
    }();
    ENSEC(tmp_fd >= 0);
    log_debug("accept socket {}", tmp_fd);
    // char ip[NI_MAXHOST], port[NI_MAXSERV];
    // if (getnameinfo(&addr, addrlen, ip, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST |
    // NI_NUMERICSERV)
    //     != 0) {
    //   return std::unexpected{errc(errno)};
    // }
    return Socket<Traits>(tmp_fd);
  }
};

template <class Traits>
struct SocketOptions {  /// TODO: add more socket options
  Expected<void, errc> reuse_addr(this auto&& self, bool reuse = true) noexcept {
    int opt = reuse ? 1 : 0;
    ENSEC(setsockopt(self.raw(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0);
    return {};
  }
  /// @brief Toggle SO_REUSEPORT: allow multiple sockets to bind the same port
  /// @note the kernel then load-balances incoming connections across all
  ///       sockets that joined the group (same UID, all set before bind) —
  ///       the basis for the multi-poller server model; implies REUSEADDR
  Expected<void, errc> reuse_port(this auto&& self, bool reuse = true) noexcept {
    int opt = reuse ? 1 : 0;
    ENSEC(setsockopt(self.raw(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == 0);
    return {};
  }
  /// @brief Toggle TCP_NODELAY (disable Nagle's algorithm); no-op on non-TCP
  /// @note keep-alive request/response servers want this on, otherwise the
  ///       client's delayed ACK stalls the next response for up to 40ms
  Expected<void, errc> no_delay(this auto&& self, bool enable = true) noexcept {
    if constexpr (requires { self.is_tcp; }) {
      if constexpr (self.is_tcp) {
        int opt = enable ? 1 : 0;
        ENSEC(setsockopt(self.raw(), IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == 0);
      }
    }
    return {};
  }
};

// @brief Compose a socket with multiple flags
// @note This is a convenience type alias for Socket with multiple flags.
template <class... Flags>
using SocketCompose = Socket<SocketTraits<Flags...>>;

template <class Traits>
Expected<Socket<Traits>, errc> socket(const SockAddr<Traits>& sa,
                                      SocketAttribute attr
                                      = SocketAttribute::NonBlocking
                                        | SocketAttribute::CloseOnExec) noexcept {
  int fd = ::socket(sa.family(), sa.type() | static_cast<int>(attr), sa.protocol());
  ENSEC(fd != -1);
  return Socket<Traits>(fd);
}
template <class Traits>
Expected<Socket<Traits>, errc> socket(const addrinfo& ai,
                                      SocketAttribute attr
                                      = SocketAttribute::NonBlocking
                                        | SocketAttribute::CloseOnExec) noexcept {
  int fd = ::socket(ai.ai_family, ai.ai_socktype | static_cast<int>(attr), ai.ai_protocol);
  ENSEC(fd != -1);
  return Socket<Traits>(fd);
}

XSL_SYS_NET_NE
#endif
