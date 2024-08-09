#include "xsl/convert.h"
#include "xsl/net.h"

#include <gtest/gtest.h>

#include <system_error>
using namespace xsl::http;
TEST(http_parse, complete) {
  ParseUnit parser;
  const char* data = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto [sz, res] = parser.parse(data, len);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(xsl::from_string_view<Method>(view.method), Method::GET);
  ASSERT_EQ(view.path, "/");
  ASSERT_EQ(xsl::from_string_view<Version>(view.version), Version::HTTP_1_1);
  ASSERT_EQ(view.headers.size(), 1);
  ASSERT_EQ(view.headers["Host"], "localhost:8080");
}
TEST(http_parse, partial) {
  ParseUnit parser;
  const char* data = "GET / HTTP/1.1\r\nHost: localhost:8080";
  size_t len = strlen(data);
  auto [sz, res] = parser.parse(data, len);
  ASSERT_FALSE(res.has_value());
  auto err = std::move(res.error());
  ASSERT_EQ(err, std::errc::resource_unavailable_try_again);
}
TEST(http_parse, invalid_format) {
  ParseUnit parser;
  const char* data = "GET / HTTP/1.1\rHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto [sz, res] = parser.parse(data, len);
  ASSERT_FALSE(res.has_value());
  auto err = std::move(res.error());
  ASSERT_EQ(err, std::errc::illegal_byte_sequence);
}
TEST(http_parse, test_version) {
  ParseUnit parser;
  const char* data = "GET / HTT/1.0\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto [sz, res] = parser.parse(data, len);
  ASSERT_FALSE(res.has_value());
  auto err = std::move(res.error());
  ASSERT_EQ(err, std::errc::illegal_byte_sequence);
}
TEST(http_parse, test_query) {
  ParseUnit parser;
  const char* data = "GET /?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto [sz, res] = parser.parse(data, len);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(view.query.size(), 2);
  ASSERT_EQ(view.query["a"], "1");
  ASSERT_EQ(view.query["b"], "2");
}
TEST(http_parse, test_query_empty) {
  ParseUnit parser;
  const char* data = "GET /?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto [sz, res] = parser.parse(data, len);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(view.query.size(), 2);
  ASSERT_EQ(view.query["a"], "1");
  ASSERT_EQ(view.query["b"], "2");
}

TEST(request_target, origin_form) {
  ParseUnit parser;
  const std::string_view example1 = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz, res] = parser.parse(example1);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(view.authority, "");
  ASSERT_EQ(view.path, "/");
  ASSERT_EQ(view.query.size(), 0);
  const std::string_view example2 = "GET /?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz2, res2] = parser.parse(example2);
  ASSERT_TRUE(res2.has_value());
  auto view2 = std::move(*res2);
  ASSERT_EQ(view2.authority, "");
  ASSERT_EQ(view2.path, "/");
  ASSERT_EQ(view2.query.size(), 2);
  ASSERT_EQ(view2.query["a"], "1");
  ASSERT_EQ(view2.query["b"], "2");
}

TEST(request_target, absolute_form) {
  ParseUnit parser;
  const std::string_view example1 = "GET http://localhost/ HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz, res] = parser.parse(example1);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(view.authority, "localhost");
  ASSERT_EQ(view.path, "/");
  ASSERT_EQ(view.query.size(), 0);
  const std::string_view example2 = "GET http://localhost/?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz2, res2] = parser.parse(example2);
  ASSERT_TRUE(res2.has_value());
  auto view2 = std::move(*res2);
  ASSERT_EQ(view2.authority, "localhost");
  ASSERT_EQ(view2.path, "/");
  ASSERT_EQ(view2.query.size(), 2);
  ASSERT_EQ(view2.query["a"], "1");
  ASSERT_EQ(view2.query["b"], "2");
}

TEST(request_target, authority_form) {
  ParseUnit parser;
  const std::string_view example1 = "GET localhost HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz, res] = parser.parse(example1);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(view.authority, "localhost");
  ASSERT_EQ(view.path, "");
  ASSERT_EQ(view.query.size(), 0);
  const std::string_view example2 = "GET localhost:8080 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz2, res2] = parser.parse(example2);
  ASSERT_TRUE(res2.has_value());
  auto view2 = std::move(*res2);
  ASSERT_EQ(view2.authority, "localhost:8080");
  ASSERT_EQ(view2.path, "");
  ASSERT_EQ(view2.query.size(), 0);
}

int main() {
  xsl::no_log();
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
