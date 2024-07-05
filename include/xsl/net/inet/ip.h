#pragma once
#ifndef XSL_NET_INET_IP
#  define XSL_NET_INET_IP
#  include "xsl/net/inet/def.h"

#  include <cstdint>
#  include <string>
INET_NAMESPACE_BEGIN
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
using Ip = impl::IpBase;
// ipv4
using IpV4 = impl::Ip<4>;
// ipv6
using IpV6 = impl::Ip<6>;

namespace impl {
  class IpAddrBase {
  public:
    IpAddrBase() = default;
    IpAddrBase(const char *ip, const char *port) : ip(ip), port(port) {}
    IpAddrBase(std::string_view ip, std::string_view port) : ip(ip), port(port) {}
    IpAddrBase(std::string_view sa4) : ip(), port() {
      size_t pos = sa4.find(':');
      if (pos == std::string_view::npos) {
        ip = sa4;
        port = "";
      } else {
        ip = sa4.substr(0, pos);
        port = sa4.substr(pos + 1);
      }
    }
    bool operator==(const IpAddrBase &rhs) const { return ip == rhs.ip && port == rhs.port; }
    std::string to_string() const { return ip + ":" + port; }
    std::string ip;
    std::string port;
  };
  template <uint8_t version>
  class IpAddr : public IpAddrBase {};
}  // namespace impl

using IpAddr = impl::IpAddrBase;
// ipv4
using IpV4Addr = impl::IpAddr<4>;
// ipv6
using IpV6Addr = impl::IpAddr<6>;

INET_NAMESPACE_END
#endif
