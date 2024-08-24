#pragma once
#ifndef XSL_SYS_NET_DEF
#  define XSL_SYS_NET_DEF
#  define XSL_SYS_NET_NB namespace xsl::_sys::net {
#  define XSL_SYS_NET_NE }
#  include "xsl/ai.h"
#  include "xsl/feature.h"
#  include "xsl/sys/sync.h"

#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>

#  include <cassert>
#  include <type_traits>
XSL_SYS_NET_NB

template <class SockTraits>
concept CSocketTraits = SockTraits::type == SOCK_STREAM || SockTraits::type == SOCK_SEQPACKET;

/**
 * @brief connection-based socket concept
 *
 * @tparam Sock
 */
template <class Sock>
concept CSocket = CSocketTraits<typename Sock::socket_traits_type>
                  && (ai::BRL<Sock> || ai::BWL<Sock> || ai::ABRL<Sock> || ai::ABWL<Sock>);

template <class Sock>
concept BindableSocket
    = Sock::socket_traits_type::family == AF_INET
      || Sock::socket_traits_type::family == AF_INET6;  // TODO: more family support

namespace impl_sock {
  struct DeviceTraits {
    using value_type = byte;
    using poll_traits_type = DefaultPollTraits;
  };

  template <int Family>
  struct FamilyTraits {
    static constexpr int family = Family;
  };

  template <int Type>
  struct TypeTraits {
    static constexpr int type = Type;
  };

  template <int Protocol>
  struct ProtocolTraits {
    static constexpr int protocol = Protocol;
  };

  struct AnySocketTraits {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };

  struct TcpIpv4SocketTraits : FamilyTraits<AF_INET>,
                               TypeTraits<SOCK_STREAM>,
                               ProtocolTraits<IPPROTO_TCP> {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };
  struct TcpIpv6SocketTraits : FamilyTraits<AF_INET6>,
                               TypeTraits<SOCK_STREAM>,
                               ProtocolTraits<IPPROTO_TCP> {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };
  struct TcpIpSocketTraits : FamilyTraits<AF_UNSPEC>,
                             TypeTraits<SOCK_STREAM>,
                             ProtocolTraits<IPPROTO_TCP> {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };
  struct UdpIpv4SocketTraits : FamilyTraits<AF_INET>,
                               TypeTraits<SOCK_DGRAM>,
                               ProtocolTraits<IPPROTO_UDP> {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };
  struct UdpIpv6SocketTraits : FamilyTraits<AF_INET6>,
                               TypeTraits<SOCK_DGRAM>,
                               ProtocolTraits<IPPROTO_UDP> {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };
  struct UdpIpSocketTraits : FamilyTraits<AF_UNSPEC>,
                             TypeTraits<SOCK_DGRAM>,
                             ProtocolTraits<IPPROTO_UDP> {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };
}  // namespace impl_sock

namespace impl_sock {
  using namespace xsl::feature;
  template <class... Flags>
  struct SocketTraitsTag;

  template <class... Flags>
  using SocketTraitsTagCompose = organize_feature_flags_t<
      SocketTraitsTag<
          set<Tcp<Ip<4>>, Tcp<Ip<6>>, Tcp<placeholder>, TcpIpv4, TcpIpv6, TcpIp,
              TcpIpv4SocketTraits, TcpIpv6SocketTraits, TcpIpSocketTraits, Udp<Ip<4>>, Udp<Ip<6>>,
              Udp<placeholder>, UdpIpv4, UdpIpv6, UdpIp, UdpIpv4SocketTraits, UdpIpv6SocketTraits,
              UdpIpSocketTraits, AnySocketTraits>>,
      Flags...>::type;

