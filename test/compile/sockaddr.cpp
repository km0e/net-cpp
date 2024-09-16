/**
 * @file test_net.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/sys/net/def.h"

using namespace xsl::_sys::net;
using namespace xsl;

int main() {
  static_assert(SocketTraitsCompatible<TcpIpv4SocketTraits, TcpIpv4SocketTraits>);
  static_assert(SocketTraitsCompatible<UdpIpv4SocketTraits, UdpIpv4SocketTraits>);
  static_assert(!SocketTraitsCompatible<TcpIpv4SocketTraits, UdpIpv4SocketTraits>);

  // using Down = UdpIpv4SocketTraits;
  // using Up = TcpIpv4SocketTraits;
  // static_assert(!requires { Down::family(); });
  // static_assert(requires { Down::family() == Up::family(); });
  // static_assert(!requires { Down::type(); });
  // static_assert(Down::type() == Up::type());
  // static_assert((!requires { Down::protocol(); })
                // || (requires { Down::protocol() == Up::protocol(); }));
}
