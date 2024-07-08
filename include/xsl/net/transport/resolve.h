#pragma once
#include <iterator>
#ifndef XSL_NET_TRANSPORT_RESOLVE
#  define XSL_NET_TRANSPORT_RESOLVE
#  include "xsl/feature.h"
#  include "xsl/net/transport/def.h"
#  include "xsl/wheel/type_traits.h"

#  include <netdb.h>
#  include <sys/socket.h>

#  include <cstdint>
#  include <cstring>
#  include <expected>
#  include <string>
#  include <system_error>
#  include <tuple>
#  include <utility>

TRANSPORT_NAMESPACE_BEGIN
namespace impl {
  class ResolveCategory : public std::error_category {
  public:
    ~ResolveCategory() = default;

    const char *name() const noexcept override { return "resolve"; }
    std::error_condition default_error_condition(int ev) const noexcept override {
      return ev == 0 ? std::error_condition{} : std::error_condition{ev, *this};
    }
    bool equivalent(int code, const std::error_condition &condition) const noexcept override {
      return condition.category() == *this && condition.value() == code;
    }
    bool equivalent(const std::error_code &code, int condition) const noexcept override {
      return code.category() == *this && code.value() == condition;
    }
    std::string message(int ev) const override { return gai_strerror(ev); }
  };
  class AddrInfo {
    class Iterator {
    public:
      using value_type = addrinfo;
      using difference_type = std::ptrdiff_t;
      using pointer = addrinfo *;
      using reference = addrinfo &;
      using iterator_category = std::forward_iterator_tag;

      Iterator() : info(nullptr) {}
      Iterator(addrinfo *info) : info(info) {}
      Iterator(const Iterator &) = default;
      Iterator &operator=(const Iterator &) = default;
      ~Iterator() = default;

      addrinfo &operator*() const { return *info; }
      addrinfo *operator->() const { return info; }

      Iterator &operator++() {
        info = info->ai_next;
        return *this;
      }

      Iterator operator++(int) {
        Iterator tmp = *this;
        ++*this;
        return tmp;
      }
      bool operator!=(const Iterator &rhs) const { return info != rhs.info; }
      bool operator==(const Iterator &rhs) const { return info == rhs.info; }

    private:
      addrinfo *info;
    };

    static_assert(std::forward_iterator<Iterator>);

  public:
    AddrInfo(addrinfo *info) : info(info) {}
    AddrInfo(AddrInfo &&other) : info(std::exchange(other.info, nullptr)) {}
    AddrInfo &operator=(AddrInfo &&other) {
      if (this != &other) {
        freeaddrinfo(this->info);
        this->info = other.info;
        other.info = nullptr;
      }
      return *this;
    }
    AddrInfo(const AddrInfo &) = delete;
    AddrInfo &operator=(const AddrInfo &) = delete;
    ~AddrInfo() {
      if (info) freeaddrinfo(info);
    }

    Iterator begin() const { return Iterator(info); }
    Iterator end() const { return Iterator(nullptr); }

    addrinfo *info;
  };
}  // namespace impl
using AddrInfo = impl::AddrInfo;

using ResolveResult = std::expected<AddrInfo, std::error_condition>;

namespace impl {
  template <uint8_t ipv>
  int family(feature::ip<ipv>) {
    if constexpr (ipv == 4) {
      return AF_INET;
    } else if constexpr (ipv == 6) {
      return AF_INET6;
    } else {
      return AF_UNSPEC;
    }
  }

  template <class Proto>
  std::tuple<int, int> traits() {
    if constexpr (std::is_same_v<Proto, feature::tcp>) {
      return {SOCK_STREAM, IPPROTO_TCP};
    } else if constexpr (std::is_same_v<Proto, feature::udp>) {
      return {SOCK_DGRAM, IPPROTO_UDP};
    } else {
      return {0, 0};
    }
  }

  template <class IpV, class Proto>
  ResolveResult resolve(const char *name, const char *serv, int flags = AI_ADDRCONFIG) {
    addrinfo hints;
    addrinfo *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_flags = flags;
    hints.ai_family = family(IpV());
    auto [type, protocol] = traits<Proto>();
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    int ret = getaddrinfo(name, serv, &hints, &res);
    if (ret != 0) {
      return std::unexpected{std::error_condition{ret, ResolveCategory()}};
    }
    return {AddrInfo(res)};
  }

  template <class IpV, class Proto>
  struct Resolver {
    static_assert(wheel::type_traits::existing_v<
                  IpV, wheel::type_traits::_n<feature::ip<4>, feature::ip<6>>>);
    static_assert(
        wheel::type_traits::existing_v<Proto, wheel::type_traits::_n<feature::tcp, feature::udp>>);
    /**
     * @brief Resolve the name and service to an address, typically used for connect
     *
     * @param name The name of the host
     * @param serv The service name or port number
     * @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG
     * @return ResolveResult
     */
    static ResolveResult resolve(const char *name, const char *serv, int flags = AI_ADDRCONFIG) {
      return impl::resolve<IpV, Proto>(name, serv, flags);
    }
    /**
     * @brief Resolve the service to an address, typically used for bind
     *
     * @param serv The service name or port number
     * @param flags The flags for getaddrinfo, default to AI_ADDRCONFIG | AI_PASSIVE
     * @return ResolveResult
     */
    static ResolveResult resolve(const char *serv, int flags = AI_ADDRCONFIG | AI_PASSIVE) {
      return impl::resolve<IpV, Proto>(nullptr, serv, flags);
    }
    static ResolveResult resolve(uint16_t port, int flags = AI_ADDRCONFIG | AI_PASSIVE) {
      char port_str[6];
      std::snprintf(port_str, sizeof(port_str), "%u", port);
      return impl::resolve<IpV, Proto>(nullptr, port_str, flags);
    }
  };
}  // namespace impl
template <class IpV, class Proto>
using Resolver = impl::Resolver<IpV, Proto>;
TRANSPORT_NAMESPACE_END
#endif