  template <>
  struct SocketTraitsTag<feature::Tcp<feature::Ip<4>>> : std::type_identity<TcpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv4> : std::type_identity<TcpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv4SocketTraits> : std::type_identity<TcpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<feature::Tcp<feature::Ip<6>>> : std::type_identity<TcpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv6> : std::type_identity<TcpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv6SocketTraits> : std::type_identity<TcpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<feature::Tcp<feature::placeholder>>
      : std::type_identity<TcpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIp> : std::type_identity<TcpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpSocketTraits> : std::type_identity<TcpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<feature::Udp<feature::Ip<4>>> : std::type_identity<UdpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv4> : std::type_identity<UdpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv4SocketTraits> : std::type_identity<UdpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<feature::Udp<feature::Ip<6>>> : std::type_identity<UdpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv6> : std::type_identity<UdpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv6SocketTraits> : std::type_identity<UdpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<feature::Udp<feature::placeholder>>
      : std::type_identity<UdpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIp> : std::type_identity<UdpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpSocketTraits> : std::type_identity<UdpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<placeholder> : std::type_identity<AnySocketTraits> {};
}  // namespace impl_sock

template <class... Flags>
using SocketTraits = impl_sock::SocketTraitsTagCompose<Flags...>;

namespace impl_sock {
  class SockAddrStorage {
  public:
    static std::pair<sockaddr *, socklen_t *> null() { return {nullptr, nullptr}; }

    SockAddrStorage() : _addr{}, _addrlen(sizeof(_addr)) {}
    SockAddrStorage(const SockAddrStorage &other) = default;
    SockAddrStorage(SockAddrStorage &&other) = default;
    SockAddrStorage &operator=(const SockAddrStorage &other) = default;
    SockAddrStorage &operator=(SockAddrStorage &&other) = default;

    SockAddrStorage(sockaddr *addr, socklen_t addrlen)
        : _addr(reinterpret_cast<sockaddr_storage &>(*addr)), _addrlen(addrlen) {}

    bool operator==(const SockAddrStorage &other) const {
      return this->_addrlen == other._addrlen && std::memcmp(&_addr, &other._addr, _addrlen) == 0;
    }

    void reset() {
      _addrlen = sizeof(_addr);
      std::memset(&_addr, 0, sizeof(_addr));
    }

    std::pair<sockaddr *, socklen_t *> raw() {
      return {reinterpret_cast<sockaddr *>(&_addr), &_addrlen};
    }

    std::expected<void, std::errc> parse(this auto &&self, std::string &ip, uint16_t &port) {
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
    sockaddr_storage _addr;
    socklen_t _addrlen;
  };
  template <class Traits>
  class SockAddr;

  template <>
  class SockAddr<AnySocketTraits> : public SockAddrStorage {
    using Base = SockAddrStorage;

  public:
    using traits_type = AnySocketTraits;
    using sockaddr_type = sockaddr;

  private:
    using Base::_addr;
    using Base::_addrlen;
  };

  template <class Traits>
    requires(Traits::family == AF_INET)
  class SockAddr<Traits> : public SockAddrStorage {
    using Base = SockAddrStorage;

  public:
    using traits_type = Traits;
    using sockaddr_type = sockaddr_in;
    using Base::Base;

    SockAddr(std::string_view ip, uint16_t port) : Base() {
      sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(&_addr);
      addr->sin_family = AF_INET;
      addr->sin_port = htons(port);
      inet_pton(AF_INET, ip.data(), &addr->sin_addr);
      _addrlen = sizeof(sockaddr_in);
    }

  private:
    using Base::_addr;
    using Base::_addrlen;
  };

  template <class Traits>
    requires(Traits::family == AF_INET6)
  class SockAddr<Traits> : public SockAddrStorage {
    using Base = SockAddrStorage;

  public:
    using traits_type = Traits;
    using sockaddr_type = sockaddr_in6;
    using Base::Base;

    SockAddr(std::string_view ip, uint16_t port) : Base() {
      sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(&_addr);
      addr->sin6_family = AF_INET6;
      addr->sin6_port = htons(port);
      inet_pton(AF_INET6, ip.data(), &addr->sin6_addr);
      _addrlen = sizeof(sockaddr_in6);
    }

  private:
    using Base::_addr;
    using Base::_addrlen;
  };
}  // namespace impl_sock

template <class... Flags>
using SockAddr = impl_sock::SockAddr<SocketTraits<Flags...>>;

XSL_SYS_NET_NE
#endif
