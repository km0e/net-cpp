#pragma once
#ifndef _XSL_WHEEL_HASH_MAP_H_
#  define _XSL_WHEEL_HASH_MAP_H_
#  include "xsl/wheel/giant.h"
#  include "xsl/wheel/mutex.h"

WHEEL_NAMESPACE_BEGIN

template <class K, class V>
class ConcurrentHashMap {
public:
  ConcurrentHashMap() : map(), mutex() {}
  ~ConcurrentHashMap() {}
  SharedLockGuard<giant::unordered_map<K, V>> share() {
    return SharedLockGuard<giant::unordered_map<K, V>>(this->mutex, map);
  }
  LockGuard<SharedMutex, giant::unordered_map<K, V>> lock() {
    return LockGuard<SharedMutex, giant::unordered_map<K, V>>(this->mutex, map);
  }

private:
  giant::unordered_map<K, V> map;
  SharedMutex mutex;
};
WHEEL_NAMESPACE_END
#endif
