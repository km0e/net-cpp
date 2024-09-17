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
#  include "xsl/sys/net/socket.h"
#  include "xsl/sys/raw.h"
#  include "xsl/sys/sync.h"

#  include <netdb.h>

#  include <cerrno>
#  include <cstddef>
#  include <cstdio>
#  include <cstring>
#  include <iterator>
#  include <system_error>
#  include <utility>
XSL_SYS_NET_NB

/// @brief Address information
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
constexpr decltype(auto) getaddrinfo(const uint16_t port,
                                     ResolveFlag flags
                                     = ResolveFlag::ADDRCONFIG | ResolveFlag::PASSIVE) {
  char port_str[6];
  std::snprintf(port_str, sizeof(port_str), "%u", port);
  return resolve<SocketTraits<Flags...>>(nullptr, port_str, flags);
};

/**
 * @brief Connect to a remote address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param args The arguments for getaddrinfo
 * @return std::expected<Socket<Traits>, std::error_condition>
 */
template <class... Flags, ConnectionLessSocketTraits Traits = SocketTraits<Flags...>, class... Args>
constexpr std::expected<Socket<Traits>, std::error_condition> gai_connect(Args &&...args) {
  auto res_resolved = getaddrinfo<Flags...>(std::forward<Args>(args)...);
  if (!res_resolved) {
    return std::unexpected{res_resolved.error()};
  }
  auto skt = Socket<Traits>();
  if (!skt.is_valid()) {
    return std::unexpected{current_ec()};
  }
  int ec = 0;
  for (auto &ai : *res_resolved) {
    ec = filter_interrupt(::connect, skt.raw(), ai.ai_addr, ai.ai_addrlen);
    if (ec == 0) {
      return std::move(skt);
    }
  }
  return std::unexpected{errc{errno}};
}

/**
 * @brief Connect to a remote address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param poller The poller
 * @param args The arguments for getaddrinfo
 * @return Task<std::expected<AsyncSocket<Traits>, std::error_condition>>
 */
template <class... Flags, ConnectionBasedSocketTraits Traits = SocketTraits<Flags...>,
          class... Args>
Task<std::expected<AsyncSocket<Traits>, std::error_condition>> gai_async_connect(Poller &poller,
                                                                                 Args &&...args) {
  auto res_resolved = getaddrinfo<Traits>(std::forward<Args>(args)...);
  if (!res_resolved) {
    co_return std::unexpected{res_resolved.error()};
  }
  net::ReadWriteSocket<Traits> skt{};
  if (!skt.is_valid()) {
    co_return std::unexpected{current_ec()};
  }
  LOG5("Created fd: {}", skt.raw());
  errc ec;
  for (auto &ai : *res_resolved) {
    ec = skt.check_and_upgrade(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    if (ec != errc{}) {
      continue;
    }
    auto res_skt = co_await raw_connect(std::move(skt), *ai.ai_addr, ai.ai_addrlen, poller);
    if (res_skt) {
      co_return std::move(*res_skt);
    }
  }
  co_return std::unexpected{errc{ec}};
}
/**
 * @brief Bind to a local address
 *
 * @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp, TcpIpv4 ...
 * @tparam Traits
 * @tparam Args
 * @param args The arguments for getaddrinfo
 * @return std::expected<Socket<Traits>, std::error_condition>
 */
template <class... Flags, class Traits = SocketTraits<Flags...>, class... Args>
std::expected<Socket<Traits>, std::error_condition> gai_bind(Args &&...args) {
  auto res_resolved = getaddrinfo<Flags...>(std::forward<Args>(args)...);
  if (!res_resolved) {
    return std::unexpected{res_resolved.error()};
  }
  auto skt = Socket<Traits>();
  if (!skt.is_valid()) {
    return std::unexpected{current_ec()};
  }
  errc ec{};
  for (auto &ai : *res_resolved) {
    ec = skt.check_and_upgrade(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    LOG5("Set non-blocking to fd: {}", skt.raw());
    int opt = 1;
    if (setsockopt(skt.raw(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      continue;
    }
    LOG5("Set reuse addr to fd: {}", skt.raw());
    if (::bind(skt.raw(), ai.ai_addr, ai.ai_addrlen) == 0) {
      return std::move(skt);
    }
    ec = errc{errno};
  }
  return std::unexpected{std::make_error_condition(ec)};
}

XSL_SYS_NET_NE
#endif
