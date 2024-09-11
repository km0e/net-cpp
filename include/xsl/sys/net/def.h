/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network definitions
 * @version 0.12
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_DEF
#  define XSL_SYS_NET_DEF
#  define XSL_SYS_NET_NB namespace xsl::_sys::net {
#  define XSL_SYS_NET_NE }
#  include "xsl/feature.h"
#  include "xsl/sys/sync.h"

#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>

#  include <cassert>
#  include <type_traits>
XSL_SYS_NET_NB

/// @brief Connection-based socket concept
template <class SockTraits>
concept CSocketTraits = SockTraits::type == SOCK_STREAM || SockTraits::type == SOCK_SEQPACKET;

/// @brief Datagram-based socket concept
template <class Sock>
concept CSocket = CSocketTraits<typename Sock::socket_traits_type>;

template <class Sock>
concept BindableSocket
    = Sock::socket_traits_type::family == AF_INET
      || Sock::socket_traits_type::family == AF_INET6;  // TODO: more family support

template <int Family>
class StaticFamily {
public:
  static consteval int family() { return Family; }
};

template <int Family>
class FamilyTraits {
protected:
  int _family = Family;  ///< family constant
public:
  consteval int family() { return _family; }
};

template <>
class FamilyTraits<AF_INET> : public StaticFamily<AF_INET> {};

template <>
class FamilyTraits<AF_INET6> : public StaticFamily<AF_INET6> {};

template <int Type>
class StaticType {
public:
  consteval int type() { return Type; }
};

template <int Type>
class TypeTraits {
protected:
  int _type = Type;  ///< type constant
public:
  consteval int type() { return _type; }
  static consteval bool is_connection_based() { return false; }
};

template <>
class TypeTraits<SOCK_STREAM> : public StaticType<SOCK_STREAM> {
public:
  static consteval bool is_connection_based() { return true; }
};

template <>
class TypeTraits<SOCK_DGRAM> : public StaticType<SOCK_DGRAM> {
public:
  static consteval bool is_connection_based() { return false; }
};

template <int Protocol>
class StaticProtocol {
public:
  consteval int protocol() { return Protocol; }
};

template <int Protocol>
class ProtocolTraits {
protected:
  int _protocol = Protocol;  ///< protocol constant
public:
  consteval int protocol() { return _protocol; }
};

template <>
class ProtocolTraits<IPPROTO_TCP> : public StaticProtocol<IPPROTO_TCP> {};

template <>
class ProtocolTraits<IPPROTO_UDP> : public StaticProtocol<IPPROTO_UDP> {};

struct AnySocketTraits : FamilyTraits<AF_UNSPEC>,
                         TypeTraits<SOCK_STREAM>,
                         ProtocolTraits<0> {
  using poll_traits_type = DefaultPollTraits;
};

struct TcpIpv4SocketTraits : FamilyTraits<AF_INET>,
                             TypeTraits<SOCK_STREAM>,
                             ProtocolTraits<IPPROTO_TCP> {
  using poll_traits_type = DefaultPollTraits;  ///< poll traits
};
struct TcpIpv6SocketTraits : FamilyTraits<AF_INET6>,
                             TypeTraits<SOCK_STREAM>,
                             ProtocolTraits<IPPROTO_TCP> {
  using poll_traits_type = DefaultPollTraits;  ///< poll traits
};
struct TcpIpSocketTraits : FamilyTraits<AF_UNSPEC>,
                           TypeTraits<SOCK_STREAM>,
                           ProtocolTraits<IPPROTO_TCP> {
  using poll_traits_type = DefaultPollTraits;  ///< poll traits
};
struct UdpIpv4SocketTraits : FamilyTraits<AF_INET>,
                             TypeTraits<SOCK_DGRAM>,
                             ProtocolTraits<IPPROTO_UDP> {
  using poll_traits_type = DefaultPollTraits;  ///< poll traits
};
struct UdpIpv6SocketTraits : FamilyTraits<AF_INET6>,
                             TypeTraits<SOCK_DGRAM>,
                             ProtocolTraits<IPPROTO_UDP> {
  using poll_traits_type = DefaultPollTraits;  ///< poll traits
};
struct UdpIpSocketTraits : FamilyTraits<AF_UNSPEC>,
                           TypeTraits<SOCK_DGRAM>,
                           ProtocolTraits<IPPROTO_UDP> {
  using poll_traits_type = DefaultPollTraits;  ///< poll traits
};

namespace impl_sock {
  template <class... Flags>
  struct SocketTraitsTag;

  template <class... Flags>
  using SocketTraitsTagCompose = organize_feature_flags_t<
      SocketTraitsTag<
          set<Tcp<Ip<4>>, Tcp<Ip<6>>, Tcp<Placeholder>, TcpIpv4, TcpIpv6, TcpIp,
              TcpIpv4SocketTraits, TcpIpv6SocketTraits, TcpIpSocketTraits, Udp<Ip<4>>, Udp<Ip<6>>,
              Udp<Placeholder>, UdpIpv4, UdpIpv6, UdpIp, UdpIpv4SocketTraits, UdpIpv6SocketTraits,
              UdpIpSocketTraits, AnySocketTraits>>,
      Flags...>;
  template <>
  struct SocketTraitsTag<AnySocketTraits> : std::type_identity<AnySocketTraits> {};

  template <>
  struct SocketTraitsTag<Tcp<Ip<4>>> : std::type_identity<TcpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv4> : std::type_identity<TcpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv4SocketTraits> : std::type_identity<TcpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<Tcp<Ip<6>>> : std::type_identity<TcpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv6> : std::type_identity<TcpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpv6SocketTraits> : std::type_identity<TcpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<Tcp<Placeholder>> : std::type_identity<TcpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIp> : std::type_identity<TcpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<TcpIpSocketTraits> : std::type_identity<TcpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<Udp<Ip<4>>> : std::type_identity<UdpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv4> : std::type_identity<UdpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv4SocketTraits> : std::type_identity<UdpIpv4SocketTraits> {};

  template <>
  struct SocketTraitsTag<Udp<Ip<6>>> : std::type_identity<UdpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv6> : std::type_identity<UdpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpv6SocketTraits> : std::type_identity<UdpIpv6SocketTraits> {};

  template <>
  struct SocketTraitsTag<Udp<Placeholder>> : std::type_identity<UdpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIp> : std::type_identity<UdpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<UdpIpSocketTraits> : std::type_identity<UdpIpSocketTraits> {};

  template <>
  struct SocketTraitsTag<Placeholder> : std::type_identity<AnySocketTraits> {};
}  // namespace impl_sock

template <class... Flags>
using SocketTraits = impl_sock::SocketTraitsTagCompose<Flags...>::type;

template <class Traits>
class ReadWriteSocket;

XSL_SYS_NET_NE
#endif
