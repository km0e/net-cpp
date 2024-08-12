#pragma once
#ifndef XSL_SYS_NET_SOCKET
#  define XSL_SYS_NET_SOCKET
#  include "xsl/feature.h"
#  include "xsl/sys/net/def.h"
#  include "xsl/sys/net/dev.h"

#  include <sys/socket.h>

#  include <cstdint>
#  include <expected>
#  include <string>
XSL_SYS_NET_NB

namespace impl {
  class IpBase {
  public:
    IpBase() = default;
    IpBase(const char *ip) : _ip(ip) {}
    IpBase(std::string_view ip) : _ip(ip) {}
    bool operator==(const IpBase &rhs) const { return _ip == rhs._ip; }
    std::string _ip;
  };
  template <uint8_t version>
  class Ip : public IpBase {};
}  // namespace impl
using IpAddr = impl::IpBase;
// ipv4
using IpV4Addr = impl::Ip<4>;
// ipv6
using IpV6Addr = impl::Ip<6>;

class SockAddr {
public:
  static constexpr std::tuple<sockaddr *, socklen_t *> null() { return {nullptr, nullptr}; }

  constexpr SockAddr() noexcept : _addr(), _len(sizeof(_addr)) {}
  constexpr std::tuple<sockaddr *, socklen_t *> raw() { return {&_addr, &_len}; }

private:
  sockaddr _addr;
  socklen_t _len;
};

template <class Traits>
using Socket = sys::net::Device<feature::InOut<Traits>>;

template <class LowerLayer>
using TcpSocket = Socket<SocketTraits<feature::Tcp<LowerLayer>>>;

template <class Traits>
using AsyncSocket = sys::net::AsyncDevice<feature::InOut<Traits>>;

template <class LowerLayer>
using AsyncTcpSocket = AsyncSocket<SocketTraits<feature::Tcp<LowerLayer>>>;

template <class LowerLayer>
using DynAsyncTcpSocket
    = sys::net::AsyncDevice<feature::InOut<SocketTraits<feature::Tcp<LowerLayer>>>, feature::Dyn>;

XSL_SYS_NET_NE
#endif
