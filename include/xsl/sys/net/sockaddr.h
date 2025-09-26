/**
 * @file sockaddr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket address handling
 * @version 0.2.1
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
XSL_SYS_NET_NB
/**
 * @brief parse a socket address
 *
 * @param storage socket address storage
 * @param ip ip address, "xxx.xxx.xxx.xxx" for IPv4, "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx" for
 * IPv6
 * @param port port number
 * @return errc
 * @see https://man7.org/linux/man-pages/man3/inet_ntop.3.html
 */
[[nodiscard]]
auto inet4_n2p(const sockaddr_in& storage, std::string& ip, inet::port_t& port) -> errc;
/**
 * @brief parse a socket address
 *
 * @param storage socket address storage
 * @param ip ip address, "xxx.xxx.xxx.xxx"
 * @param port port number
 * @return errc
 * @see https://man7.org/linux/man-pages/man3/inet_ntop.3.html
 */
[[nodiscard]]
auto inet6_n2p(const sockaddr_in6& storage, std::string& ip, inet::port_t& port) -> errc;
/**
 * @brief parse a socket address
 *
 * @param storage socket address storage
 * @param ip ip address, "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx"
 * @param port port number
 * @return errc
 * @see https://man7.org/linux/man-pages/man3/inet_ntop.3.html
 */
[[nodiscard]]
auto inet_n2p(const sockaddr_storage& storage, std::string& ip, inet::port_t& port) -> errc;

[[nodiscard]]
auto inet4_n2p(const sockaddr_in& storage, std::string& addr) -> errc;
[[nodiscard]]
auto inet6_n2p(const sockaddr_storage& storage, std::string& addr) -> errc;
[[nodiscard]]
auto inet_n2p(const sockaddr_storage& storage, std::string& addr) -> errc;
[[nodiscard]] inline auto inet4_n2p(const sockaddr_in& storage) -> Expected<std::string, errc> {
  std::string addr;
  ENSEC(inet4_n2p(storage, addr));
  return addr;
}
[[nodiscard]] inline auto inet6_n2p(const sockaddr_storage& storage)
    -> Expected<std::string, errc> {
  std::string addr;
  ENSEC(inet6_n2p(storage, addr));
  return addr;
}
[[nodiscard]]
inline auto inet_n2p(const sockaddr_storage& storage) -> Expected<std::string, errc> {
  std::string addr;
  ENSEC(inet_n2p(storage, addr));
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
  template <std::invocable<sockaddr_storage*, socklen_t&> F>
  constexpr SockAddr(F&& f) noexcept {
#  pragma GCC diagnostic pop
    f(&_addr, _addrlen);
  }
  constexpr SockAddr(SockAddr&&) noexcept = default;
  constexpr SockAddr& operator=(SockAddr&&) noexcept = default;
  constexpr int family() const noexcept {
    if constexpr (requires { Traits{}.family(); }) {
      return Traits{}.family();
    } else {
      return this->_addr.ss_family;
    }
  }
  constexpr bool operator==(this auto&& self, const SockAddr& ano) noexcept {
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
  constexpr errc parse(this auto&& self, std::string& ip, uint16_t& port) {
    return inet_n2p(self._addr, ip, port);
  }
  inline auto addr(this auto&& self) noexcept -> like_t<decltype(self), sockaddr> {
    return reinterpret_cast<like_t<decltype(self), sockaddr>>(self._addr);
  }
  inline auto len(this auto&& self) noexcept -> like_t<decltype(self), socklen_t> {
    return self._addrlen;
  }
  /**
   * @brief get the string representation of the address
   * @note format: "ip:port"
   * @param self this, some derived class
   * @return std::string
   */
  constexpr errc to_string(this auto&& self, std::string& addr) {
    return inet_n2p(self._addr, addr);
  }

  sockaddr_storage _addr;  ///< address storage
  socklen_t _addrlen;      ///< address length
};

template <class... Flags>
using SockAddrCompose = SockAddr<SocketTraits<Flags...>>;

template <class Traits>
  requires(Traits::family() == AF_INET)
Expected<SockAddr<Traits>, errc> make_sockaddr(const char* ip, inet::port_t port) noexcept {
  errc ec{};
  SockAddr<Traits> addr([&](sockaddr_storage* addr, socklen_t& addrlen) {
    if (sockaddr_in* addr4 = reinterpret_cast<sockaddr_in*>(addr);
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
Expected<SockAddr<Traits>, errc> make_sockaddr(const char* ip, inet::port_t port) noexcept {
  errc ec{};
  SockAddr<Traits> addr([&](sockaddr_storage* addr, socklen_t& addrlen) {
    if (sockaddr_in6* addr6 = reinterpret_cast<sockaddr_in6*>(addr);
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
Expected<SockAddr<Traits>, errc> make_sockaddr(const char* ip, inet::port_t port) noexcept {
  errc ec{};
  SockAddr<Traits> addr([&](sockaddr_storage* addr, socklen_t& addrlen) {
    if (sockaddr_in6* addr6 = reinterpret_cast<sockaddr_in6*>(addr);
        inet_pton(AF_INET6, ip, &addr6->sin6_addr) == 1) {
      addr6->sin6_family = AF_INET6;
      addr6->sin6_port = htons(port);
      addrlen = sizeof(sockaddr_in6);
    } else if (sockaddr_in* addr4 = reinterpret_cast<sockaddr_in*>(addr);
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
decltype(auto) make_sockaddr(const char* ip, const char* port) noexcept {
  return make_sockaddr<SocketTraits<Flags...>>(ip, static_cast<inet::port_t>(std::atoi(port)));
}
template <class... Flags>
decltype(auto) make_sockaddr(const std::string_view& ip, const char* port) noexcept {
  return make_sockaddr<SocketTraits<Flags...>>(ip.data(),
                                               static_cast<inet::port_t>(std::atoi(port)));
}
template <class... Flags>
decltype(auto) make_sockaddr(const std::string_view& ip, inet::port_t port) noexcept {
  return make_sockaddr<SocketTraits<Flags...>>(ip.data(), port);
}

template <class... Flags>
decltype(auto) make_sockaddr(const std::string_view& ip, const std::string_view& port) noexcept {
  return make_sockaddr<SocketTraits<Flags...>>(ip.data(), port.data());
}
template <class... Flags>
decltype(auto) make_sockaddr() noexcept {
  return SockAddr<SocketTraits<Flags...>>();
}
XSL_SYS_NET_NE
#endif
