/**
 * @file dns.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS utilities
 * @version 0.1
 * @date 2024-09-10
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_DNS
#  define XSL_SYS_NET_DNS
#  include "xsl/sys/net/def.h"

#  include <netdb.h>

#  include <cstddef>
#  include <cstdio>
#  include <cstring>
#  include <iterator>
#  include <system_error>
#  include <utility>
XSL_SYS_NET_NB
/// @brief The traits for the socket
template <class Traits>
class AddrInfos {
  class Iterator {
  public:
    using value_type = addrinfo;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;

    constexpr Iterator() : ai(nullptr) {}
    constexpr Iterator(addrinfo *info) : ai(info) {}
    constexpr Iterator(const Iterator &) = default;
    constexpr Iterator &operator=(const Iterator &) = default;
    constexpr ~Iterator() = default;

    constexpr auto &&operator*(this auto &&self) { return *std::forward<decltype(self)>(self).ai; }

    constexpr auto operator->(this auto &&self) {
      return &std::forward_like<decltype(self)>(self)._ep;
    }

    constexpr Iterator &operator++() {
      ai = ai->ai_next;
      return *this;
    }

    constexpr Iterator operator++(int) {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr bool operator!=(const Iterator &rhs) const { return ai != rhs.ai; }
    constexpr bool operator==(const Iterator &rhs) const { return ai == rhs.ai; }

  private:
    pointer ai;
  };

  static_assert(std::forward_iterator<Iterator>);

public:
  constexpr AddrInfos(addrinfo *info) : info(info) {}
  constexpr AddrInfos(AddrInfos &&other) : info(std::exchange(other.info, nullptr)) {}
  constexpr AddrInfos &operator=(AddrInfos &&other) {
    if (this != &other) {
      this->~AddrInfos();
      new (this) AddrInfos(std::exchange(other.info, nullptr));
    }
    return *this;
  }
  constexpr AddrInfos(const AddrInfos &) = delete;
  constexpr AddrInfos &operator=(const AddrInfos &) = delete;
  constexpr ~AddrInfos() {
    if (info) freeaddrinfo(info);
  }

  constexpr Iterator begin() const { return Iterator(info); }
  constexpr Iterator end() const { return Iterator(nullptr); }

  addrinfo *info;
};

namespace {
  class ResolveCategory : public std::error_category {
  public:
    ~ResolveCategory() = default;

    constexpr const char *name() const noexcept override { return "resolve"; }
    std::error_condition default_error_condition(int ev) const noexcept override {
      return ev == 0 ? std::error_condition{} : std::error_condition{ev, *this};
    }
    constexpr bool equivalent(int code,
                              const std::error_condition &condition) const noexcept override {
      return condition.category() == *this && condition.value() == code;
    }
    constexpr bool equivalent(const std::error_code &code, int condition) const noexcept override {
      return code.category() == *this && code.value() == condition;
    }
    constexpr std::string message(int ev) const override { return gai_strerror(ev); }
  };

}  // namespace

enum class ResolveFlag : int {
  ZERO = 0,
  V4MAPPED = AI_V4MAPPED,
  ALL = AI_ALL,
  ADDRCONFIG = AI_ADDRCONFIG,
  CANONNAME = AI_CANONNAME,
  NUMERICHOST = AI_NUMERICHOST,
  NUMERICSERV = AI_NUMERICSERV,
  PASSIVE = AI_PASSIVE,
};

constexpr ResolveFlag operator|(ResolveFlag lhs, ResolveFlag rhs) {
  return static_cast<ResolveFlag>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

const ResolveFlag SERVER_FLAGS = ResolveFlag::ADDRCONFIG | ResolveFlag::PASSIVE;
const ResolveFlag CLIENT_FLAGS = ResolveFlag::ADDRCONFIG;

template <class Traits>
using ResolveResult = std::expected<AddrInfos<Traits>, std::error_condition>;

namespace {

  template <class Traits>
  constexpr ResolveResult<Traits> resolve(const char *name, const char *serv, ResolveFlag flags) {
    addrinfo hints;
    addrinfo *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_flags = static_cast<int>(flags);
    hints.ai_family = Traits{}.family();
    hints.ai_socktype = Traits{}.type();
    hints.ai_protocol = Traits{}.protocol();
    int ret = getaddrinfo(name, serv, &hints, &res);
    if (ret != 0) {
      return std::unexpected{std::error_condition{ret, ResolveCategory()}};
    }
    return {AddrInfos<Traits>(res)};
  }

}  // namespace

/**
 @brief Resolve the name and service to an address, typically used for connect

 @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp
 @param name The name of the host
 @param serv The service name or port number
 @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG
 @return ResolveResult
 */
template <class... Flags>
constexpr decltype(auto) getaddrinfo(const char *name, const char *serv,
                                     ResolveFlag flags = ResolveFlag::ADDRCONFIG) {
  return resolve<SocketTraits<Flags...>>(name, serv, flags);
}
/**
 @brief Resolve the name and service to an address, typically used for connect

 @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp
 @param name The name of the host
 @param serv The service name or port number
 @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG
 @return ResolveResult
 */
template <class... Flags>
constexpr decltype(auto) getaddrinfo(const char *name, int serv,
                                     ResolveFlag flags = ResolveFlag::ADDRCONFIG) {
  char serv_str[6];
  std::snprintf(serv_str, sizeof(serv_str), "%d", serv);
  return resolve<SocketTraits<Flags...>>(name, serv_str, flags);
}
/**
 @brief Resolve the service to an address, typically used for bind

 @param serv The service name or port number
 @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG | AI_PASSIVE
 @return ResolveResult
 */
template <class... Flags>
constexpr decltype(auto) getaddrinfo(const char *serv, ResolveFlag flags = ResolveFlag::ADDRCONFIG
                                                                           | ResolveFlag::PASSIVE) {
  return resolve<SocketTraits<Flags...>>(nullptr, serv, flags);
}
/**
 @brief Resolve the service to an address, typically used for bind

 @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp
 @param port The port number
 @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG | AI_PASSIVE
 @return ResolveResult
 */
template <class... Flags>
constexpr decltype(auto) getaddrinfo(uint16_t port, ResolveFlag flags = ResolveFlag::ADDRCONFIG
                                                                        | ResolveFlag::PASSIVE) {
  char port_str[6];
  std::snprintf(port_str, sizeof(port_str), "%u", port);
  return resolve<SocketTraits<Flags...>>(nullptr, port_str, flags);
};

XSL_SYS_NET_NE
#endif
