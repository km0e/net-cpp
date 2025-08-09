/**
 * @file socket.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket type
 * @version 0.2.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_SOCKET
#  define XSL_SYS_NET_SOCKET
#  include <sys/socket.h>
#  include <xsl/def.h>
#  include <xsl/error.h>
#  include <xsl/sys/dev.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/net/sockaddr.h>

#  include <expected>

XSL_SYS_NET_NB
using namespace xsl::io;
template <class Traits>
class Socket;

template <class Traits>
struct ConnectionUtils {
  /// @brief Connect to a address
  expected<void, errc> connect(this Socket<Traits> &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(filter_interrupt(::connect, self.raw(), &addr, addrlen));
  }
  /// @brief Bind to a address
  constexpr Expected<void> bind(this auto &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(::bind(self.raw(), &addr, addrlen));
  }
};

/// @brief Socket
/// @tparam Traits Socket traits
template <class Traits>
class Socket : public Traits, public RawDevice, public ConnectionUtils<Traits> {
public:
  using Base = RawDevice;
  using Base::Base;

  using traits_type = Traits;
  using poll_traits_type = typename traits_type::poll_traits_type;

  using value_type = byte;

  Socket(int family, int type, int protocol,
         SocketAttribute attr = SocketAttribute::NonBlocking | SocketAttribute::CloseOnExec)
      : Traits(family, type, protocol),
        Base(::socket(family, type | static_cast<int>(attr), protocol)) {}

  explicit Socket(SocketAttribute attr
                  = SocketAttribute::NonBlocking | SocketAttribute::CloseOnExec)
    requires requires { Traits::StaticFamily; }
      : Traits(),
        Base(::socket(this->family(), this->type() | static_cast<int>(attr), this->protocol())) {}
  constexpr RawOwner into_raw() && { return std::move(*this); }
};

template <ConnectionBasedSocketTraits Traits>
struct ConnectionUtils<Traits> {
  /// @brief Connect to a address
  errc connect(this Socket<Traits> &self,
               const SockAddr<Traits>
                   &sa) {  // @NOTE:This may return INPROGRESS, because the socket is non-blocking
    auto [addr, addrlen] = sa.raw();
    return check_ec(filter_interrupt(::connect, self.raw(), &addr, addrlen));
  }
  /// @brief Bind to a address
  constexpr errc bind(this auto &&self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(::bind(self.raw(), &addr, addrlen));
  }
  /// @brief Accept a connection
  constexpr std::expected<Socket<Traits>, errc> accept(this auto &&self,
                                                       SockAddr<Traits> *addr = nullptr) {
    return ConnectionUtils::accept(self.raw(), addr);
  }
  /// @brief Start listening
  constexpr std::expected<void, errc> listen(this auto &&self, int max_connections = 128) {
    return check_ec(::listen(self.raw(), max_connections));
  }

protected:
  static constexpr std::expected<Socket<Traits>, errc> accept(RawHandle _raw,
                                                              SockAddr<Traits> *addr) {
    auto tmp_fd = [_raw, addr] {
      if (addr == nullptr) {
        return ::accept4(_raw, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
      } else {
        auto [sockaddr, addrlen] = addr->raw();
        return ::accept4(_raw, &sockaddr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
      }
    }();
    if (tmp_fd < 0) {
      return std::unexpected{errc(errno)};
    }
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
// @brief Compose a socket with multiple flags
// @note This is a convenience type alias for Socket with multiple flags.
template <class... Flags>
using SocketCompose = Socket<SocketTraits<Flags...>>;
XSL_SYS_NET_NE
#endif
