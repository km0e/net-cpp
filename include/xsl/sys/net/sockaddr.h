/**
 * @file sockaddr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Socket address handling
 * @version 0.1.1
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
#  include <xsl/sys/net/def.h>

#  include <cstdlib>
#  include <cstring>
#  include <utility>

XSL_SYS_NET_NB
/// @brief Socket address storage
class SockAddrStorage {
public:
  constexpr SockAddrStorage(sa_family_t family) : _addr{}, _addrlen(sizeof(_addr)) {
    _addr.ss_family = family;
  }
  constexpr SockAddrStorage() : SockAddrStorage(AF_UNSPEC) {}
  constexpr SockAddrStorage(const SockAddrStorage &other) = default;  ///< copy constructor
  constexpr SockAddrStorage(SockAddrStorage &&other) = default;       ///< move constructor
  constexpr SockAddrStorage &operator=(const SockAddrStorage &other)
      = default;                                                            ///< copy assignment
  constexpr SockAddrStorage &operator=(SockAddrStorage &&other) = default;  ///< move assignment
  /**
   * @brief Construct a new Sock Addr Storage object from raw address and length
   *
   * @param addr raw address
   * @param addrlen address length
   */
  inline SockAddrStorage(sockaddr *addr, socklen_t addrlen)
      : _addr(reinterpret_cast<sockaddr_storage &>(*addr)), _addrlen(addrlen) {}

  /**
   * @brief compare address
   *
   * @param other other address
   * @return true if equal
   * @return false if not equal
   */
  constexpr bool operator==(const SockAddrStorage &other) const {
    return this->_addrlen == other._addrlen && std::memcmp(&_addr, &other._addr, _addrlen) == 0;
  }
  /**
   * @brief reset address
   *
   */
  inline void reset(this auto &&self) {
    auto &_addr = self.SockAddrStorage::_addr;
    self.SockAddrStorage::_addrlen = sizeof(_addr);
    std::memset(&_addr, 0, sizeof(_addr));
    _addr.ss_family = self.family();
  }
  /**
   * @brief raw address
   *
   * @return std::pair<sockaddr *, socklen_t *>
   */
  inline auto raw(this auto &&self)
      -> std::pair<like_t<decltype(self), sockaddr>, like_t<decltype(self), socklen_t>> {
    return {reinterpret_cast<like_t<decltype(self), sockaddr>>(self.SockAddrStorage::_addr),
            self.SockAddrStorage::_addrlen};
  }

protected:
  sockaddr_storage _addr;  ///< address storage
  socklen_t _addrlen;      ///< address length
};

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
class SockAddr : public Traits, public SockAddrStorage {
  using Base = SockAddrStorage;

public:
  using traits_type = Traits;          ///< traits type
  using sockaddr_type = sockaddr_in6;  ///< sockaddr type
  using Base::Base;
  SockAddr() : Base() {}
  constexpr int family() const {
    if constexpr (requires { Traits{}.family(); }) {
      return Traits{}.family();
    } else {
      return this->SockAddrStorage::_addr.ss_family;
    }
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
    auto &storage = self.SockAddrStorage::_addr;
    ip.resize(INET6_ADDRSTRLEN);
    sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&storage);
    if (inet_ntop(AF_INET6, &addr->sin6_addr, ip.data(), INET6_ADDRSTRLEN)) {
      ip.resize(std::strlen(ip.data()));
      port = ntohs(addr->sin6_port);
    } else {
      return errc{errno};
    }
    return {};
  }

  /**
   * @brief get the string representation of the address
   * @note format: "ip:port"
   * @param self this, some derived class
   * @return std::string
   */
  constexpr void to_string(this auto &&self, std::string &addr) {
    inet_to_string(self.SockAddrStorage::_addr, addr);
  }

protected:
  using Base::_addr;
  using Base::_addrlen;
};

template <class Traits>
  requires(Traits::family() == AF_INET)
