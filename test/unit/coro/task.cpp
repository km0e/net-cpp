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
#include "coro/tool.h"
#include "xsl/coro/executor.h"

#include <gtest/gtest.h>
using namespace xsl::_coro;

TEST(Task, just_return) {
  int value = 0;
  no_return_task(value).block();
  ASSERT_EQ(value, 1);
  EXPECT_EQ(return_task().block(), 1);

  auto executor = std::make_shared<NewThreadExecutor>();

  std::binary_semaphore sem{0};
  sync_no_return_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 2);
}

TEST(Task, just_throw) {
  ASSERT_THROW(no_return_exception_task().block(), std::runtime_error);

  ASSERT_THROW(return_exception_task().block(), std::runtime_error);
}

TEST(Task, multi_task) {
  int value = 0;
  multi_task(value).block();
  ASSERT_EQ(value, 2);

  auto executor = std::make_shared<NewThreadExecutor>();

  std::binary_semaphore sem{0};
  sync_multi_task(value, sem).detach(executor);
  sem.acquire();
  EXPECT_EQ(value, 4);
}

TEST(Task, penetrate_exception) {
  ASSERT_THROW(exception_penetrate_task().block(), std::runtime_error);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
