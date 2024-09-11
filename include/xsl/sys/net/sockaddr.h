/**
 * @file sockaddr.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_SOCKADDR
#  define XSL_SYS_NET_SOCKADDR
#  include "xsl/sys/net/def.h"

#  include <arpa/inet.h>
#  include <sys/socket.h>
#  include <unistd.h>

#  include <cstring>
#  include <expected>
#  include <system_error>
#  include <type_traits>
#  include <utility>

XSL_SYS_NET_NB

class SockAddrStorage {
public:
  /**
   * @brief Construct a new Sock Addr Storage object
   *
   */
  constexpr SockAddrStorage() : _addr{}, _addrlen(0) {}
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
  constexpr operator bool() const { return _addrlen != 0; }
  /**
   * @brief reset address
   *
   */
  constexpr void reset() {
    _addrlen = 0;
    std::memset(&_addr, 0, sizeof(_addr));
  }
  /**
   * @brief raw address
   *
   * @return std::pair<sockaddr *, socklen_t *>
   */
  inline auto raw(this auto &&self)
      -> std::pair<std::remove_reference_t<like_t<decltype(self), sockaddr>>,
                   like_t<decltype(self), socklen_t>> {
    return {reinterpret_cast<std::remove_reference_t<like_t<decltype(self), sockaddr>> &>(
                self.SockAddrStorage::_addr),
            self.SockAddrStorage::_addrlen};
  }
  /**
   * @brief raw address
   *
   * @param self this, some derived class
   * @param ip ip address
   * @param port port number
   * @return std::expected<void, std::errc>
   */
  constexpr std::expected<void, std::errc> parse(this auto &&self, std::string &ip,
                                                 uint16_t &port) {
    using traits_type = std::remove_cvref_t<decltype(self)>::traits_type;
    static_assert(traits_type::family == AF_INET || traits_type::family == AF_INET6);
    auto &storage = self.SockAddrStorage::_addr;
    if constexpr (traits_type::family == AF_INET) {
      ip.resize(INET_ADDRSTRLEN);
      sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(&storage);
      if (inet_ntop(AF_INET, &addr->sin_addr, ip.data(), INET_ADDRSTRLEN)) {
        ip.resize(std::strlen(ip.data()));
        port = ntohs(addr->sin_port);
      } else {
        return std::unexpected(std::errc{errno});
      }
    } else if constexpr (traits_type::family == AF_INET6) {
      ip.resize(INET6_ADDRSTRLEN);
      sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&storage);
      if (inet_ntop(AF_INET6, &addr->sin6_addr, ip.data(), INET6_ADDRSTRLEN)) {
        ip.resize(std::strlen(ip.data()));
        port = ntohs(addr->sin6_port);
      } else {
        return std::unexpected(std::errc{errno});
      }
    } else {
      return std::unexpected(std::errc::address_family_not_supported);
    }
    return {};
  }

protected:
  sockaddr_storage _addr;  ///< address storage
  socklen_t _addrlen;      ///< address length
};
template <class Traits>
class SockAddr;

template <class Traits>
  requires(Traits::family() == AF_INET)
class SockAddr<Traits> : public SockAddrStorage {
  using Base = SockAddrStorage;

public:
  using traits_type = Traits;
  using sockaddr_type = sockaddr_in;
  using Base::Base;
  /**
   * @brief Construct a new Sock Addr object from ip and port
   *
   * @param ip ip address
   * @param port port number
   */
  constexpr SockAddr(std::string_view ip, uint16_t port) : Base() {
    sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(&_addr);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_pton(AF_INET, ip.data(), &addr->sin_addr) == 1) {
      _addrlen = sizeof(sockaddr_in);
    }
  }
  /**
   * @brief parse the address
   *
   * @param self this, some derived class
   * @param ip ip address
   * @param port port number
   * @return constexpr std::errc
   */
  constexpr std::errc parse(this auto &&self, std::string &ip, uint16_t &port) {
    auto &storage = self.SockAddrStorage::_addr;
    ip.resize(INET_ADDRSTRLEN);
    sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(&storage);
    if (inet_ntop(AF_INET, &addr->sin_addr, ip.data(), INET_ADDRSTRLEN)) {
      ip.resize(std::strlen(ip.data()));
      port = ntohs(addr->sin_port);
    } else {
      return std::errc{errno};
    }
    return {};
  }

protected:
  using Base::_addr;
  using Base::_addrlen;
};
/// @brief compose a socket address
template <class Traits>
  requires(Traits::family() == AF_INET6)
class SockAddr<Traits> : public SockAddrStorage {
  using Base = SockAddrStorage;

public:
  using traits_type = Traits;          ///< traits type
  using sockaddr_type = sockaddr_in6;  ///< sockaddr type
  using Base::Base;

  /**
   * @brief Construct a new Sock Addr object from ip and port
   *
   * @param ip ip address
   * @param port port number
   */
  constexpr SockAddr(std::string_view ip, uint16_t port) : Base() {
    sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&_addr);
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip.data(), &addr->sin6_addr) == 1) {
      _addrlen = sizeof(sockaddr_in6);
    }
  }
  /**
   * @brief raw address
   *
   * @param self this, some derived class
   * @param ip ip address
   * @param port port number
   * @return std::expected<void, std::errc>
   */
  constexpr std::errc parse(this auto &&self, std::string &ip, uint16_t &port) {
    auto &storage = self.SockAddrStorage::_addr;
    ip.resize(INET6_ADDRSTRLEN);
    sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&storage);
    if (inet_ntop(AF_INET6, &addr->sin6_addr, ip.data(), INET6_ADDRSTRLEN)) {
      ip.resize(std::strlen(ip.data()));
      port = ntohs(addr->sin6_port);
    } else {
      return std::errc{errno};
    }
    return {};
  }

protected:
  using Base::_addr;
  using Base::_addrlen;
};

template <class... Flags>
using SockAddrCompose = SockAddr<SocketTraits<Flags...>>;

XSL_SYS_NET_NE
#endif
