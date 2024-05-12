#include "xsl/wheel/mutex.h"

#include <spdlog/spdlog.h>
int main() {
  xsl::wheel::Mutex m;
  m.lock();
  xsl::wheel::TryLockGuard g(m);
  if (g.is_locked()) {
    SPDLOG_ERROR("Mutex should not be locked");
  }
  m.unlock();
  g.try_lock();
  if (!g.is_locked()) {
    SPDLOG_ERROR("Mutex should be locked");
  }
  return 0;
}
