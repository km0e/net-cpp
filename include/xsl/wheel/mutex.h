#pragma once
#ifndef _XSL_WHEEL_MUTEX_H_
#  define _XSL_WHEEL_MUTEX_H_
#  include "xsl/wheel/def.h"

#  include <spdlog/spdlog.h>

#  include <mutex>
#  include <shared_mutex>
WHEEL_NAMESPACE_BEGIN

template <class T>
concept Lockable = requires(T t) {
  { t.lock() } -> std::same_as<void>;
  { t.unlock() } -> std::same_as<void>;
  { t.try_lock() } -> std::same_as<bool>;
};

using Mutex = std::mutex;

using SharedMutex = std::shared_mutex;
template <class T>
class SharedLockGuard {
public:
  SharedLockGuard(SharedMutex& m, T& t);
  ~SharedLockGuard();
  T* operator->();
  T& operator*();

private:
  SharedMutex& m;
  T& t;
};

template <class T>
SharedLockGuard<T>::SharedLockGuard(SharedMutex& m, T& t) : m(m), t(t) {
  // SPDLOG_TRACE("");
  m.lock_shared();
}
template <class T>
SharedLockGuard<T>::~SharedLockGuard() {
  // SPDLOG_TRACE("");
  m.unlock_shared();
}
template <class T>
T* SharedLockGuard<T>::operator->() {
  return &t;
}
template <class T>
T& SharedLockGuard<T>::operator*() {
  return t;
}

template <Lockable T, class V>
class LockGuard {
public:
  LockGuard(T& m, V& v);
  ~LockGuard();
  V* operator->();
  V& operator*();

private:
  T& m;
  V& v;
};

template <Lockable T, class V>
LockGuard<T, V>::LockGuard(T& m, V& v) : m(m), v(v) {
  // SPDLOG_TRACE("");
  m.lock();
}
template <Lockable T, class V>
LockGuard<T, V>::~LockGuard() {
  // SPDLOG_TRACE("");
  m.unlock();
}
template <Lockable T, class V>
V* LockGuard<T, V>::operator->() {
  return &v;
}
template <Lockable T, class V>
V& LockGuard<T, V>::operator*() {
  return v;
}
template <class C>
class ShareContainer {
public:
  ShareContainer() : container(), mutex() {}
  ~ShareContainer() {}
  SharedLockGuard<C> lock_shared() { return SharedLockGuard<C>(mutex, container); }
  LockGuard<SharedMutex, C> lock() { return LockGuard<SharedMutex, C>(mutex, container); }

private:
  C container;
  SharedMutex mutex;
};
WHEEL_NAMESPACE_END
#endif
