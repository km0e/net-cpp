/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network definitions
 * @version 0.13
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

enum class SocketAttribute : int {
  NonBlocking = SOCK_NONBLOCK,
  CloseOnExec = SOCK_CLOEXEC,
};

constexpr SocketAttribute operator|(SocketAttribute a, SocketAttribute b) {
  return static_cast<SocketAttribute>(static_cast<int>(a) | static_cast<int>(b));
}

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
  constexpr int family() { return _family; }
};

template <>
class FamilyTraits<AF_INET> : public StaticFamily<AF_INET> {};

template <>
class FamilyTraits<AF_INET6> : public StaticFamily<AF_INET6> {};

template <int Type>
class StaticType {
public:
  static constexpr int type() { return Type; }
};

template <int Type>
class TypeTraits {
protected:
  int _type = Type;  ///< type constant
public:
  constexpr int type() { return _type; }
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

/// @brief Connection-based socket concept
template <class SockTraits>
concept ConnectionBasedSocketTraits = SockTraits::is_connection_based();

/// @brief Connection-less socket concept
template <class SockTraits>
concept ConnectionLessSocketTraits = !SockTraits::is_connection_based();

template <int Protocol>
class StaticProtocol {
public:
  static constexpr int protocol() { return Protocol; }
};

template <int Protocol>
class ProtocolTraits {
protected:
  int _protocol = Protocol;  ///< protocol constant
public:
  constexpr int protocol() { return _protocol; }
};

template <>
class ProtocolTraits<IPPROTO_TCP> : public StaticProtocol<IPPROTO_TCP> {};

template <>
class ProtocolTraits<IPPROTO_UDP> : public StaticProtocol<IPPROTO_UDP> {};

template <class Up, class Down>
concept SocketTraitsCompatible
    = ((!requires { Down::family(); }) || (Down::family() == Up::family()))
      && ((!requires { Down::type(); }) || (Down::type() == Up::type()))
      && ((!requires { Down::protocol(); }) || (Down::protocol() == Up::protocol()));

template <int Family, int Type, int Protocol>
struct SocketTraitsBase : FamilyTraits<Family>, TypeTraits<Type>, ProtocolTraits<Protocol> {
  using poll_traits_type = DefaultPollTraits;  ///< poll traits
  SocketTraitsBase() = default;
  SocketTraitsBase(int family, int type, int protocol)
      : FamilyTraits<Family>(family), TypeTraits<Type>(type), ProtocolTraits<Protocol>(protocol) {}
};

struct AnySocketTraits : public SocketTraitsBase<AF_UNSPEC, 0, 0> {};
struct TcpIpv4SocketTraits : public SocketTraitsBase<AF_INET, SOCK_STREAM, IPPROTO_TCP> {};
struct TcpIpv6SocketTraits : public SocketTraitsBase<AF_INET6, SOCK_STREAM, IPPROTO_TCP> {};
struct TcpIpSocketTraits : public SocketTraitsBase<AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP> {};
struct UdpIpv4SocketTraits : public SocketTraitsBase<AF_INET, SOCK_DGRAM, IPPROTO_UDP> {};
struct UdpIpv6SocketTraits : public SocketTraitsBase<AF_INET6, SOCK_DGRAM, IPPROTO_UDP> {};
struct UdpIpSocketTraits : public SocketTraitsBase<AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP> {};

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

template <class Traits>
class AsyncReadWriteSocket;
XSL_SYS_NET_NE
#endif
