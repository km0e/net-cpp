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
#include "CLI/CLI.hpp"
#include "xsl/coro.h"

#include <gtest/gtest.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
using namespace xsl;

std::size_t TEST_COUNT = 1;

Task<int> bar(std::unique_ptr<int>& ptr) {
  int res = *ptr + 1;
  co_return res;
}

auto unsafe_bar() {
  auto ptr = std::make_unique<int>(42);
  return bar(ptr);
}
auto safe_bar() {
  auto ptr = std::make_unique<int>(42);
  return ArgGuard{bar, std::move(ptr)};
}

TEST(ArgGuard, Basic) {
  auto N = TEST_COUNT;
  while (N--) {
    auto unsafe = unsafe_bar();
    auto safe = safe_bar();
    ASSERT_NE(std::move(unsafe).block(), 43);
    ASSERT_EQ(block(std::move(safe)), 43);
  }
}

int main(int argc, char** argv) {
  CLI::App app{"Echo server"};
  app.add_option("-c,--count", TEST_COUNT, "Test count");
  CLI11_PARSE(app, argc, argv);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
