#pragma once
#ifndef _XSL_SYNC_MUTEX_H_
#  define _XSL_SYNC_MUTEX_H_
#  include <xsl/sync/sync.h>

#  include <mutex>
SYNC_NAMESPACE_BEGIN
using Mutex = std::mutex;
class TryLockGuard {
public:
  TryLockGuard(Mutex& m);
  ~TryLockGuard();
  bool is_locked();
  bool try_lock();

private:
  Mutex& m;
  bool locked;
};
SYNC_NAMESPACE_END
#endif
