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
auto inet4_to_string(const sockaddr_storage &storage, std::string &addr) -> void {
  addr.resize(INET_ADDRSTRLEN);
  auto sa = reinterpret_cast<const sockaddr_in *>(&storage);
  if (inet_ntop(AF_INET, &sa->sin_addr, addr.data(), INET_ADDRSTRLEN)) {
    addr.resize(std::strlen(addr.data()) + 1);
    addr.back() = ':';  // add a colon for port
    addr += std::to_string(ntohs(sa->sin_port));
  } else {
    addr.clear();  // clear if error
  }
}
auto inet6_to_string(const sockaddr_storage &storage, std::string &addr) -> void {
  addr.resize(INET6_ADDRSTRLEN);
  auto sa = reinterpret_cast<const sockaddr_in6 *>(&storage);
  if (inet_ntop(AF_INET, &sa->sin6_addr, addr.data(), INET_ADDRSTRLEN)) {
    addr.resize(std::strlen(addr.data()) + 1);
    addr.back() = ':';  // add a colon for port
    addr += std::to_string(ntohs(sa->sin6_port));
  } else {
    addr.clear();  // clear if error
  }
  return;
}

auto inet_to_string(const sockaddr_storage &storage, std::string &addr) -> void {
  switch (storage.ss_family) {
    case AF_INET:
      inet4_to_string(storage, addr);
      break;
    case AF_INET6:
      inet6_to_string(storage, addr);
      break;
    default:
      addr.clear();  // clear if unknown family
      break;
  }
}

XSL_SYS_NET_NE
