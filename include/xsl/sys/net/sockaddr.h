/**
 * @file sockaddr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket address handling
 * @version 0.2.0
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_SOCKADDR
#  define XSL_SYS_NET_SOCKADDR
#  include <arpa/inet.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <xsl/error.h>
#  include <xsl/sys/net/def.h>
#  include <xsl/sys/raw.h>

#  include <cstdlib>
#  include <cstring>
#  include <utility>

XSL_SYS_NET_NB

auto inet_to_string(const sockaddr_storage &storage, std::string &addr) -> void;
auto inet4_to_string(const sockaddr_storage &storage, std::string &addr) -> void;
auto inet6_to_string(const sockaddr_storage &storage, std::string &addr) -> void;
inline auto inet4_to_string(const sockaddr_storage &storage) -> std::string {
  std::string addr;
  inet4_to_string(storage, addr);
  return addr;
}
inline auto inet6_to_string(const sockaddr_storage &storage) -> std::string {
  std::string addr;
  inet6_to_string(storage, addr);
  return addr;
}
inline auto inet_to_string(const sockaddr_storage &storage) -> std::string {
  std::string addr;
  inet_to_string(storage, addr);
  return addr;
}
/// @brief compose a socket address

template <class Traits>
struct SockAddr : public Traits {
public:
  using traits_type = Traits;          ///< traits type
  using sockaddr_type = sockaddr_in6;  ///< sockaddr type
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Weffc++"
  constexpr SockAddr() noexcept = default;
  template <std::invocable<sockaddr_storage *, socklen_t &> F>
  constexpr SockAddr(F &&f) noexcept {
#  pragma GCC diagnostic pop
    f(&_addr, _addrlen);
  }
  constexpr SockAddr(SockAddr &&) noexcept = default;
  constexpr SockAddr &operator=(SockAddr &&) noexcept = default;
  constexpr int family() const noexcept {
    if constexpr (requires { Traits{}.family(); }) {
      return Traits{}.family();
    } else {
      return this->_addr.ss_family;
    }
  }
  constexpr bool operator==(this auto &&self, const SockAddr &ano) noexcept {
    if (self.family() != ano.family()) {
      return false;
    }
    if (self._addrlen != ano._addrlen) {
      return false;
    }
    return std::memcmp(&self._addr, &ano._addr, self._addrlen) == 0;
  }
  /**
   * @brief raw address
   *
   * @param self this, some derived class
   * @param ip ip address
   * @param port port number
   * @return errc
   */
  constexpr errc parse(this auto &&self, std::string &ip, uint16_t &port) {
    ip.resize(INET6_ADDRSTRLEN);
    sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&self._addr);
    if (inet_ntop(AF_INET6, &addr->sin6_addr, ip.data(), INET6_ADDRSTRLEN)) {
      ip.resize(std::strlen(ip.data()));
      port = ntohs(addr->sin6_port);
    } else {
      return errc{errno};
    }
    return {};
  }
  /**
   * @brief raw address
   *
   * @return std::pair<sockaddr *, socklen_t *>
   */
  inline auto raw(this auto &&self) noexcept
      -> std::pair<like_t<decltype(self), sockaddr>, like_t<decltype(self), socklen_t>> {
    return {reinterpret_cast<like_t<decltype(self), sockaddr>>(self._addr), self._addrlen};
  }
  /**
   * @brief get the string representation of the address
   * @note format: "ip:port"
   * @param self this, some derived class
   * @return std::string
   */
  constexpr void to_string(this auto &&self, std::string &addr) {
    inet_to_string(self._addr, addr);
  }

  sockaddr_storage _addr;  ///< address storage
  socklen_t _addrlen;      ///< address length
};

template <class... Flags>
using SockAddrCompose = SockAddr<SocketTraits<Flags...>>;

template <class Traits>
  requires(Traits::family() == AF_INET)
Expected<SockAddr<Traits>, errc> make_sockaddr(const char *ip, inet::port_t port) noexcept {
  errc ec{};
  SockAddr<Traits> addr([&](sockaddr_storage *addr, socklen_t &addrlen) {
    if (sockaddr_in *addr4 = reinterpret_cast<sockaddr_in *>(addr);
        inet_pton(AF_INET, ip, &addr4->sin_addr) == 1) {
      addr4->sin_family = AF_INET;
      addr4->sin_port = htons(port);
      addrlen = sizeof(sockaddr_in);
    } else {
      ec = errc{errno};
    }
  });
  ENSEC(ec == errc{}, ec);
  return addr;
}

template <class Traits>
  requires(Traits::family() == AF_INET6)
Expected<SockAddr<Traits>, errc> make_sockaddr(const char *ip, inet::port_t port) noexcept {
  errc ec{};
  SockAddr<Traits> addr([&](sockaddr_storage *addr, socklen_t &addrlen) {
    if (sockaddr_in6 *addr6 = reinterpret_cast<sockaddr_in6 *>(addr);
        inet_pton(AF_INET6, ip, &addr6->sin6_addr) == 1) {
      addr6->sin6_family = AF_INET6;
      addr6->sin6_port = htons(port);
      addrlen = sizeof(sockaddr_in6);
    } else {
      ec = errc{errno};
    }
  });
  ENSEC(ec == errc{}, ec);
  return addr;
}

template <class Traits>
Expected<SockAddr<Traits>, errc> make_sockaddr(const char *ip, inet::port_t port) noexcept {
  errc ec{};
  SockAddr<Traits> addr([&](sockaddr_storage *addr, socklen_t &addrlen) {
    if (sockaddr_in *addr4 = reinterpret_cast<sockaddr_in *>(addr);
        inet_pton(AF_INET, ip, &addr4->sin_addr) == 1) {
      addr4->sin_family = AF_INET;
      addr4->sin_port = htons(port);
      addrlen = sizeof(sockaddr_in);
    } else if (sockaddr_in *addr4 = reinterpret_cast<sockaddr_in *>(addr);
               inet_pton(AF_INET, ip, &addr4->sin_addr) == 1) {
      addr4->sin_family = AF_INET;
      addr4->sin_port = htons(port);
      addrlen = sizeof(sockaddr_in);
    } else {
      ec = errc{errno};
    }
  });
  ENSEC(ec == errc{}, ec);
  return addr;
}

template <class... Flags>
decltype(auto) make_sockaddr(const char *ip, const char *port) noexcept {
  return make_sockaddr<SocketTraits<Flags...>>(ip, static_cast<inet::port_t>(std::atoi(port)));
}
template <class... Flags>
decltype(auto) make_sockaddr(const std::string_view &ip, const char *port) noexcept {
  return make_sockaddr<SocketTraits<Flags...>>(ip.data(),
                                               static_cast<inet::port_t>(std::atoi(port)));
}
template <class... Flags>
decltype(auto) make_sockaddr(const std::string_view &ip, inet::port_t port) noexcept {
  return make_sockaddr<SocketTraits<Flags...>>(ip.data(), port);
}
template <class... Flags>
decltype(auto) make_sockaddr() noexcept {
  return SockAddr<SocketTraits<Flags...>>();
}
XSL_SYS_NET_NE
#endif
