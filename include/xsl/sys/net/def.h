#pragma once
#ifndef XSL_SYS_NET_DEF
#  define XSL_SYS_NET_DEF
#  define XSL_SYS_NET_NB namespace xsl::sys::net {
#  define XSL_SYS_NET_NE }
#  include "xsl/coro/semaphore.h"
#  include "xsl/feature.h"
#  include "xsl/sync.h"

#  include <netinet/in.h>
#  include <sys/socket.h>

#  include <concepts>
XSL_SYS_NET_NB
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
  struct SocketTraits<feature::Tcp<feature::Ip<4>>>
      : FamilyTraits<AF_INET>, TypeTraits<SOCK_STREAM>, ProtocolTraits<IPPROTO_TCP> {
    using poll_traits = sync::PollTraits;
  };

  template <>
  struct SocketTraits<feature::Tcp<feature::Ip<6>>>
      : FamilyTraits<AF_INET6>, TypeTraits<SOCK_STREAM>, ProtocolTraits<IPPROTO_TCP> {
    using poll_traits = sync::PollTraits;
  };

  template <>
  struct SocketTraits<feature::Tcp<feature::placeholder>>
      : FamilyTraits<AF_UNSPEC>, TypeTraits<SOCK_STREAM>, ProtocolTraits<IPPROTO_TCP> {
    using poll_traits = sync::PollTraits;
  };

  template <class... Flags>
  using SocketTraitsCompose = feature::organize_feature_flags_t<
      SocketTraits<feature::Item<wheel::type_traits::is_same_pack, feature::Tcp<void>>>, Flags...>;

}  // namespace impl_socket

template <class... Flags>
using SocketTraits = impl_socket::SocketTraitsCompose<Flags...>;

template <class S, template <class> class IO = feature::InOut>
concept SocketLike
    = wheel::type_traits::is_same_pack_v<typename S::socket_traits_type, SocketTraits<void>>
      && wheel::type_traits::is_same_pack_v<typename S::device_traits_type, IO<void>>;

template <class S, template <class> class IO = feature::InOut>
concept AsyncSocketLike = SocketLike<S, IO> && requires(S &s) {
  { s.sem() } -> std::convertible_to<coro::CountingSemaphore<1> &>;
};
XSL_SYS_NET_NE
#endif
