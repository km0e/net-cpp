#pragma once
#ifndef XSL_NET_TRANSPORT_RESOLVE
#  define XSL_NET_TRANSPORT_RESOLVE
#  include "xsl/feature.h"
#  include "xsl/net/transport/def.h"
#  include "xsl/wheel.h"
#  include "xsl/wheel/type_traits.h"

#  include <netdb.h>
#  include <sys/socket.h>

#  include <cstdint>
#  include <cstring>
#  include <tuple>
#  include <utility>

TRANSPORT_NAMESPACE_BEGIN
namespace impl {

  // template <class... Flags>
  class AddrInfo {
  public:
    AddrInfo(addrinfo *info) : info(info) {}
    AddrInfo(AddrInfo &&other) : info(std::exchange(other.info, nullptr)) {}
    AddrInfo &operator=(AddrInfo &&other) {
      if (this != &other) {
        freeaddrinfo(this->info);
        this->info = other.info;
        other.info = nullptr;
      }
      return *this;
    }
    AddrInfo(const AddrInfo &) = delete;
    AddrInfo &operator=(const AddrInfo &) = delete;
    ~AddrInfo() { freeaddrinfo(this->info); }

    addrinfo *info;
  };
}  // namespace impl
using AddrInfo = impl::AddrInfo;

using ResolveResult = Result<AddrInfo, std::string>;

namespace impl {
  template <uint8_t ipv>
  int family(feature::ip<ipv>) {
    if constexpr (ipv == 4) {
      return AF_INET;
    } else if constexpr (ipv == 6) {
      return AF_INET6;
    } else {
      return AF_UNSPEC;
    }
  }

  template <class Proto>
  std::tuple<int, int> traits() {
    if constexpr (std::is_same_v<Proto, feature::tcp>) {
      return {SOCK_STREAM, IPPROTO_TCP};
    } else if constexpr (std::is_same_v<Proto, feature::udp>) {
      return {SOCK_DGRAM, IPPROTO_UDP};
    } else {
      return {0, 0};
    }
  }

  template <class IpV, class Proto>
  ResolveResult resolve(const char *name, const char *serv, int flags = AI_ADDRCONFIG) {
    addrinfo hints;
    addrinfo *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_flags = flags;
    hints.ai_family = family(IpV());
    auto [type, protocol] = traits<Proto>();
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    int ret = getaddrinfo(name, serv, &hints, &res);
    if (ret != 0) {
      return {std::string{gai_strerror(ret)}};
    }
    return {AddrInfo(res)};
  }

  template <class IpV, class Proto>
  struct Resolver {
    static_assert(wheel::type_traits::existing_v<
                  IpV, wheel::type_traits::_n<feature::ip<4>, feature::ip<6>>>);
    static_assert(
        wheel::type_traits::existing_v<Proto, wheel::type_traits::_n<feature::tcp, feature::udp>>);
    static ResolveResult resolve(const char *name, const char *serv, int flags = AI_ADDRCONFIG) {
      return impl::resolve<IpV, Proto>(name, serv, flags);
    }
  };
}  // namespace impl
template <class IpV, class Proto>
using Resolver = impl::Resolver<IpV, Proto>;
TRANSPORT_NAMESPACE_END
#endif
