#include "xsl/convert.h"
#include "xsl/net/http.h"

#include <gtest/gtest.h>

#include <system_error>
using namespace xsl::net;
TEST(http_parse, complete) {
  HttpParser parser;
  const char* data = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(xsl::from_string_view<HttpMethod>(view.method), HttpMethod::GET);
  ASSERT_EQ(view.url, "/");
  ASSERT_EQ(xsl::from_string_view<HttpVersion>(view.version), HttpVersion::HTTP_1_1);
  ASSERT_EQ(view.headers.size(), 1);
  ASSERT_EQ(view.headers["Host"], "localhost:8080");
}
TEST(http_parse, partial) {
  HttpParser parser;
  const char* data = "GET / HTTP/1.1\r\nHost: localhost:8080";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_FALSE(res.has_value());
  auto err = std::move(res.error());
  ASSERT_EQ(err, std::errc::resource_unavailable_try_again);
}
TEST(http_parse, invalid_format) {
  HttpParser parser;
  const char* data = "GET / HTTP/1.1\rHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_FALSE(res.has_value());
  auto err = std::move(res.error());
  ASSERT_EQ(err, std::errc::illegal_byte_sequence);
}
TEST(http_parse, test_version) {
  HttpParser parser;
  const char* data = "GET / HTT/1.0\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_FALSE(res.has_value());
  auto err = std::move(res.error());
  ASSERT_EQ(err, std::errc::illegal_byte_sequence);
}
TEST(http_parse, test_query) {
  HttpParser parser;
  const char* data = "GET /?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(view.query.size(), 2);
  ASSERT_EQ(view.query["a"], "1");
  ASSERT_EQ(view.query["b"], "2");
}
TEST(http_parse, test_query_empty) {
  HttpParser parser;
  const char* data = "GET /?a=1&b= HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.has_value());
  auto view = std::move(*res);
  ASSERT_EQ(view.query.size(), 2);
  ASSERT_EQ(view.query["a"], "1");
  ASSERT_EQ(view.query["b"], "");
}

int main() {
  xsl::no_log();
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
