#include <spdlog/spdlog.h>
#include <xsl/sync/mutex.h>
int main() {
  xsl::sync::Mutex m;
  m.lock();
  xsl::sync::TryLockGuard g(m);
  if (g.is_locked()) {
    spdlog::error("Mutex should not be locked");
  }
  m.unlock();
  g.try_lock();
  if (!g.is_locked()) {
    spdlog::error("Mutex should be locked");
  }
  return 0;
}
