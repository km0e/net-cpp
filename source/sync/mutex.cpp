#include "xsl/sync/mutex.h"
#include "xsl/sync/sync.h"

SYNC_NAMESPACE_BEGIN
TryLockGuard::TryLockGuard(Mutex& m) : m(m) { locked = m.try_lock(); }
TryLockGuard::~TryLockGuard() {
  if (locked) {
    m.unlock();
  }
}
bool TryLockGuard::is_locked() { return locked; }
bool TryLockGuard::try_lock() { return locked = m.try_lock(); }
SYNC_NAMESPACE_END
