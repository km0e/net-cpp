/**
 * @file compose.h
 * @brief shared_memory: shared ownership with explicit part lifecycle
 *
 * Uses custom atomic refcounting — no shared_ptr dependency.
 * Full control over construction, destruction, and deallocation.
 */
#pragma once
#ifndef XSL_WHEEL_COMPOSE
#  define XSL_WHEEL_COMPOSE

#  include <xsl/def.h>
#  include <xsl/feature.h>
#  include <xsl/type_traits.h>

#  include <atomic>
#  include <concepts>
#  include <memory>
#  include <utility>

XSL_NB

template <class Tag>
struct StorageMixin {};

template <class T>
class shared_memory {
  template <class>
  friend class shared_memory;

public:
  using value_type = T;

private:
  T* _raw = nullptr;
  std::atomic<unsigned>* _ref_count = nullptr;

  void _inc() noexcept {
    if (_ref_count) _ref_count->fetch_add(1, std::memory_order_relaxed);
  }

  void _dec() noexcept {
    if (!_ref_count) return;
    if (_ref_count->fetch_sub(1, std::memory_order_acq_rel) != 1) return;
    delete _raw;
    _raw = nullptr;
    delete _ref_count;
    _ref_count = nullptr;
  }

public:
  shared_memory() : _raw(new T{}), _ref_count(new std::atomic<unsigned>{1}) {}

  ~shared_memory() { _dec(); }

  shared_memory(const shared_memory& other) noexcept
      : _raw(other._raw), _ref_count(other._ref_count) {
    _inc();
  }
  shared_memory& operator=(const shared_memory& other) noexcept {
    if (this != &other) {
      _dec();
      _raw = other._raw;
      _ref_count = other._ref_count;
      _inc();
    }
    return *this;
  }

  template <std::derived_from<T> Derived>
  shared_memory(const shared_memory<Derived>& other) noexcept
      : _raw(other._raw), _ref_count(other._ref_count) {
    _inc();
  }

  shared_memory(shared_memory&& other) noexcept
      : _raw(std::exchange(other._raw, nullptr)),
        _ref_count(std::exchange(other._ref_count, nullptr)) {}
  shared_memory& operator=(shared_memory&& other) noexcept {
    if (this != &other) {
      _dec();
      _raw = std::exchange(other._raw, nullptr);
      _ref_count = std::exchange(other._ref_count, nullptr);
    }
    return *this;
  }

  template <std::derived_from<T> Derived>
  shared_memory(shared_memory<Derived>&& other) noexcept
      : _raw(std::exchange(other._raw, nullptr)),
        _ref_count(std::exchange(other._ref_count, nullptr)) {}

  template <class... Args>
    requires std::constructible_from<T, Args...>
  decltype(auto) emplace(Args&&... args) {
    return static_cast<T&>(*std::construct_at(_raw, std::forward<Args>(args)...));
  }

  void destroy() { std::destroy_at(_raw); }

  T* get() const { return _raw; }
  T& operator*() const { return *_raw; }

  auto operator->() {
    if constexpr (requires(T* p) { p->operator->(); }) {
      struct Chain {
        T* p_;
        auto operator->() { return p_->operator->(); }
      };
      return Chain{_raw};
    } else {
      return _raw;
    }
  }
  auto operator->() const {
    if constexpr (requires(const T* p) { p->operator->(); }) {
      struct Chain {
        const T* p_;
        auto operator->() { return p_->operator->(); }
      };
      return Chain{_raw};
    } else {
      return _raw;
    }
  }

  unsigned use_count() const noexcept {
    return _ref_count ? _ref_count->load(std::memory_order_relaxed) : 0;
  }
};

template <class Alloc, class... S>
class LocalComosite : public S... {
public:
  LocalComosite() {
    static_assert((... && std::is_default_constructible_v<S>),
                  "All base classes must be default constructible");
  }
  LocalComosite(const LocalComosite&) = default;
  LocalComosite(LocalComosite&&) noexcept = default;
  LocalComosite& operator=(const LocalComosite&) = default;
  LocalComosite& operator=(LocalComosite&&) noexcept = default;

  template <class Part, class... Args>
  decltype(auto) emplace(Args&&... args) {
    return *std::construct_at<Part>(static_cast<Part*>(this), std::forward<Args>(args)...);
  }

  template <class Part>
  void destroy() {
    std::destroy_at(static_cast<Part*>(this));
  }

  template <class... Parts>
  void abandon() {
    (destroy<Parts>(), ...);
  }
};

namespace _detail {
  template <typename, typename = void>
  struct is_allocator : std::false_type {};
  template <typename T>
  struct is_allocator<
      T, std::void_t<typename T::value_type,
                     decltype(std::declval<T&>().allocate(std::declval<std::size_t>())),
                     decltype(std::declval<T&>().deallocate(std::declval<typename T::value_type*>(),
                                                            std::declval<std::size_t>()))>>
      : std::true_type {};
}  // namespace _detail

template <class... S>
using LocalCompose = select_feature_flags_t<
    LocalComosite<Item<_detail::is_allocator<std::allocator<void>>>, Rest>, S...>;

XSL_NE
#endif
