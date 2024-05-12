#pragma once
#ifndef _XSL_WHEEL_HASH_MAP_H_
#  define _XSL_WHEEL_HASH_MAP_H_
#  include "xsl/wheel/mutex.h"
#  include "xsl/wheel/wheel.h"

#  include <unordered_map>
WHEEL_NAMESPACE_BEGIN
template <class K, class V>
// using hash_map = absl::flat_hash_map<K, V>;
using HashMap = std::unordered_map<K, V>;

template <class K, class V>
class ConcurrentHashMap {
public:
  ConcurrentHashMap() {}
  ~ConcurrentHashMap() {}
  SharedLockGuard<HashMap<K, V>> share() {
    return SharedLockGuard<HashMap<K, V>>(this->mutex, map);
  }
  LockGuard<SharedMutex, HashMap<K, V>> lock() {
    return LockGuard<SharedMutex, HashMap<K, V>>(this->mutex, map);
  }

private:
  HashMap<K, V> map;
  SharedMutex mutex;
};
WHEEL_NAMESPACE_END
#endif
