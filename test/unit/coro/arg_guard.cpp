/**
 * @file arg_guard.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-23
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/coro.h"

#include <gtest/gtest.h>

#include <cassert>
#include <cstddef>
#include <memory>
using namespace xsl;

bool safe = false;

const size_t ALIVE_MAGIC = 0xdeadbeef;

struct CheckDestructor {
  std::unique_ptr<size_t> num = std::make_unique<size_t>(ALIVE_MAGIC);
  CheckDestructor() = default;
  CheckDestructor(CheckDestructor&&) = default;
  CheckDestructor(const CheckDestructor&) = delete;
  ~CheckDestructor() { this->num = 0; }
};

Task<size_t> bar(CheckDestructor& c) {
  auto magic = *c.num;
  co_return magic;
}

auto unsafe_bar() {
  auto c = CheckDestructor{};
  return bar(c);
}

auto safe_bar() { return ArgGuard{bar, CheckDestructor{}}; }

TEST(ArgGuard, Basic) {
  log_info("safe mode");
  ASSERT_EQ(block(safe_bar()), ALIVE_MAGIC);
  log_info("unsafe mode");
  ASSERT_NE(unsafe_bar().block(), ALIVE_MAGIC);  /// will crash
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
