/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network definitions
 * @version 0.11
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
    using value_type = byte;                     ///< value type
    using poll_traits_type = DefaultPollTraits;  ///< poll traits
  };

  template <int Family>
  struct FamilyTraits {
    static constexpr int family = Family;  ///< family constant
  };

  template <int Type>
  struct TypeTraits {
    static constexpr int type = Type;  ///< type constant
  };

  template <int Protocol>
  struct ProtocolTraits {
    static constexpr int protocol = Protocol;  ///< protocol constant
  };

  struct AnySocketTraits {
    using poll_traits_type = DefaultPollTraits;
    using device_traits_type = DeviceTraits;
  };

  struct TcpIpv4SocketTraits : FamilyTraits<AF_INET>,
                               TypeTraits<SOCK_STREAM>,
                               ProtocolTraits<IPPROTO_TCP> {
    using poll_traits_type = DefaultPollTraits;  ///< poll traits
    using device_traits_type = DeviceTraits;     ///< device traits
  };
  struct TcpIpv6SocketTraits : FamilyTraits<AF_INET6>,
                               TypeTraits<SOCK_STREAM>,
                               ProtocolTraits<IPPROTO_TCP> {
    using poll_traits_type = DefaultPollTraits;  ///< poll traits
    using device_traits_type = DeviceTraits;     ///< device traits
  };
  struct TcpIpSocketTraits : FamilyTraits<AF_UNSPEC>,
                             TypeTraits<SOCK_STREAM>,
                             ProtocolTraits<IPPROTO_TCP> {
    using poll_traits_type = DefaultPollTraits;  ///< poll traits
    using device_traits_type = DeviceTraits;     ///< device traits
  };
  struct UdpIpv4SocketTraits : FamilyTraits<AF_INET>,
                               TypeTraits<SOCK_DGRAM>,
                               ProtocolTraits<IPPROTO_UDP> {
    using poll_traits_type = DefaultPollTraits;  ///< poll traits
    using device_traits_type = DeviceTraits;     ///< device traits
  };
  struct UdpIpv6SocketTraits : FamilyTraits<AF_INET6>,
                               TypeTraits<SOCK_DGRAM>,
                               ProtocolTraits<IPPROTO_UDP> {
    using poll_traits_type = DefaultPollTraits;  ///< poll traits
    using device_traits_type = DeviceTraits;     ///< device traits
  };
  struct UdpIpSocketTraits : FamilyTraits<AF_UNSPEC>,
                             TypeTraits<SOCK_DGRAM>,
                             ProtocolTraits<IPPROTO_UDP> {
    using poll_traits_type = DefaultPollTraits;  ///< poll traits
    using device_traits_type = DeviceTraits;     ///< device traits
  };
}  // namespace impl_sock

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
      Flags...>::type;

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
using SocketTraits = impl_sock::SocketTraitsTagCompose<Flags...>;

namespace impl_sock {
  class SockAddrStorage {
  public:
    /**
     * @brief null address
     *
     * @return std::pair<sockaddr *, socklen_t *>
     */
    static std::pair<sockaddr *, socklen_t *> null() { return {nullptr, nullptr}; }
    /**
     * @brief Construct a new Sock Addr Storage object
     *
     */
    SockAddrStorage() : _addr{}, _addrlen(sizeof(_addr)) {}
    SockAddrStorage(const SockAddrStorage &other) = default;             ///< copy constructor
    SockAddrStorage(SockAddrStorage &&other) = default;                  ///< move constructor
    SockAddrStorage &operator=(const SockAddrStorage &other) = default;  ///< copy assignment
    SockAddrStorage &operator=(SockAddrStorage &&other) = default;       ///< move assignment
    /**
     * @brief Construct a new Sock Addr Storage object from raw address and length
     *
     * @param addr raw address
     * @param addrlen address length
     */
    SockAddrStorage(sockaddr *addr, socklen_t addrlen)
        : _addr(reinterpret_cast<sockaddr_storage &>(*addr)), _addrlen(addrlen) {}

    /**
     * @brief compare address
     *
     * @param other other address
     * @return true if equal
     * @return false if not equal
     */
    bool operator==(const SockAddrStorage &other) const {
      return this->_addrlen == other._addrlen && std::memcmp(&_addr, &other._addr, _addrlen) == 0;
    }
    /**
     * @brief reset address
     *
     */
    void reset() {
      _addrlen = sizeof(_addr);
      std::memset(&_addr, 0, sizeof(_addr));
    }
    /**
     * @brief raw address
     *
     * @return std::pair<sockaddr *, socklen_t *>
     */
    std::pair<sockaddr *, socklen_t *> raw() {
      return {reinterpret_cast<sockaddr *>(&_addr), &_addrlen};
    }
    /**
     * @brief raw address
     *
     * @param self this, some derived class
     * @param ip ip address
     * @param port port number
     * @return std::expected<void, std::errc>
     */
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
    sockaddr_storage _addr;  ///< address storage
    socklen_t _addrlen;      ///< address length
  };
  template <class Traits>
  class SockAddr;

  template <>
  class SockAddr<AnySocketTraits> : public SockAddrStorage {
    using Base = SockAddrStorage;

  public:
    using traits_type = AnySocketTraits;  ///< traits type
    using sockaddr_type = sockaddr;       ///< sockaddr type

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
    /**
     * @brief Construct a new Sock Addr object from ip and port
     *
     * @param ip ip address
     * @param port port number
     */
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
    using traits_type = Traits;          ///< traits type
    using sockaddr_type = sockaddr_in6;  ///< sockaddr type
    using Base::Base;

    /**
     * @brief Construct a new Sock Addr object from ip and port
     *
     * @param ip ip address
     * @param port port number
     */
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
