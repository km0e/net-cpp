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
  Expected<void, errc> connect(this Socket<Traits> &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    ENSEC(filter_interrupt(::connect, self.raw(), &addr, addrlen));
    return {};
  }
  /// @brief Bind to a address
  constexpr Expected<void> bind(this auto &self, const SockAddr<Traits> &sa) {
    auto [addr, addrlen] = sa.raw();
    return check_ec(::bind(self.raw(), &addr, addrlen));
  }
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
  using poll_traits_type = typename traits_type::poll_traits_type;

  using value_type = byte;

  constexpr RawOwner into_raw() && noexcept(std::is_nothrow_move_constructible_v<Socket>) {
    return std::move(*this);
  }
};

template <ConnectionBasedSocketTraits Traits>
struct ConnectionUtils<Traits> {
  /// @brief Connect to a address
  Expected<> connect(this Socket<Traits> &self,
                     const SockAddr<Traits> &sa) {  // @NOTE:This may return INPROGRESS, because
                                                    // the socket is non-blocking
    auto [addr, addrlen] = sa.raw();
    return check_ec(filter_interrupt(::connect, self.raw(), &addr, addrlen));
  }
  /// @brief Bind to a address
  constexpr Expected<void, errc> bind(this auto &&self, const SockAddr<Traits> &sa) noexcept {
    auto [addr, addrlen] = sa.raw();
    ENSEC(::bind(self.raw(), &addr, addrlen) == 0);
    return {};
  }
  /// @brief Accept a connection
  constexpr Expected<Socket<Traits>> accept(this auto &&self, SockAddr<Traits> *addr = nullptr) {
    return ConnectionUtils::accept(self.raw(), addr);
  }
  /// @brief Start listening
  constexpr Expected<void, errc> listen(this auto &&self, int max_connections = 128) noexcept {
    ENSEC(::listen(self.raw(), max_connections) == 0);
    return {};
  }

protected:
  static constexpr Expected<Socket<Traits>, errc> accept(RawHandle _raw, SockAddr<Traits> *addr) {
    auto tmp_fd = [_raw, addr] {
      if (addr == nullptr) {
        return ::accept4(_raw, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
      } else {
        auto [sockaddr, addrlen] = addr->raw();
        return ::accept4(_raw, &sockaddr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
  Expected<void, errc> reuse_addr(this auto &&self, bool reuse = true) noexcept {
    int opt = reuse ? 1 : 0;
    ENSEC(setsockopt(self.raw(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0);
    return {};
  }
};

// @brief Compose a socket with multiple flags
// @note This is a convenience type alias for Socket with multiple flags.
template <class... Flags>
using SocketCompose = Socket<SocketTraits<Flags...>>;

template <class Traits>
Expected<Socket<Traits>, errc> socket(const SockAddr<Traits> &sa,
                                      SocketAttribute attr
                                      = SocketAttribute::NonBlocking
                                        | SocketAttribute::CloseOnExec) noexcept {
  int fd = ::socket(sa.family(), sa.type() | static_cast<int>(attr), sa.protocol());
  ENSEC(fd != -1);
  return Socket<Traits>(fd);
}

XSL_SYS_NET_NE
#endif
