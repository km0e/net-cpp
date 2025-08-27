/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network definitions
 * @version 0.2.0
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
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <xsl/feature.h>
#  include <xsl/io/context.h>
#  include <xsl/sys/def.h>

#  include <cassert>
#  include <type_traits>
XSL_SYS_NET_NB
namespace inet {
  using port_t = in_port_t;  ///< port type
}  // namespace inet

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
  constexpr StaticFamily() = default;
  constexpr StaticFamily(int) {}
};

template <int Family>
class FamilyTraits {
public:
  constexpr FamilyTraits() = default;
  constexpr FamilyTraits(int) {}
};

template <>
class FamilyTraits<AF_INET> : public StaticFamily<AF_INET> {
public:
  using StaticFamily::StaticFamily;
  static constexpr bool is_ip = true;
};

template <>
class FamilyTraits<AF_INET6> : public StaticFamily<AF_INET6> {
public:
  using StaticFamily::StaticFamily;
  static constexpr bool is_ip = true;
};

template <int Type>
class StaticType {
public:
  static consteval int type() { return Type; }
  constexpr StaticType() = default;
  constexpr StaticType(int) {}
};

template <int Type>
class TypeTraits {
protected:
  int _type = Type;  ///< type constant
public:
  constexpr int type() const { return _type; }
  static consteval bool is_connection_based() { return false; }
};

template <>
class TypeTraits<SOCK_STREAM> : public StaticType<SOCK_STREAM> {
public:
  static consteval bool is_connection_based() { return true; }
  using StaticType::StaticType;
};

template <>
class TypeTraits<SOCK_DGRAM> : public StaticType<SOCK_DGRAM> {
public:
  static consteval bool is_connection_based() { return false; }
  using StaticType::StaticType;
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
  static consteval int protocol() { return Protocol; }
  constexpr StaticProtocol() = default;
  constexpr StaticProtocol(int) {}
};

template <int Protocol>
class ProtocolTraits {
protected:
  int _protocol = Protocol;  ///< protocol constant
public:
  constexpr int protocol() const { return _protocol; }
};

template <>
class ProtocolTraits<IPPROTO_TCP> : public StaticProtocol<IPPROTO_TCP> {
public:
  using StaticProtocol::StaticProtocol;
  static constexpr bool is_tcp = true;  ///< is TCP protocol
};

template <>
class ProtocolTraits<IPPROTO_UDP> : public StaticProtocol<IPPROTO_UDP> {
public:
  using StaticProtocol::StaticProtocol;
  static constexpr bool is_udp = true;  ///< is UDP protocol
};

template <int Family, int Type, int Protocol>
struct SocketTraitsBase : public FamilyTraits<Family>,
                          public TypeTraits<Type>,
                          public ProtocolTraits<Protocol> {
  using poll_traits_type = io::DefaultPollTraits;  ///< poll traits
  SocketTraitsBase() = default;
  SocketTraitsBase(int family, int type, int protocol)
      : FamilyTraits<Family>(family), TypeTraits<Type>(type), ProtocolTraits<Protocol>(protocol) {}
};

struct AnySocketTraits : SocketTraitsBase<AF_UNSPEC, 0, 0> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct TcpIpv4SocketTraits : TcpIpv4, SocketTraitsBase<AF_INET, SOCK_STREAM, IPPROTO_TCP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct TcpIpv6SocketTraits : TcpIpv6, SocketTraitsBase<AF_INET6, SOCK_STREAM, IPPROTO_TCP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct TcpIpSocketTraits : TcpIp, SocketTraitsBase<AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct UdpIpv4SocketTraits : UdpIpv4, SocketTraitsBase<AF_INET, SOCK_DGRAM, IPPROTO_UDP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct UdpIpv6SocketTraits : UdpIpv6, SocketTraitsBase<AF_INET6, SOCK_DGRAM, IPPROTO_UDP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct UdpIpSocketTraits : UdpIp, SocketTraitsBase<AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP> {
  using SocketTraitsBase::SocketTraitsBase;
};

template <class Up, class Down>
concept SocketTraitsCompatible
    = ((!requires { Down::family(); }) || (Down::family() == Up::family()))
      && ((!requires { Down::type(); }) || (Down::type() == Up::type()))
      && ((!requires { Down::protocol(); }) || (Down::protocol() == Up::protocol()));

static_assert(SocketTraitsCompatible<TcpIpv4SocketTraits, AnySocketTraits>,
              "AnySocketTraits should be compatible with TcpIpv4SocketTraits");

namespace impl_sock {
  template <class... Flags>
  struct SocketTraitsTag;

  template <class T, class U>
  struct SocketMerge : std::type_identity<T> {};

  template <class T>
  struct SocketMerge<AnySocketTraits, T> : std::type_identity<T> {};
  template <class T>
  struct SocketMerge<T, T> : std::type_identity<T> {};
  template <class T, class U>
    requires(std::is_base_of_v<T, U>)
  struct SocketMerge<T, U> : std::type_identity<U> {};
  template <class T, class U>
    requires(std::is_base_of_v<U, T>)
  struct SocketMerge<T, U> : std::type_identity<T> {};

#  define XSL_DEFINE_SOCKET_MERGE(Flag1, Flag2, Result) \
    template <>                                         \
    struct SocketMerge<Flag1, Flag2> : std::type_identity<Result> {};

  XSL_DEFINE_SOCKET_MERGE(Tcp, Ip, TcpIpSocketTraits)
  XSL_DEFINE_SOCKET_MERGE(Udp, Ip, UdpIpSocketTraits)
  XSL_DEFINE_SOCKET_MERGE(TcpIp, Ipv4, TcpIpv4SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(TcpIp, Ipv6, TcpIpv6SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(UdpIp, Ipv4, UdpIpv4SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(UdpIp, Ipv6, UdpIpv6SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(Tcp, Ipv4, TcpIpv4SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(Tcp, Ipv6, TcpIpv6SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(Udp, Ipv4, UdpIpv4SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(Udp, Ipv6, UdpIpv6SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, AnySocketTraits, AnySocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, Tcp, TcpIpSocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, Udp, UdpIpSocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, Ipv4, TcpIpv4SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, Ipv6, TcpIpv6SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, TcpIp, TcpIpSocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, UdpIp, UdpIpSocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, TcpIpv4, TcpIpv4SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, TcpIpv6, TcpIpv6SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, UdpIpv4, UdpIpv4SocketTraits)
  XSL_DEFINE_SOCKET_MERGE(AnySocketTraits, UdpIpv6, UdpIpv6SocketTraits)

#  undef XSL_DEFINE_SOCKET_MERGE

}  // namespace impl_sock

template <class... Flags>
using SocketTraits = merge_feature_flags_t<impl_sock::SocketMerge, AnySocketTraits, Flags...>;

XSL_SYS_NET_NE
#endif
