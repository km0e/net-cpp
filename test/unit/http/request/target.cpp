/**
 * @file target.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP request target parsing
 * @version 0.1.0
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <gtest/gtest.h>
#include <xsl/net/http/request/target.h>
using namespace xsl::_net::http;

TEST(HttpRequestTarget, Target) {
  // Test OriginForm
  {
    std::string_view target_str = "/path/to/resource?query=1&another=2";
    RequestTarget target(target_str);
    OriginForm *ptr = target.origin_form();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->path, "/path/to/resource");
    EXPECT_EQ(ptr->query, "query=1&another=2");
  }
  // Test AbsoluteForm
  {
    std::string_view target_str = "http://example.com/path?query=1&another=2";
    RequestTarget target(target_str);
    AbsoluteForm *ptr = target.absolute_form();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->scheme, "http");
    EXPECT_EQ(ptr->host, "example.com");
    EXPECT_TRUE(ptr->port.empty());
    EXPECT_EQ(ptr->path, "/path");
    EXPECT_EQ(ptr->query, "query=1&another=2");
  }
  // Test AuthorityForm
  {
    std::string_view target_str = "example.com:8080";
    RequestTarget target(target_str);
    AuthorityForm *ptr = target.authority_form();
    ASSERT_NE(ptr, nullptr) << target.target.index();
    EXPECT_EQ(ptr->host, "example.com");
    EXPECT_EQ(ptr->port, "8080");
  }
  // Test AsteriskForm
  {
    std::string_view target_str = "*";
    RequestTarget target(target_str);
    AsteriskForm *ptr = target.asterisk_form();
    ASSERT_NE(ptr, nullptr);
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
