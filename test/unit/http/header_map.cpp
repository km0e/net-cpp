/**
 * @file header_map.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief SmallHeaderMap tests — first-wins emplace, linear-scan lookup,
 *        insertion-order iteration and the overflow path
 * @version 0.1.0
 * @date 2025-09-08
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <gtest/gtest.h>
#include <xsl/asio.h>

using namespace xsl;
using namespace xsl::asio;

TEST(header_map, emplace_and_contains) {
  SmallHeaderMap m;
  EXPECT_TRUE(m.emplace("Content-Type", "text/plain"));
  EXPECT_TRUE(m.contains("Content-Type"));
  EXPECT_FALSE(m.contains("Content-Length"));
  EXPECT_EQ(m.size(), 1uz);
}

TEST(header_map, first_wins_emplace) {
  // mirrors unordered_map::emplace: an existing key is left untouched,
  // set_header relies on this for idempotence
  SmallHeaderMap m;
  EXPECT_TRUE(m.emplace("Date", "first"));
  EXPECT_FALSE(m.emplace("Date", "second"));
  EXPECT_EQ(m.size(), 1uz);
  EXPECT_EQ(m.entry(0).value, "first");
}

TEST(header_map, string_view_and_literal_args) {
  // call sites pass string literals, string_view and std::string alike
  SmallHeaderMap m;
  std::string value = "29-char value: Mon, 16 Feb 2026 10:00:00 GMT";
  std::string_view view = "text/plain";
  EXPECT_TRUE(m.emplace("Date", value));
  EXPECT_TRUE(m.emplace("Content-Type", view));
  EXPECT_TRUE(m.emplace(std::string("Server"), std::string("xsl")));
  EXPECT_EQ(m.size(), 3uz);
  EXPECT_TRUE(m.contains("Content-Type"));
  EXPECT_TRUE(m.contains("Server"));
}

TEST(header_map, insertion_order_iteration) {
  SmallHeaderMap m;
  m.emplace("A", "1");
  m.emplace("B", "2");
  m.emplace("C", "3");
  ASSERT_EQ(m.size(), 3uz);
  EXPECT_EQ(m.entry(0).key, "A");
  EXPECT_EQ(m.entry(1).key, "B");
  EXPECT_EQ(m.entry(2).key, "C");
}

TEST(header_map, overflow_path) {
  SmallHeaderMap m;
  for (int i = 0; i < static_cast<int>(SmallHeaderMap::inline_capacity) + 4; ++i) {
    EXPECT_TRUE(m.emplace(std::format("X-H{}", i), std::to_string(i)));
  }
  EXPECT_EQ(m.size(), SmallHeaderMap::inline_capacity + 4);
  EXPECT_TRUE(m.contains("X-H3"));    // inline slot
  EXPECT_TRUE(m.contains("X-H11"));   // spilled entry
  EXPECT_FALSE(m.emplace("X-H3", "dup"));  // first-wins across the boundary
  // iteration lists all inline slots first, then the spilled entries
  for (std::size_t i = 0; i < m.size(); ++i) {
    EXPECT_EQ(m.entry(i).value, std::to_string(i));
  }
}

TEST(header_map, oversized_value_spills) {
  // values longer than the POD slot go to the overflow list, first-wins
  // still applies across both storages
  SmallHeaderMap m;
  std::string long_value(200, 'x');
  EXPECT_TRUE(m.emplace("X-Long", long_value));
  EXPECT_TRUE(m.emplace("Date", "Mon, 16 Feb 2026 10:00:00 GMT"));
  EXPECT_FALSE(m.emplace("X-Long", "dup"));
  EXPECT_EQ(m.size(), 2uz);
  // spilled entries render after the inline slots
  EXPECT_EQ(m.entry(0).key, "Date");
  EXPECT_EQ(m.entry(1).key, "X-Long");
  EXPECT_EQ(m.entry(1).value, long_value);
  EXPECT_TRUE(m.contains("X-Long"));
}

TEST(header_map, render_head_matches_to_string) {
  // render_head (frame buffer) and to_string (heap fallback) must agree
  xsl::asio::ResponsePart part;
  part.headers.emplace("Date", "Mon, 16 Feb 2026 10:00:00 GMT");
  part.headers.emplace("Content-Length", "13");
  part.headers.emplace("Content-Type", "text/plain");
  char buf[256];
  auto rendered = part.render_head(buf, sizeof buf);
  ASSERT_FALSE(rendered.empty());
  EXPECT_EQ(rendered, part.to_string());
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
