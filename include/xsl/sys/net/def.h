#pragma once
#ifndef XSL_SYS_NET_DEF
#  define XSL_SYS_NET_DEF
#  define XSL_SYS_NET_NB namespace xsl::_sys::net {
#  define XSL_SYS_NET_NE }
#  include "xsl/feature.h"
#  include "xsl/sys/sync.h"

#  include <netinet/in.h>
#  include <sys/socket.h>

#  include <concepts>
#  include <type_traits>
XSL_SYS_NET_NB
namespace tag {
  struct TcpIpv4 {};
  struct TcpIpv6 {};
  struct TcpIp {};
}  // namespace tag
namespace impl_socket {
  using namespace xsl::feature;
  template <class... Flags>
  struct SocketTraits;

  template <class... Flags>
  using SocketTraitsTagCompose
      = organize_feature_flags_t<SocketTraits<set<Tcp<Ip<4>>, Tcp<Ip<6>>, Tcp<placeholder>,
                                                  tag::TcpIpv4, tag::TcpIpv6, tag::TcpIp>>,
                                 Flags...>::type;

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

  template <>
  struct SocketTraits<feature::Tcp<feature::Ip<4>>> : std::type_identity<tag::TcpIpv4> {};

  template <>
  struct SocketTraits<tag::TcpIpv4> : std::type_identity<tag::TcpIpv4> {};

  template <>
  struct SocketTraits<feature::Tcp<feature::Ip<6>>> : std::type_identity<tag::TcpIpv6> {};

  template <>
  struct SocketTraits<tag::TcpIpv6> : std::type_identity<tag::TcpIpv6> {};

  template <>
  struct SocketTraits<feature::Tcp<feature::placeholder>> : std::type_identity<tag::TcpIp> {};

  template <>
  struct SocketTraits<tag::TcpIp> : std::type_identity<tag::TcpIp> {};
}  // namespace impl_socket

template <class Tag>
struct SocketTraits;

template <>
struct SocketTraits<tag::TcpIpv4> : impl_socket::FamilyTraits<AF_INET>,
                                    impl_socket::TypeTraits<SOCK_STREAM>,
                                    impl_socket::ProtocolTraits<IPPROTO_TCP> {
  using tag_type = tag::TcpIpv4;
  using poll_traits = PollTraits;
};

template <>
struct SocketTraits<tag::TcpIpv6> : impl_socket::FamilyTraits<AF_INET6>,
                                    impl_socket::TypeTraits<SOCK_STREAM>,
                                    impl_socket::ProtocolTraits<IPPROTO_TCP> {
  using tag_type = tag::TcpIpv6;
  using poll_traits = PollTraits;
};

template <>
struct SocketTraits<tag::TcpIp> : impl_socket::FamilyTraits<AF_UNSPEC>,
                                  impl_socket::TypeTraits<SOCK_STREAM>,
                                  impl_socket::ProtocolTraits<IPPROTO_TCP> {
  using tag_type = tag::TcpIp;
  using poll_traits = PollTraits;
};

template <class... Flags>
using SocketTraitsTag = impl_socket::SocketTraitsTagCompose<Flags...>;

template <class S, template <class> class IO = feature::InOut>
concept SocketLike = type_traits::is_same_pack_v<typename S::socket_traits_type, SocketTraits<void>>
                     && type_traits::is_same_pack_v<typename S::device_features_type, IO<void>>;

template <class S, template <class> class IO = feature::InOut>
concept AsyncSocketLike = SocketLike<S, IO> && requires(S &s) {
  { s.sem() } -> std::convertible_to<coro::CountingSemaphore<1> &>;
};
XSL_SYS_NET_NE

#  include "xsl/ai/def.h"
XSL_AI_NB
template <>
struct DeviceTraits<_sys::net::tag::TcpIpv4> {
  using value_type = byte;
};
template <>
struct DeviceTraits<_sys::net::tag::TcpIpv6> {
  using value_type = byte;
};
template <>
struct DeviceTraits<_sys::net::tag::TcpIp> {
  using value_type = byte;
};
XSL_AI_NE
#endif
