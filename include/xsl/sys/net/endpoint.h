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
  Endpoint(addrinfo *info) : info(info) {}
  addrinfo *raw() const { return info; }

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

    Iterator() : _ep(nullptr) {}
    Iterator(addrinfo *info) : _ep(info) {}
    Iterator(const Iterator &) = default;
    Iterator &operator=(const Iterator &) = default;
    ~Iterator() = default;

    auto &&operator*(this auto &&self) { return self._ep; }
    auto operator->(this auto &&self) { return &self._ep; }

    Iterator &operator++() {
      _ep = _ep.raw()->ai_next;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    bool operator!=(const Iterator &rhs) const { return _ep.raw() != rhs._ep.raw(); }
    bool operator==(const Iterator &rhs) const { return _ep.raw() == rhs._ep.raw(); }

  private:
    value_type _ep;
  };

  // static_assert(std::forward_iterator<Iterator>);

public:
  EndpointSet(addrinfo *info) : info(info) {}
  EndpointSet(EndpointSet &&other) : info(std::exchange(other.info, nullptr)) {}
  EndpointSet &operator=(EndpointSet &&other) {
    if (this != &other) {
      freeaddrinfo(this->info);
      this->info = other.info;
      other.info = nullptr;
    }
    return *this;
  }
  EndpointSet(const EndpointSet &) = delete;
  EndpointSet &operator=(const EndpointSet &) = delete;
  ~EndpointSet() {
    if (info) freeaddrinfo(info);
  }

  Iterator begin() const { return Iterator(info); }
  Iterator end() const { return Iterator(nullptr); }

  addrinfo *info;
};
XSL_SYS_NET_NE
#endif
