/**
 * @file test_task.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/coro/executor.h"
#include "xsl/coro/task.h"

#include <gtest/gtest.h>
using namespace xsl::_coro;

Task<void> no_return_task(int &value) {
  ++value;
  co_return;
}

Task<int> return_task() { co_return 1; }

Task<void> no_return_exception_task() {
  throw std::runtime_error("error");
  co_return;
}

Task<int> return_exception_task() {
  throw std::runtime_error("error");
  co_return 1;
}

Task<void> multi_task(int &value) {
  value += co_await return_task();
  co_await no_return_task(value);
}

Task<void> exception_penetrate_task() { co_await no_return_exception_task(); }

Task<void> sync_no_return_task(int &value, std::binary_semaphore &sem) {
  ++value;
  sem.release();
  co_return;
}

Task<void> sync_multi_task(int &value, std::binary_semaphore &sem) {
  value += co_await return_task();
  co_await sync_no_return_task(value, sem);
}

TEST(Task, block) {
  int value = 0;
  no_return_task(value).block();
  ASSERT_EQ(value, 1);
  EXPECT_EQ(return_task().block(), 1);

  auto executor = std::make_shared<NewThreadExecutor>();

  std::binary_semaphore sem{0};
  sync_no_return_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 2);

  ASSERT_THROW(no_return_exception_task().block(), std::runtime_error);

  ASSERT_THROW(return_exception_task().block(), std::runtime_error);

  multi_task(value).block();
  ASSERT_EQ(value, 4);

  sync_multi_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 6);

  ASSERT_THROW(exception_penetrate_task().block(), std::runtime_error);
}

TEST(Task, detach) {
  int value = 0;
  auto executor = std::make_shared<NewThreadExecutor>();

  std::binary_semaphore sem{0};
  sync_no_return_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 1);

  sync_multi_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 3);
}

TEST(Task, then) {
  int value = 0;
  no_return_task(value).then([&value] { value += 1; }).block();
  ASSERT_EQ(value, 2);

  return_task().then([&value](int v) { value += v; }).block();
  ASSERT_EQ(value, 3);

  multi_task(value).then([&value]() { value += 1; }).block();
  ASSERT_EQ(value, 6);

  auto executor = std::make_shared<NewThreadExecutor>();

  std::binary_semaphore sem{0};
  sync_no_return_task(value, sem).then([&value] { value += 1; }).detach(executor);
  sem.acquire();
  ASSERT_EQ(value, 8);

  sync_multi_task(value, sem).then([&value]() { value += 1; }).detach(executor);
  sem.acquire();
  ASSERT_EQ(value, 11);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
