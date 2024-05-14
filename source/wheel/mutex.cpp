#include "xsl/wheel/mutex.h"
#include "xsl/wheel/wheel.h"

WHEEL_NAMESPACE_BEGIN
TryLockGuard::TryLockGuard(Mutex& m) : m(m), locked(false) { locked = m.try_lock(); }
TryLockGuard::~TryLockGuard() {
  if (locked) {
    m.unlock();
  }
}
bool TryLockGuard::is_locked() { return locked; }
bool TryLockGuard::try_lock() { return locked = m.try_lock(); }
WHEEL_NAMESPACE_END
