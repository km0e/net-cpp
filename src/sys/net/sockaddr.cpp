/**
 * @file sockaddr.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.1
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <xsl/sys/net/def.h>
XSL_SYS_NET_NB

auto inet4_n2p(const sockaddr_in& storage, std::string& ip, inet::port_t& port) -> errc {
  errc ec{};
  ip.resize_and_overwrite(INET_ADDRSTRLEN, [&](auto data, auto) {
    if (inet_ntop(AF_INET, &storage.sin_addr, data, INET_ADDRSTRLEN)) {
      port = ntohs(storage.sin_port);
      return std::strlen(data);
    } else {
      ec = errc{errno};
      return 0uz;
    }
  });
  return ec;
}
auto inet6_n2p(const sockaddr_in6& storage, std::string& ip, inet::port_t& port) -> errc {
  errc ec{};
  ip.resize_and_overwrite(INET6_ADDRSTRLEN, [&](auto data, auto) {
    if (inet_ntop(AF_INET6, &storage.sin6_addr, data, INET6_ADDRSTRLEN)) {
      port = ntohs(storage.sin6_port);
      return std::strlen(data);
    } else {
      ec = errc{errno};
      return 0uz;
    }
  });
  return ec;
}

auto inet_n2p(const sockaddr_storage& storage, std::string& ip, inet::port_t& port) -> errc {
  switch (storage.ss_family) {
    case AF_INET:
      return inet4_n2p(reinterpret_cast<const sockaddr_in&>(storage), ip, port);
    case AF_INET6:
      return inet6_n2p(reinterpret_cast<const sockaddr_in6&>(storage), ip, port);
    default:
      return errc::address_family_not_supported;
  }
}

auto inet4_n2p(const sockaddr_in& storage, std::string& addr) -> errc {
  errc ec{};
  addr.resize_and_overwrite(INET_ADDRSTRLEN + 6, [&](auto data, auto) {
    if (inet_ntop(AF_INET, &storage.sin_addr, data, INET_ADDRSTRLEN)) {
      auto len = std::strlen(data);
      data[len] = ':';  // add a colon for port
      std::to_chars_result res
          = std::to_chars(data + len + 1, data + INET_ADDRSTRLEN + 6, ntohs(storage.sin_port));
      if (res.ec == std::errc{}) {
        return static_cast<std::size_t>(res.ptr - data);
      }
      ec = res.ec;
    } else {
      ec = errc{errno};
    }
    return 0uz;
  });
  return ec;
}
auto inet6_n2p(const sockaddr_in6& storage, std::string& addr) -> errc {
  errc ec{};
  addr.resize_and_overwrite(INET6_ADDRSTRLEN + 8, [&](auto data, auto) {
    if (inet_ntop(AF_INET6, &storage.sin6_addr, data + 1, INET6_ADDRSTRLEN)) {
      data[0] = '[';  // add a leading '['
      auto len = std::strlen(data + 1) + 1;
      data[len] = ']';      // add a trailing ']'
      data[len + 1] = ':';  // add a colon for port
      std::to_chars_result res
          = std::to_chars(data + len + 2, data + INET6_ADDRSTRLEN + 7, ntohs(storage.sin6_port));
      if (res.ec == std::errc{}) {
        return static_cast<std::size_t>(res.ptr - data);
      }
      ec = res.ec;
    } else {
      ec = errc{errno};
    }
    return 0uz;
  });
  return ec;
}

auto inet_n2p(const sockaddr_storage& storage, std::string& addr) -> errc {
  switch (storage.ss_family) {
    case AF_INET:
      return inet4_n2p(reinterpret_cast<const sockaddr_in&>(storage), addr);
    case AF_INET6:
      return inet6_n2p(reinterpret_cast<const sockaddr_in6&>(storage), addr);
    default:
      return errc::address_family_not_supported;
  }
}

XSL_SYS_NET_NE
