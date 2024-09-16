/**
 * @file test_spsc.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Test for xsl::spsc.
 * @version 0.1
 * @date 2024-08-23
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/sync.h"

#include <gtest/gtest.h>

#include <memory>
#include <thread>
#include <vector>
using namespace xsl;

TEST(spsc, constructor) {
  auto s = spsc<int>(500);
  auto [r, w] = std::move(s).split();
  auto s1 = spsc<int>(500, std::allocator<int>());
  auto [r1, w1] = std::move(s1).split();
}

TEST(spsc, concurrent) {
  auto s = spsc<int>(500);
  auto [r, w] = std::move(s).split();
  std::vector<int> read{}, write{};
  std::thread reader([&] {
    for (int i = 0; i < 1000; i++) {
      int v = 0;
      if (r.pop(v)) {
        read.push_back(v);
      }
    }
  });
  std::thread writer([&] {
    for (int i = 0; i < 1000; i++) {
      if (w.push(i)) {
        write.push_back(i);
      }
    }
  });
  reader.join();
  writer.join();
  for (auto i{0u}; i < write.size(); i++) {
    EXPECT_EQ(read[i], write[i]);
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
