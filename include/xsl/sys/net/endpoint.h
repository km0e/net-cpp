/**
 * @file endpoint.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_NET_ENDPOINT
#  define XSL_SYS_NET_ENDPOINT
#  include "xsl/sys/net/def.h"

#  include <netdb.h>

#  include <cstddef>
#  include <iterator>
#  include <utility>

XSL_SYS_NET_NB
template <class Traits>
class Endpoint {
public:
  constexpr Endpoint(addrinfo *info) : info(info) {}
  constexpr addrinfo *raw() const { return info; }

private:
  addrinfo *info;
};
template <class Traits>
class EndpointSet {
  class Iterator {
  public:
    using value_type = Endpoint<Traits>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;

    constexpr Iterator() : _ep(nullptr) {}
    constexpr Iterator(addrinfo *info) : _ep(info) {}
    constexpr Iterator(const Iterator &) = default;
    constexpr Iterator &operator=(const Iterator &) = default;
    constexpr ~Iterator() = default;

    constexpr auto &&operator*(this auto &&self) { return self._ep; }
    constexpr auto operator->(this auto &&self) { return &self._ep; }

    constexpr Iterator &operator++() {
      _ep = _ep.raw()->ai_next;
      return *this;
    }

    constexpr Iterator operator++(int) {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    constexpr bool operator!=(const Iterator &rhs) const { return _ep.raw() != rhs._ep.raw(); }
    constexpr bool operator==(const Iterator &rhs) const { return _ep.raw() == rhs._ep.raw(); }

  private:
    value_type _ep;
  };

  // static_assert(std::forward_iterator<Iterator>);

public:
  constexpr EndpointSet(addrinfo *info) : info(info) {}
  constexpr EndpointSet(EndpointSet &&other) : info(std::exchange(other.info, nullptr)) {}
  constexpr EndpointSet &operator=(EndpointSet &&other) {
    if (this != &other) {
      freeaddrinfo(this->info);
      this->info = other.info;
      other.info = nullptr;
    }
    return *this;
  }
  constexpr EndpointSet(const EndpointSet &) = delete;
  constexpr EndpointSet &operator=(const EndpointSet &) = delete;
  constexpr ~EndpointSet() {
    if (info) freeaddrinfo(info);
  }

  constexpr Iterator begin() const { return Iterator(info); }
  constexpr Iterator end() const { return Iterator(nullptr); }

  addrinfo *info;
};
XSL_SYS_NET_NE
#endif