class SockAddr<Traits> : public Traits, public SockAddrStorage {
  using Base = SockAddrStorage;

public:
  using traits_type = Traits;
  using sockaddr_type = sockaddr_in;
  using Base::Base;
  using Base::operator==;
  SockAddr() : Base(AF_INET) {}
  /**
   * @brief Construct a new Sock Addr object from ip and port
   *
   * @param ip ip address
   * @param port port number
   */
  constexpr SockAddr(const std::string_view &ip, uint16_t port) : Base() {
    sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(&_addr);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_pton(AF_INET, ip.data(), &addr->sin_addr) == 1) {
      _addrlen = sizeof(sockaddr_in);
    }
  }
  constexpr SockAddr(const char *ip, uint16_t port) : SockAddr(std::string_view(ip), port) {}
  constexpr SockAddr(const std::string &ip, uint16_t port) : SockAddr(std::string_view(ip), port) {}
  constexpr SockAddr(const std::string &ip, const std::string &port)
      : SockAddr(std::string_view(ip), std::stoi(port)) {}
  /**
   * @brief parse the address
   *
   * @param self this, some derived class
   * @param ip ip address
   * @param port port number
   * @return errc
   */
  constexpr errc parse(this auto &&self, std::string &ip, uint16_t &port) {
    auto &storage = self.SockAddrStorage::_addr;
    ip.resize(INET_ADDRSTRLEN);
    sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(&storage);
    if (inet_ntop(AF_INET, &addr->sin_addr, ip.data(), INET_ADDRSTRLEN)) {
      ip.resize(std::strlen(ip.data()));
      port = ntohs(addr->sin_port);
    } else {
      return errc{errno};
    }
    return {};
  }
  /**
   * @brief get the string representation of the address
   * @note format: "ip:port"
   * @param self this, some derived class
   * @return std::string
   */
  constexpr std::string to_string(this auto &&self) {
    auto &storage = self.SockAddrStorage::_addr;
    std::string addr;
    addr.resize(INET_ADDRSTRLEN);
    sockaddr_in *sa = reinterpret_cast<sockaddr_in *>(&storage);
    if (inet_ntop(AF_INET, &sa->sin_addr, addr.data(), INET_ADDRSTRLEN)) {
      addr.resize(std::strlen(addr.data()) + 1);
      addr.back() = ':';  // add a colon for port
      addr += std::to_string(ntohs(sa->sin_port));
    } else {
      addr.clear();  // clear if error
    }
    return addr;
  }

protected:
  using Base::_addr;
  using Base::_addrlen;
};
/// @brief compose a socket address
template <class Traits>
  requires(Traits::family() == AF_INET6)
class SockAddr<Traits> : public Traits, public SockAddrStorage {
  using Base = SockAddrStorage;

public:
  using traits_type = Traits;          ///< traits type
  using sockaddr_type = sockaddr_in6;  ///< sockaddr type
  using Base::Base;
  SockAddr() : Base(AF_INET6) {}
  /**
   * @brief Construct a new Sock Addr object from ip and port
   *
   * @param ip ip address
   * @param port port number
   */
  constexpr SockAddr(const std::string_view &ip, uint16_t port) : Base() {
    sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&_addr);
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip.data(), &addr->sin6_addr) == 1) {
      _addrlen = sizeof(sockaddr_in6);
    }
  }
  constexpr SockAddr(const char *ip, uint16_t port) : SockAddr(std::string_view(ip), port) {}
  constexpr SockAddr(const std::string &ip, uint16_t port) : SockAddr(std::string_view(ip), port) {}
  constexpr SockAddr(const std::string &ip, const std::string &port)
      : SockAddr(std::string_view(ip), std::stoi(port)) {}
  /**
   * @brief raw address
   *
   * @param self this, some derived class
   * @param ip ip address
   * @param port port number
   * @return errc
   */
  constexpr errc parse(this auto &&self, std::string &ip, uint16_t &port) {
    auto &storage = self.SockAddrStorage::_addr;
    ip.resize(INET6_ADDRSTRLEN);
    sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&storage);
    if (inet_ntop(AF_INET6, &addr->sin6_addr, ip.data(), INET6_ADDRSTRLEN)) {
      ip.resize(std::strlen(ip.data()));
      port = ntohs(addr->sin6_port);
    } else {
      return errc{errno};
    }
    return {};
  }

protected:
  using Base::_addr;
  using Base::_addrlen;
};

template <class... Flags>
using SockAddrCompose = SockAddr<SocketTraits<Flags...>>;

template <class... Flags>
std::expected<SockAddrCompose<Flags...>, errc> make_sockaddr(const char *ip, uint16_t port) {
  SockAddrCompose<Flags...> _addr{};
  auto [_addr_ptr, _addrlen] = _addr.raw();
  if (sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&_addr_ptr);
      inet_pton(AF_INET6, ip, &addr->sin6_addr) == 1) {
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    _addrlen = sizeof(sockaddr_in6);
    return std::expected<SockAddrCompose<Flags...>, errc>{std::in_place, std::move(_addr)};
  }
  if (sockaddr_in *addr4 = reinterpret_cast<sockaddr_in *>(&_addr_ptr);
      inet_pton(AF_INET, ip, &addr4->sin_addr) == 1) {
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(port);
    _addrlen = sizeof(sockaddr_in);
    return std::expected<SockAddrCompose<Flags...>, errc>{std::in_place, std::move(_addr)};
  }
  return std::unexpected{errc{errno}};
}

template <class... Flags>
decltype(auto) make_sockaddr(const char *ip, const char *port) {
  return make_sockaddr<Flags...>(ip, static_cast<uint16_t>(std::atoi(port)));
}

XSL_SYS_NET_NE
#endif
