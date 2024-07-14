#pragma once
#ifndef XSL_WHEEL_MUTEX
#  define XSL_WHEEL_MUTEX
#  include "xsl/sync/def.h"

#  include <shared_mutex>
XSL_SYNC_NB

template <class T>
concept Lockable = requires(T t) {
  { t.lock() } -> std::same_as<void>;
  { t.unlock() } -> std::same_as<void>;
  { t.try_lock() } -> std::same_as<bool>;
};

template <class T>
class ShrdGuard {
public:
  ShrdGuard(std::shared_mutex& m, T& t);
  ~ShrdGuard();
  T* operator->();
  T& operator*();

private:
  std::shared_mutex& m;
  T& t;
};

template <class T>
ShrdGuard<T>::ShrdGuard(std::shared_mutex& m, T& t) : m(m), t(t) {
  // LOG_TRACE_L3(logger,"");
  m.lock_shared();
}
template <class T>
ShrdGuard<T>::~ShrdGuard() {
  // LOG_TRACE_L3(logger,"");
  m.unlock_shared();
}
template <class T>
T* ShrdGuard<T>::operator->() {
  return &t;
}
template <class T>
T& ShrdGuard<T>::operator*() {
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
  // LOG_TRACE_L3(logger,"");
  m.lock();
}
template <Lockable T, class V>
LockGuard<T, V>::~LockGuard() {
  // LOG_TRACE_L3(logger,"");
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
class ShrdRes {  // rename to SharedResource
public:
  ShrdRes() : container(), mutex() {}
  ~ShrdRes() {}
  ShrdGuard<C> lock_shared() { return ShrdGuard<C>(mutex, container); }
  LockGuard<std::shared_mutex, C> lock() {
    return LockGuard<std::shared_mutex, C>(mutex, container);
  }

private:
  C container;
  std::shared_mutex mutex;
};
XSL_SYNC_NE
#endif
