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
  LockGuard(T& m, V& v) : lock(m), v(v) {}
  ~LockGuard() {}
  decltype(auto) operator->(this auto&& self) { return &self.v; }
  decltype(auto) operator*(this auto&& self) { return self.v; }

private:
  std::lock_guard<T> lock;
  V& v;
};

template <class T>
class UnqRes {
public:
  UnqRes() : src(), mutex() {}
  UnqRes(T&& src) : src(std::move(src)), mutex() {}
  ~UnqRes() {}
  decltype(auto) lock(this auto&& self) { return LockGuard<std::mutex, T>(self.mutex, self.src); }

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
  ShardGuard(std::shared_mutex& m, T& t) : m(m), t(t) { m.lock_shared(); }
  ~ShardGuard() { m.unlock_shared(); }
  decltype(auto) operator->(this auto&& self) { return &self.t; }
  decltype(auto) operator*(this auto&& self) { return self.t; }

private:
  std::shared_mutex& m;
  T& t;
};

template <class R>
class ShardRes {  // rename to SharedResource
public:
  ShardRes() : src(), mutex() {}
  ShardRes(R&& src) : src(std::move(src)), mutex() {}
  ~ShardRes() {}
  decltype(auto) lock_shared(this auto&& self) { return ShardGuard<R>(self.mutex, self.src); }
  decltype(auto) lock(this auto&& self) {
    return LockGuard<std::shared_mutex, R>(self.mutex, self.src);
  }

private:
  R src;
  std::shared_mutex mutex;
};
XSL_SYNC_NE
#endif
