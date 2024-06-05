#include "xsl/net/http.h"

#include <gtest/gtest.h>
using namespace xsl::net;
TEST(http_parse, complete) {
  HttpParser parser;
  const char* data = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.is_ok());
  auto view = res.unwrap();
  ASSERT_EQ(xsl::from_string<HttpMethod>(view.method), HttpMethod::GET);
  ASSERT_EQ(view.url, "/");
  ASSERT_EQ(xsl::from_string<HttpVersion>(view.version), HttpVersion::HTTP_1_1);
  ASSERT_EQ(view.headers.size(), 1);
  ASSERT_EQ(view.headers["Host"], "localhost:8080");
}
TEST(http_parse, partial) {
  HttpParser parser;
  const char* data = "GET / HTTP/1.1\r\nHost: localhost:8080";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.is_err());
  auto err = res.unwrap_err();
  ASSERT_EQ(err.kind, HttpParseErrorKind::Partial);
}
TEST(http_parse, invalid_format) {
  HttpParser parser;
  const char* data = "GET / HTTP/1.1\rHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.is_err());
  auto err = res.unwrap_err();
  ASSERT_EQ(err.kind, HttpParseErrorKind::InvalidFormat);
}
TEST(http_parse, test_version) {
  HttpParser parser;
  const char* data = "GET / HTT/1.0\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.is_err());
  auto err = res.unwrap_err();
  ASSERT_EQ(err.kind, HttpParseErrorKind::InvalidFormat);
}
TEST(http_parse, test_query) {
  HttpParser parser;
  const char* data = "GET /?a=1&b=2 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.is_ok());
  auto view = res.unwrap();
  ASSERT_EQ(view.query.size(), 2);
  ASSERT_EQ(view.query["a"], "1");
  ASSERT_EQ(view.query["b"], "2");
}
TEST(http_parse, test_query_empty) {
  HttpParser parser;
  const char* data = "GET /?a=1&b= HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  size_t len = strlen(data);
  auto res = parser.parse(data, len);
  ASSERT_TRUE(res.is_ok());
  auto view = res.unwrap();
  ASSERT_EQ(view.query.size(), 2);
  ASSERT_EQ(view.query["a"], "1");
  ASSERT_EQ(view.query["b"], "");
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
