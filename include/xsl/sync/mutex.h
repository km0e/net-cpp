/**
 * @file mutex.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_WHEEL_MUTEX
#  define XSL_WHEEL_MUTEX
#  include "xsl/sync/def.h"

#  include <mutex>
#  include <shared_mutex>
XSL_SYNC_NB

template <class T>
concept BasicLockable = requires(T t) {
  { t.lock() } -> std::same_as<void>;
  { t.unlock() } -> std::same_as<void>;
};
template <BasicLockable T, class V>
class LockGuard {
public:
  constexpr LockGuard(T& m, V& v) : lock(m), v(v) {}
  constexpr ~LockGuard() {}
  constexpr decltype(auto) operator->(this auto&& self) { return &self.v; }
  constexpr decltype(auto) operator*(this auto&& self) { return self.v; }

private:
  std::lock_guard<T> lock;
  V& v;
};

template <class T>
class UnqRes {
public:
  constexpr UnqRes() : src(), mutex() {}
  constexpr UnqRes(T&& src) : src(std::move(src)), mutex() {}
  constexpr ~UnqRes() {}
  constexpr decltype(auto) lock(this auto&& self) { return LockGuard<std::mutex, T>(self.mutex, self.src); }

private:
  T src;
  std::mutex mutex;
};

template <class T>
concept Lockable = BasicLockable<T> && requires(T t) {
  { t.try_lock() } -> std::same_as<bool>;
};

template <class T>
class ShardGuard {
public:
  constexpr ShardGuard(std::shared_mutex& m, T& t) : m(m), t(t) { m.lock_shared(); }
  constexpr ~ShardGuard() { m.unlock_shared(); }
  constexpr decltype(auto) operator->(this auto&& self) { return &self.t; }
  constexpr decltype(auto) operator*(this auto&& self) { return self.t; }

private:
  std::shared_mutex& m;
  T& t;
};

template <class R>
class ShardRes {  // rename to SharedResource
public:
  constexpr ShardRes() : src(), mutex() {}
  constexpr ShardRes(R&& src) : src(std::move(src)), mutex() {}
  constexpr ShardRes(auto&&... args) : src(std::forward<decltype(args)>(args)...), mutex() {}
  constexpr ~ShardRes() {}
  constexpr decltype(auto) lock_shared(this auto&& self) {
    return ShardGuard<R>(self.mutex, self.src);
  }
  constexpr decltype(auto) lock(this auto&& self) {
    return LockGuard<std::shared_mutex, R>(self.mutex, self.src);
  }

private:
  R src;
  std::shared_mutex mutex;
};
XSL_SYNC_NE
#endif
