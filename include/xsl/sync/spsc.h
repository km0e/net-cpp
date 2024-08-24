/**
 * @file spsc.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Single producer single consumer queue.
 * @version 0.2
 * @date 2024-08-23
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_WHEEL_SPSC
#  define XSL_WHEEL_SPSC
#  include "xsl/sync/def.h"
#  include "xsl/wheel/bit.h"

#  include <atomic>
#  include <cassert>
#  include <cstddef>
#  include <memory>
#  include <utility>
XSL_SYNC_NB
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
  // const char _padding[64 - sizeof(std::atomic_size_t) - sizeof(std::size_t) - sizeof(T *)
  //                     - sizeof(std::size_t)];
};
/**
 * @brief storage of the queue
 *
 * @tparam Alloc allocator
 */
template <class Alloc>
class Storage {
  using alloc_traits = std::allocator_traits<Alloc>;
  using value_type = typename alloc_traits::value_type;

  template <class _Alloc>
  constexpr Storage(_Alloc &&alloc, std::size_t size)
      : _head{0, 0, nullptr, size},
        _tail{0, 0, nullptr, size},
        _alloc(std::forward<_Alloc>(alloc)) {
    _head._buffer = alloc_traits::allocate(_alloc, size + 1);
    _tail._buffer = _head._buffer;
  }

public:
  /**
   * @brief Construct a new Storage object
   *
   * @tparam _Alloc
   * @param size size of the queue, actual size will be 2^ceil2pow2(size + 1)
   */
  template <class _Alloc>
  constexpr Storage(std::size_t size, _Alloc &&alloc)
      : Storage(std::forward<_Alloc>(alloc), xsl::wheel::ceil2pow2(size + 1) - 1) {}
  constexpr Storage(std::size_t size) : Storage(Alloc(), xsl::wheel::ceil2pow2(size + 1) - 1) {}

  ~Storage() {
    if (!std::is_trivially_destructible_v<value_type>) {  // if the type is not trivially
                                                          // destructible
      std::atomic_thread_fence(
          std::memory_order_acquire);  // acquire fence, ensure all previous writes are visible,
                                       // specially for the pop operation
      auto i = _head._ctl.load(std::memory_order_relaxed);
      auto n = _tail._ctl.load(std::memory_order_relaxed);
      while (i != n) {
        alloc_traits::destroy(_alloc, _head._buffer + i);
        i = (i + 1) & _head._size_mask;
      }
    }
    alloc_traits::deallocate(_alloc, _head._buffer, _head._size_mask + 1);
  }
  alignas(64) SnapShot<value_type> _head;
  alignas(64) SnapShot<value_type> _tail;
  Alloc _alloc;
};

template <class Alloc>
struct RxTraits {
  using storage_type = Storage<Alloc>;
  using alloc_traits = std::allocator_traits<Alloc>;
  using value_type = typename alloc_traits::value_type;
  /**
   * @brief push a value to the queue
   *
   * @tparam Args
   * @param storage
   * @param args
   * @return true if the value is pushed successfully
   * @return false if the queue is full
   */
  template <class... Args>
  static bool push(storage_type &storage, Args &&...args) {
    SnapShot<value_type> &ep = storage._tail;
    const std::size_t tail = ep._ctl.load(std::memory_order_relaxed);
    const std::size_t n_tail = (tail + 1) & ep._size_mask;
    if (n_tail != ep._local
        || n_tail != (ep._local = storage._head._ctl.load(std::memory_order_acquire))) {
      alloc_traits::construct(storage._alloc, ep._buffer + tail, std::forward<Args>(args)...);
      ep._ctl.store(n_tail, std::memory_order_release);
      return true;
    }
    return false;
  }
};

