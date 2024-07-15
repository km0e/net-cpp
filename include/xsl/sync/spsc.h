#pragma once
#ifndef XSL_WHEEL_SPSC
#  define XSL_WHEEL_SPSC
#  include "xsl/sync/def.h"

#  include <atomic>
#  include <cstddef>
#  include <optional>
XSL_SYNC_NB
template <class T, std::size_t N = 1024>
class SPSC {
public:
  static_assert(N > 0, "SPSC buffer size must be greater than 1");

  SPSC() : _buffer(), _head(0), _tail(0) {}
  ~SPSC() {}

  bool push(T&& t) {
    auto head = _head.load(std::memory_order_relaxed);
    auto next_head = (head + 1) % (N + 1);
    if (next_head == _tail.load(std::memory_order_acquire)) {
      return false;
    }
    _buffer[head] = std::move(t);
    _head.store(next_head, std::memory_order_release);
    return true;
  }

  std::optional<T> pop() {
    auto tail = _tail.load(std::memory_order_relaxed);
    if (tail == _head.load(std::memory_order_acquire)) {
      return std::nullopt;
    }
    auto t = std::move(_buffer[tail]);
    _tail.store((tail + 1) % (N + 1), std::memory_order_release);
    return std::make_optional(std::move(t));
  }

  std::size_t size() const {
    auto head = _head.load(std::memory_order_relaxed);
    auto tail = _tail.load(std::memory_order_relaxed);
    return (head + N + 1 - tail) % (N + 1);
  }

private:
  T _buffer[N];
  std::atomic<std::size_t> _head;
  std::atomic<std::size_t> _tail;
};
XSL_SYNC_NE
#endif
