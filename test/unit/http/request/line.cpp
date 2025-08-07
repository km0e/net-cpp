/**
 * @file line.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP request target parsing
 * @version 0.1.0
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <gtest/gtest.h>
#include <xsl/net/http/request/line.h>
using namespace xsl::_net::http;

TEST(RequestLineTest, ParseValidLine) {
  RequestLine line;
  std::string_view input = "GET /index.html HTTP/1.1\r\n";
  auto [pos, err] = line.parse(input);
  EXPECT_EQ(pos, input.size());
  EXPECT_EQ(err, xsl::errc{});
  EXPECT_EQ(line.method, Method::GET);
  EXPECT_EQ(line.path, "/index.html");
  EXPECT_EQ(line.version, "HTTP/1.1");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
