/**
 * @file uri.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief URI parsing
 * @version 0.1
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "xsl/net/uri.h"

#include <gtest/gtest.h>
using namespace xsl::_net;

TEST(URI, AbsoluteUri) {
  std::string_view uri_str = "http://example.com/path?query=1&another=2";
  AbsoluteUri uri(uri_str);
  EXPECT_EQ(uri.scheme, "http");
  EXPECT_EQ(uri.host, "example.com");
  EXPECT_TRUE(uri.port.empty());
  EXPECT_EQ(uri.path, "/path");
  EXPECT_EQ(uri.query, "query=1&another=2");
}

TEST(URI, KVQuery) {
  std::string_view query_str = "a=1&b=2&c=3";
  KVQuery query(query_str);
  EXPECT_EQ(query.map["a"], "1");
  EXPECT_EQ(query.map["b"], "2");
  EXPECT_EQ(query.map["c"], "3");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
