#pragma once
#ifndef XSL_SYS_NET_DEF
#  define XSL_SYS_NET_DEF
#  define XSL_SYS_NET_NB namespace xsl::_sys::net {
#  define XSL_SYS_NET_NE }
#  include "xsl/feature.h"
#  include "xsl/sys/sync.h"

#  include <netinet/in.h>
#  include <sys/socket.h>

#  include <type_traits>
XSL_SYS_NET_NB

/**
 * @brief connection-based socket concept
 *
 * @tparam Sock
 */
template <class Sock>
concept CSocket = Sock::socket_traits_type::type == SOCK_STREAM
                  || Sock::socket_traits_type::type == SOCK_SEQPACKET;

template <class Sock>
concept BindableSocket
    = Sock::socket_traits_type::family == AF_INET
      || Sock::socket_traits_type::family == AF_INET6;  // TODO: more family support

class SockAddr {
public:
  static constexpr std::tuple<sockaddr *, socklen_t *> null() { return {nullptr, nullptr}; }

  constexpr SockAddr() noexcept : _addr(), _len(sizeof(_addr)) {}
  constexpr std::tuple<sockaddr *, socklen_t *> raw() { return {&_addr, &_len}; }

private:
  sockaddr _addr;
  socklen_t _len;
};

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
}  // namespace impl_sock

namespace impl_sock {
  using namespace xsl::feature;
  template <class... Flags>
  struct SocketTraitsTag;

  template <class... Flags>
  using SocketTraitsTagCompose = organize_feature_flags_t<
      SocketTraitsTag<set<Tcp<Ip<4>>, Tcp<Ip<6>>, Tcp<placeholder>, TcpIpv4, TcpIpv6, TcpIp,
                          TcpIpv4SocketTraits, TcpIpv6SocketTraits, TcpIpSocketTraits>>,
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
}  // namespace impl_sock

template <class... Flags>
using SocketTraits = impl_sock::SocketTraitsTagCompose<Flags...>;
XSL_SYS_NET_NE
#endif