template <class Alloc>
struct TxTraits {
  using storage_type = Storage<Alloc>;
  using alloc_traits = std::allocator_traits<Alloc>;
  using value_type = typename alloc_traits::value_type;
  using pointer = typename alloc_traits::pointer;
  /**
   * @brief pop a value from the queue
   *
   * @tparam _Tp
   * @param storage
   * @param v
   * @return true if the value is popped successfully
   * @return false if the queue is empty
   */
  template <class _Tp>
  static bool pop(storage_type &storage, _Tp &v) {
    SnapShot<value_type> &ep = storage._head;
    std::size_t const head = ep._ctl.load(std::memory_order_relaxed);
    if (head == ep._local
        && head == (ep._local = storage._tail._ctl.load(std::memory_order_acquire))) {
      return false;
    }
    ep._ctl.store((head + 1) & ep._size_mask,
                  std::memory_order_release);  // first, update the head pointer
    pointer p = ep._buffer + head;
    v = std::move(*p);
    alloc_traits::destroy(storage._alloc, p);
    return true;
  }
};
/**
 * @brief Single producer queue
 *
 * @tparam Alloc allocator
 */
template <class Alloc>
class Rx {
public:
  template <class _Storage>
  Rx(_Storage &&storage) : _storage(std::forward<_Storage>(storage)) {}
  /**
   * @brief push a value to the queue
   *
   * @tparam _Tp
   * @param v
   * @return true if the value is pushed successfully
   * @return false if the queue is full
   */
  template <class _Tp>
  bool pop(_Tp &v) {
    return RxTraits<Alloc>::push(*_storage, v);
  }

private:
  std::shared_ptr<Storage<Alloc>> _storage;
};
/**
 * @brief Single consumer queue
 *
 * @tparam Alloc
 */
template <class Alloc>
class Tx {
public:
  template <class _Storage>
  Tx(_Storage &&storage) : _storage(std::forward<_Storage>(storage)) {}
  /**
   * @brief pop a value from the queue
   *
   * @tparam Args
   * @param args
   * @return true if the value is popped successfully
   * @return false if the queue is empty
   */
  template <class... Args>
  bool push(Args &&...args) {
    return TxTraits<Alloc>::pop(*_storage, std::forward<Args>(args)...);
  }

private:
  std::shared_ptr<Storage<Alloc>> _storage;
};
/**
 * @brief Single producer single consumer queue
 *
 * @tparam T
 * @tparam Alloc
 */
template <class T, class Alloc = std::allocator<T>>
struct spsc {
  using storage_type = Storage<Alloc>;
  using rx_traits = RxTraits<Alloc>;
  using tx_traits = TxTraits<Alloc>;
  /**
   * @brief Construct a new spsc object
   *
   * @param size size of the queue, actual size will be 2^ceil2pow2(size + 1)
   */
  spsc(std::size_t size) : _storage(std::make_shared<Storage<Alloc>>(size)) {}
  /**
   * @brief Construct a new spsc object
   *
   * @tparam _Alloc allocator
   * @param size size of the queue, actual size will be 2^ceil2pow2(size + 1)
   * @param alloc allocator
   */
  template <class _Alloc>
  spsc(std::size_t size, _Alloc &&alloc)
      : _storage(std::make_shared<Storage<Alloc>>(size, std::forward<_Alloc>(alloc))) {}

  ~spsc() {}
  /**
   * @brief push a value to the queue
   *
   * @tparam Args
   * @param args
   * @return true if the value is pushed successfully
   * @return false if the queue is full
   */
  template <class... Args>
  bool push(Args &&...args) {
    return rx_traits::push(*_storage, std::forward<Args>(args)...);
  }
  /**
   * @brief pop a value from the queue
   *
   * @tparam _Tp
   * @param v
   * @return true if the value is popped successfully
   * @return false if the queue is empty
   */
  template <class _Tp>
  bool pop(_Tp &v) {
    return tx_traits::pop(*_storage, v);
  }
  /**
   * @brief split the queue into two parts, one for producer, one for consumer
   *
   * @return std::pair<Rx<Alloc>, Tx<Alloc>>
   */
  std::pair<Rx<Alloc>, Tx<Alloc>> split() && noexcept {
    auto copy = _storage;
    return {Rx<Alloc>(std::move(copy)), Tx<Alloc>(std::move(_storage))};
  }

private:
  std::shared_ptr<storage_type> _storage;
};

XSL_SYNC_NE
#endif
