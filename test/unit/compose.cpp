/**
 * @file compose2.cpp
 * @brief Tests for shared_memory and LocalCompose
 */
#include <gtest/gtest.h>
#include <xsl/compose.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace xsl;

// ---------------------------------------------------------------------------
// Test types
// ---------------------------------------------------------------------------
struct Widget {
  int value = 0;
  std::string name;
};

struct DtorTracker {
  inline static std::atomic<int> count{0};
  ~DtorTracker() { count.fetch_add(1, std::memory_order_relaxed); }
};

struct TestFd {
  int fd = -1;
  explicit TestFd(int f) : fd(f) {}
  TestFd() = default;
  ~TestFd() {
    if (fd >= 0) fd = -1;
  }
  int raw() const { return fd; }
};

// ---------------------------------------------------------------------------
// shared_memory<Alloc, T> — single type
// ---------------------------------------------------------------------------
using WidgetMem = shared_memory<Widget>;

TEST(SharedMemory, DefaultConstruction) {
  WidgetMem mem;
  EXPECT_NE(mem.get(), nullptr);
  EXPECT_EQ(mem.use_count(), 1);
  EXPECT_EQ(mem->value, 0);
}

TEST(SharedMemory, EmplaceAndAccess) {
  WidgetMem mem;
  mem.emplace(42, "hello");

  EXPECT_EQ(mem->value, 42);
  EXPECT_EQ(mem->name, "hello");
}

TEST(SharedMemory, EmplaceIdempotent) {
  WidgetMem mem;
  mem.emplace(1, "a");
  mem.emplace(2);
  EXPECT_EQ(mem->value, 2);
  EXPECT_TRUE(mem->name.empty());
}

TEST(SharedMemory, DestroyThenEmplace) {
  WidgetMem mem;
  mem.emplace(42, "world");
  mem.destroy();
  mem.emplace();
  EXPECT_EQ(mem->value, 0);
  EXPECT_TRUE(mem->name.empty());
}

// ---------------------------------------------------------------------------
// shared ownership
// ---------------------------------------------------------------------------
TEST(SharedMemory, CopySharesOwnership) {
  WidgetMem a;
  a.emplace(42);
  {
    WidgetMem b = a;
    EXPECT_EQ(a.use_count(), 2);
    b.emplace(99);
    EXPECT_EQ(a->value, 99);
  }
  EXPECT_EQ(a.use_count(), 1);
  EXPECT_EQ(a->value, 99);
}

TEST(SharedMemory, MoveTransfersOwnership) {
  WidgetMem a;
  a.emplace(42);
  WidgetMem b = std::move(a);
  EXPECT_EQ(b.use_count(), 1);
  EXPECT_EQ(b->value, 42);
}

TEST(SharedMemory, SelfAssignment) {
  WidgetMem mem;
  mem.emplace(5);
  mem = mem;
  EXPECT_EQ(mem->value, 5);
  EXPECT_EQ(mem.use_count(), 1);
}

// ---------------------------------------------------------------------------
// thread-safe refcounting
// ---------------------------------------------------------------------------
TEST(SharedMemory, ThreadSafeRefcount) {
  WidgetMem a;
  std::atomic<bool> done{false};
  auto copier = [&] {
    std::vector<WidgetMem> copies;
    while (!done.load(std::memory_order_relaxed)) {
      copies.push_back(a);
      copies.clear();
    }
  };
  std::thread t1(copier), t2(copier);
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  done.store(true, std::memory_order_relaxed);
  t1.join();
  t2.join();
  EXPECT_EQ(a.use_count(), 1);
}

// ---------------------------------------------------------------------------
// safety
// ---------------------------------------------------------------------------
TEST(SharedMemory, DoubleDestroySafe) {
  using FdMem = shared_memory<TestFd>;
  FdMem mem;
  mem.emplace(-1);
  mem.destroy();
}

TEST(SharedMemory, DestroyDtorCount) {
  using DMem = shared_memory<DtorTracker>;
  DtorTracker::count = 0;
  {
    DMem mem;
    mem.emplace();  // destroy default → +1
    mem.destroy();  // destroy current → +1; reconstruct default
  }
  EXPECT_EQ(DtorTracker::count.load(), 2);
}

// ---------------------------------------------------------------------------
// LocalCompose: multi-part inline composition
// LocalComosite<S...> inherits directly from all S, so members are accessed
// directly without get() or operator->
// ---------------------------------------------------------------------------
using LC2Direct = LocalComosite<std::allocator<void>, TestFd, Widget>;
using LC2Alias = LocalCompose<TestFd, Widget>;

TEST(LocalCompose, DefaultConstructionDirect) {
  LC2Direct lc;
  // Direct inheritance: raw() from TestFd, value from Widget
  EXPECT_EQ(lc.raw(), -1);
  EXPECT_EQ(lc.value, 0);
}

TEST(LocalCompose, EmplacePart) {
  LC2Direct lc;
  lc.emplace<TestFd>(42);
  EXPECT_EQ(lc.raw(), 42);
  EXPECT_EQ(lc.value, 0);  // Widget still default
}

TEST(LocalCompose, EmplaceMultipleParts) {
  LC2Direct lc;
  lc.emplace<TestFd>(5);
  lc.emplace<Widget>(99, "hi");
  EXPECT_EQ(lc.raw(), 5);
  EXPECT_EQ(lc.value, 99);
  EXPECT_EQ(lc.name, "hi");
}

TEST(LocalCompose, EmplaceIdempotent) {
  LC2Direct lc;
  lc.emplace<TestFd>(1);
  lc.emplace<TestFd>(2);
  EXPECT_EQ(lc.raw(), 2);
}

TEST(LocalCompose, DestroyPart) {
  LC2Direct lc;
  lc.emplace<TestFd>(42);
  lc.emplace<Widget>(99, "test");

  lc.destroy<TestFd>();
  lc.emplace<TestFd>();
  EXPECT_EQ(lc.raw(), -1);  // back to default after emplace
  EXPECT_EQ(lc.value, 99);  // Widget still alive
}

TEST(LocalCompose, AbandonParts) {
  LC2Direct lc;
  lc.emplace<TestFd>(10);
  lc.emplace<Widget>(20);

  lc.abandon<TestFd, Widget>();
  lc.emplace<TestFd>();
  lc.emplace<Widget>();
  EXPECT_EQ(lc.raw(), -1);
  EXPECT_EQ(lc.value, 0);
}

TEST(LocalCompose, AbandonThenReemplace) {
  LC2Direct lc;
  lc.emplace<Widget>(10);
  lc.abandon<Widget>();
  lc.emplace<Widget>(20);
  EXPECT_EQ(lc.value, 20);
}

TEST(LocalCompose, SharedMemoryWrapper) {
  // shared_memory + LocalCompose — access via -> goes to LocalComosite*
  // (no operator->() on LocalComosite, so shared_memory::op-> returns T*)
  using T = shared_memory<LC2Direct>;
  T sm;
  // sm-> chains through shared_memory::op-> → LocalComosite*
  EXPECT_EQ(sm->raw(), -1);
  EXPECT_EQ(sm->value, 0);
  // emplace via get()
  sm.get()->emplace<TestFd>(42);
  EXPECT_EQ(sm->raw(), 42);
}
