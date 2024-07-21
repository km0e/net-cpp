#pragma once
#ifndef XSL_NET_TRANSPORT_RESOLVE
#  define XSL_NET_TRANSPORT_RESOLVE
#  include "xsl/feature.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/endpoint.h"

#  include <netdb.h>
#  include <sys/socket.h>

#  include <cstdint>
#  include <cstring>
#  include <expected>
#  include <string>
#  include <system_error>
#  include <utility>

SYS_NET_NB
namespace impl {
  class ResolveCategory : public std::error_category {
  public:
    ~ResolveCategory() = default;

    const char *name() const noexcept override { return "resolve"; }
    std::error_condition default_error_condition(int ev) const noexcept override {
      return ev == 0 ? std::error_condition{} : std::error_condition{ev, *this};
    }
    bool equivalent(int code, const std::error_condition &condition) const noexcept override {
      return condition.category() == *this && condition.value() == code;
    }
    bool equivalent(const std::error_code &code, int condition) const noexcept override {
      return code.category() == *this && code.value() == condition;
    }
    std::string message(int ev) const override { return gai_strerror(ev); }
  };

}  // namespace impl

enum class ResolveFlag : int {
  V4MAPPED = AI_V4MAPPED,
  ALL = AI_ALL,
  ADDRCONFIG = AI_ADDRCONFIG,
  CANONNAME = AI_CANONNAME,
  NUMERICHOST = AI_NUMERICHOST,
  NUMERICSERV = AI_NUMERICSERV,
  PASSIVE = AI_PASSIVE,
};

ResolveFlag operator|(ResolveFlag lhs, ResolveFlag rhs);

const ResolveFlag SERVER_FLAGS = ResolveFlag::ADDRCONFIG | ResolveFlag::PASSIVE;
const ResolveFlag CLIENT_FLAGS = ResolveFlag::ADDRCONFIG;

using ResolveResult = std::expected<EndpointSet, std::error_condition>;

namespace impl {

  template <class IpV, class Proto>
  class _ParamTraits {
  public:
    static consteval int family() {
      if constexpr (std::is_same_v<IpV, feature::placeholder>) {
        return AF_UNSPEC;
      } else if constexpr (std::is_same_v<IpV, feature::Ip<4>>) {
        return AF_INET;
      } else if constexpr (std::is_same_v<IpV, feature::Ip<6>>) {
        return AF_INET6;
      } else {
        return AF_UNSPEC;
      }
    }
    static consteval std::pair<int, int> params() {
      if constexpr (std::is_same_v<Proto, feature::placeholder>) {
        return {0, 0};
      } else if constexpr (std::is_same_v<Proto, feature::Tcp>) {
        return {SOCK_STREAM, IPPROTO_TCP};
      } else if constexpr (std::is_same_v<Proto, feature::Udp>) {
        return {SOCK_DGRAM, IPPROTO_UDP};
      } else {
        return {0, 0};
      }
    }
  };
  template <class... Flags>
  using ParamTraits = feature::origanize_feature_flags_t<
      _ParamTraits<feature::set<feature::Ip<4>, feature::Ip<6>>, feature::placeholder>, Flags...>;

  template <class Traits>
  ResolveResult resolve(const char *name, const char *serv, ResolveFlag flags) {
    addrinfo hints;
    addrinfo *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_flags = static_cast<int>(flags);
    hints.ai_family = Traits::family();
    auto [type, protocol] = Traits::params();
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    int ret = getaddrinfo(name, serv, &hints, &res);
    if (ret != 0) {
      return std::unexpected{std::error_condition{ret, ResolveCategory()}};
    }
    return {EndpointSet(res)};
  }

  struct Resolver {
    /**
     @brief Resolve the name and service to an address, typically used for connect

     @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp
     @param name The name of the host
     @param serv The service name or port number
     @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG
     @return ResolveResult
     */
    template <class... Flags>
    ResolveResult resolve(const char *name, const char *serv,
                          ResolveFlag flags = ResolveFlag::ADDRCONFIG) {
      return impl::resolve<ParamTraits<Flags...>>(name, serv, flags);
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
    ResolveResult resolve(const char *name, int serv, ResolveFlag flags = ResolveFlag::ADDRCONFIG) {
      char serv_str[6];
      std::snprintf(serv_str, sizeof(serv_str), "%d", serv);
      return impl::resolve<ParamTraits<Flags...>>(name, serv_str, flags);
    }
    /**
     @brief Resolve the service to an address, typically used for bind

     @param serv The service name or port number
     @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG | AI_PASSIVE
     @return ResolveResult
     */
    template <class... Flags>
    ResolveResult resolve(const char *serv,
                          ResolveFlag flags = ResolveFlag::ADDRCONFIG | ResolveFlag::PASSIVE) {
      return impl::resolve<ParamTraits<Flags...>>(nullptr, serv, flags);
    }
    /**
     @brief Resolve the service to an address, typically used for bind

     @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp
     @param port The port number
     @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG | AI_PASSIVE
     @return ResolveResult
     */
    template <class... Flags>
    ResolveResult resolve(uint16_t port,
                          ResolveFlag flags = ResolveFlag::ADDRCONFIG | ResolveFlag::PASSIVE) {
      char port_str[6];
      std::snprintf(port_str, sizeof(port_str), "%u", port);
      return impl::resolve<ParamTraits<Flags...>>(nullptr, port_str, flags);
    }
  };
}  // namespace impl
using Resolver = impl::Resolver;
SYS_NET_NE
#endif
