/**
 * @file spsc.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Single Producer Single Consumer Channel for coroutines
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_CORO_CHANNEL_SPSC
#  define XSL_CORO_CHANNEL_SPSC
#  include <xsl/coro/channel/def.h>
#  include <xsl/coro/def.h>
#  include <xsl/log.h>
#  include <xsl/wheel/bit.h>

#  include <atomic>
#  include <coroutine>
#  include <functional>
#  include <memory>
#  include <utility>

XSL_CORO_NB
/**
 * @brief snapshot of the queue
 *
 * @tparam T type of the queue
 */
template <class T>
struct SnapShot {
  std::atomic_size_t _ctl;
  std::size_t _local;
  T *_buffer;
  const std::size_t _size_mask;
};

static_assert(sizeof(SnapShot<void *>) <= 64,
              "SnapShot must be less than or equal to 64 bytes to ensure cache line alignment");

/// @brief Storage for the queue
template <class value_type>
class SPSCChannelStorage {
  constexpr SPSCChannelStorage(auto &&alloc, std::size_t mask, value_type *buffer)
      : _head{0, 0, buffer, mask},
        _tail{0, 0, buffer, mask},
        destructor_callback(
            [alloc = std::move(alloc), mask, buffer](std::size_t i, std::size_t n) mutable {
              using alloc_traits = std::allocator_traits<std::decay_t<decltype(alloc)>>;
              if (!std::is_trivially_destructible_v<value_type>) {  // if the type is not trivially
                                                                    // destructible
                while (i != n) {
                  alloc_traits::destroy(alloc, buffer + i);
                  i = (i + 1) & mask;
                }
              }
              alloc_traits::deallocate(alloc, buffer, mask + 1);
            }) {}

  constexpr SPSCChannelStorage(auto &&alloc, std::size_t size)
      : SPSCChannelStorage(
            std::move(alloc), size,
            std::allocator_traits<std::decay_t<decltype(alloc)>>::allocate(alloc, size + 1)) {}

public:
  /// @brief Construct a new Storage object, actual size will be 2^ceil2pow2(size + 1)
  constexpr SPSCChannelStorage(std::size_t size)
      : SPSCChannelStorage(std::allocator<value_type>(), xsl::wheel::ceil2pow2(size + 1) - 1) {}

  constexpr ~SPSCChannelStorage() {
    std::atomic_thread_fence(
        std::memory_order_acquire);  // acquire fence, ensure all previous writes are visible,
                                     // specially for the pop operation
    auto i = _head._ctl.load(std::memory_order_relaxed);
    auto n = _tail._ctl.load(std::memory_order_relaxed);
    if (i != n) {
      destructor_callback(i, n);  // call the destructor callback to destroy the objects in the
                                  // queue
    }
  }
  alignas(64) SnapShot<value_type> _head;
  alignas(64) SnapShot<value_type> _tail;
  std::function<void(std::size_t, std::size_t)> destructor_callback;
  std::atomic<std::function<void()> *> _callback = {};
};

template <class ValueType>
struct ChannelAwaiterTraits<SPSCChannelStorage<ValueType>> {
  constexpr bool await_ready(this auto &&self) noexcept {
    SnapShot<ValueType> &ep = self.storage._head;
    std::size_t const head = ep._ctl.load(std::memory_order_relaxed);
    if (head == ep._local
        && head == (ep._local = self.storage._tail._ctl.load(std::memory_order_acquire))) {
      return false;
      log_info("SPSCChannel: queue is empty, head = {}, local = {}", head, ep._local);
    }
    return true;  // if the queue is not empty, return true
  }

  template <class Promise>
  constexpr decltype(auto) await_suspend(this auto &&self, std::coroutine_handle<Promise> handle) {
    self.storage._callback.store(new std::function<void()>([handle, &self]() {
                                   /// NOTE: update the head's local tail pointer to the tail's ctl
                                   /// value
                                   self.storage._head._local
                                       = self.storage._tail._ctl.load(std::memory_order_relaxed);
                                   handle.promise().resume(handle);
                                 }),
                                 std::memory_order_release);
    self.storage._callback.notify_one();
  }

  [[nodiscard("must use the result of await_resume to confirm the signal is still alive")]]
  constexpr ValueType await_resume(this auto &&self) {
    SnapShot<ValueType> &ep = self.storage._head;
    std::size_t const head = ep._ctl.load(std::memory_order_relaxed);
    ep._ctl.store((head + 1) & ep._size_mask,
                  std::memory_order_relaxed);  // first, update the head pointer
    auto p = ep._buffer + head;
    return std::move(*p);
  }
};

template <class ValueType, std::size_t MaxElements>
struct ChannelTraits<SPSCChannelStorage<ValueType>, MaxElements> {
  using storage_type = SPSCChannelStorage<std::allocator<void>>;

  /**
   * @brief Push a value into the channel
   *
   * @param self the channel instance
   * @param args the value to be pushed into the channel
   * @return true if the value is pushed successfully, false if the channel is full
   */
  constexpr bool push(this auto &&self, auto &&...args) {
    SnapShot<ValueType> &ep = self.storage._tail;
    const std::size_t tail = ep._ctl.load(std::memory_order_relaxed);
    const std::size_t n_tail = (tail + 1) & ep._size_mask;
    if (n_tail != ep._local
        || n_tail != (ep._local = self.storage._head._ctl.load(std::memory_order_acquire))) {
      std::construct_at(ep._buffer + tail, std::forward<decltype(args)>(args)...);
      ep._ctl.store(n_tail, std::memory_order_release);
      if (auto func = self.storage._callback.exchange(nullptr, std::memory_order_acquire); func) {
        (*func)();    // call the callback to notify the receiver
        delete func;  // delete the callback
      }
      return true;
    }
    return false;  // queue is full
  }
};
XSL_CORO_NE
#endif
