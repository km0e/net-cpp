/**
 * @file rc.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Reference counted pointer
 * @version 0.1.0
 * @date 2025-09-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_WHEEL_RC
#  define XSL_WHEEL_RC
#  include <xsl/wheel/def.h>

#  include <cstddef>
#  include <utility>

XSL_WHEEL_NB

template <class T>
class Rc {
  struct Inner {
    T value;
    std::size_t ref_count;
    template <class... Args>
    Inner(Args&&... args) : value(std::forward<Args>(args)...), ref_count(1) {}
  };

  Inner* inner_;

public:
  Rc() : inner_(nullptr) {}
  template <class... Args>
    requires std::is_constructible_v<T, Args...>
  Rc(Args&&... args) : inner_(new Inner(std::forward<Args>(args)...)) {}
  Rc(const Rc& other) : inner_(other.inner_) { ++inner_->ref_count; }
  Rc(Rc&& other) noexcept : inner_(std::exchange(other.inner_, nullptr)) {}
  ~Rc() {
    if (inner_ && --inner_->ref_count == 0) {
      delete inner_;
    }
  }
  Rc& operator=(const Rc& other) {
    if (this != &other) {
      if (inner_ && --inner_->ref_count == 0) {
        delete inner_;
      }
      inner_ = other.inner_;
      ++inner_->ref_count;
    }
    return *this;
  }
  Rc& operator=(Rc&& other) noexcept {
    if (this != &other) {
      if (inner_ && --inner_->ref_count == 0) {
        delete inner_;
      }
      inner_ = std::exchange(other.inner_, nullptr);
    }
    return *this;
  }

  T* operator->() { return &inner_->value; }
  T& operator*() { return inner_->value; }
  /// @brief check whether this Rc actually owns an object
  constexpr explicit operator bool() const noexcept { return this->inner_ != nullptr; }
};

template <class T>
Rc(T&&) -> Rc<std::remove_cvref_t<T>>;

XSL_WHEEL_NE
#endif
