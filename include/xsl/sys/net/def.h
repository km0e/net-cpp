/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network definitions
 * @version 0.1.5
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

struct AnySocketTraits : public SocketTraitsBase<AF_UNSPEC, 0, 0> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct TcpIpv4SocketTraits : public SocketTraitsBase<AF_INET, SOCK_STREAM, IPPROTO_TCP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct TcpIpv6SocketTraits : public SocketTraitsBase<AF_INET6, SOCK_STREAM, IPPROTO_TCP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct TcpIpSocketTraits : public SocketTraitsBase<AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct UdpIpv4SocketTraits : public SocketTraitsBase<AF_INET, SOCK_DGRAM, IPPROTO_UDP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct UdpIpv6SocketTraits : public SocketTraitsBase<AF_INET6, SOCK_DGRAM, IPPROTO_UDP> {
  using SocketTraitsBase::SocketTraitsBase;
};
struct UdpIpSocketTraits : public SocketTraitsBase<AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP> {
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

XSL_SYS_NET_NE
#endif
