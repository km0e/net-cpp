#pragma once
#ifndef XSL_NET_TRANSPORT_RESOLVE
#  define XSL_NET_TRANSPORT_RESOLVE
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/endpoint.h"

#  include <netdb.h>
#  include <sys/socket.h>

#  include <cstdint>
#  include <cstring>
#  include <expected>
#  include <string>
#  include <system_error>

XSL_SYS_NET_NB
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
  ZERO = 0,
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

template <class Tag>
using ResolveResult = std::expected<EndpointSet<Tag>, std::error_condition>;

namespace impl {

  template <class Traits>
  ResolveResult<Traits> resolve(const char *name, const char *serv, ResolveFlag flags) {
    addrinfo hints;
    addrinfo *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_flags = static_cast<int>(flags);
    hints.ai_family = Traits::family;
    hints.ai_socktype = Traits::type;
    hints.ai_protocol = Traits::protocol;
    int ret = getaddrinfo(name, serv, &hints, &res);
    if (ret != 0) {
      return std::unexpected{std::error_condition{ret, ResolveCategory()}};
    }
    return {EndpointSet<Traits>(res)};
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
    decltype(auto) resolve(const char *name, const char *serv,
                           ResolveFlag flags = ResolveFlag::ADDRCONFIG) {
      return impl::resolve<SocketTraits<Flags...>>(name, serv, flags);
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
    decltype(auto) resolve(const char *name, int serv,
                           ResolveFlag flags = ResolveFlag::ADDRCONFIG) {
      char serv_str[6];
      std::snprintf(serv_str, sizeof(serv_str), "%d", serv);
      return impl::resolve<SocketTraits<Flags...>>(name, serv_str, flags);
    }
    /**
     @brief Resolve the service to an address, typically used for bind

     @param serv The service name or port number
     @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG | AI_PASSIVE
     @return ResolveResult
     */
    template <class... Flags>
    decltype(auto) resolve(const char *serv,
                           ResolveFlag flags = ResolveFlag::ADDRCONFIG | ResolveFlag::PASSIVE) {
      return impl::resolve<SocketTraits<Flags...>>(nullptr, serv, flags);
    }
    /**
     @brief Resolve the service to an address, typically used for bind

     @tparam Flags The flags for getaddrinfo, should be Ip<4>/Ip<6>, Tcp/Udp
     @param port The port number
     @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG | AI_PASSIVE
     @return ResolveResult
     */
    template <class... Flags>
    decltype(auto) resolve(uint16_t port,
                           ResolveFlag flags = ResolveFlag::ADDRCONFIG | ResolveFlag::PASSIVE) {
      char port_str[6];
      std::snprintf(port_str, sizeof(port_str), "%u", port);
      return impl::resolve<SocketTraits<Flags...>>(nullptr, port_str, flags);
    }
  };
}  // namespace impl
using Resolver = impl::Resolver;
XSL_SYS_NET_NE
#endif
