#pragma once
#ifndef XSL_SYS_NET_DEF
#  define XSL_SYS_NET_DEF
#  define SYS_NET_NB namespace xsl::sys::net {
#  define SYS_NET_NE }
#  include "xsl/feature.h"
#  include "xsl/sync.h"

#  include <netinet/in.h>
#  include <sys/socket.h>
SYS_NET_NB
namespace impl_socket {
  template <class... Flags>
  struct SocketTraits;

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
  struct SocketTraits<xsl::feature::Tcp, xsl::feature::Ip<4>>
      : FamilyTraits<AF_INET>, TypeTraits<SOCK_STREAM>, ProtocolTraits<IPPROTO_TCP> {
    using poll_traits = xsl::sync::PollTraits;
  };

  template <>
  struct SocketTraits<xsl::feature::Tcp, xsl::feature::Ip<6>>
      : FamilyTraits<AF_INET6>, TypeTraits<SOCK_STREAM>, ProtocolTraits<IPPROTO_TCP> {
    using poll_traits = xsl::sync::PollTraits;
  };

  template <>
  struct SocketTraits<xsl::feature::Tcp, xsl::feature::placeholder>
      : FamilyTraits<AF_UNSPEC>, TypeTraits<SOCK_STREAM>, ProtocolTraits<IPPROTO_TCP> {
    using poll_traits = xsl::sync::PollTraits;
  };

  template <class... Flags>
  using SocketTraitsCompose = xsl::feature::organize_feature_flags_t<
      SocketTraits<xsl::feature::set<xsl::feature::Tcp>,
                   xsl::feature::set<xsl::feature::Ip<4>, xsl::feature::Ip<6>>>,
      Flags...>;

}  // namespace impl_socket

template <class... Flags>
using SocketTraits = impl_socket::SocketTraitsCompose<Flags...>;
SYS_NET_NE
#endif
