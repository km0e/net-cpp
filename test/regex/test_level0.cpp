#include "xsl/regex.h"

#include <gtest/gtest.h>

using namespace xsl::regex;

TEST(regex, authority) {
  std::string_view auth = "user@127.0.0.1:8080";
  std::cmatch m;
  ASSERT_TRUE(std::regex_match(auth.begin(), auth.end(), authority_re));
  ASSERT_TRUE(std::regex_search(auth.begin(), auth.end(), m, authority_re));
  ASSERT_EQ(m[1].str(), "user");
  ASSERT_EQ(m[2].str(), "127.0.0.1");
  ASSERT_EQ(m[3].str(), "8080");
  std::string_view auth2 = "www.abc.com:8080";
  ASSERT_TRUE(std::regex_match(auth2.begin(), auth2.end(), authority_re));
  ASSERT_TRUE(std::regex_search(auth2.begin(), auth2.end(), m, authority_re));
  ASSERT_EQ(m[1].str(), "");
  ASSERT_EQ(m[2].str(), "www.abc.com");
  ASSERT_EQ(m[3].str(), "8080");
  std::string_view auth3 = "www.abc.com";
  ASSERT_TRUE(std::regex_match(auth3.begin(), auth3.end(), authority_re));
  ASSERT_TRUE(std::regex_search(auth3.begin(), auth3.end(), m, authority_re));
  ASSERT_EQ(m[1].str(), "");
  ASSERT_EQ(m[2].str(), "www.abc.com");
  ASSERT_EQ(m[3].str(), "");
}
TEST(regex, absoulte_uri) {
  std::string_view uri = "http://www.ics.uci.edu/pub/ietf/uri/?a=1";
  std::cmatch m;
  ASSERT_TRUE(std::regex_match(uri.begin(), uri.end(), absolute_uri_re));
  ASSERT_TRUE(std::regex_search(uri.begin(), uri.end(), m, absolute_uri_re));
  ASSERT_EQ(m[1].str(), "http");
  ASSERT_EQ(m[2].str(), "www.ics.uci.edu");
  ASSERT_EQ(m[3].str(), "/pub/ietf/uri/");
  ASSERT_EQ(m[4].str(), "a=1");
  std::string_view uri2 = "http://www.ics.uci.edu/pub/ietf/uri/";
  ASSERT_TRUE(std::regex_match(uri2.begin(), uri2.end(), absolute_uri_re));
  ASSERT_TRUE(std::regex_search(uri2.begin(), uri2.end(), m, absolute_uri_re));
  ASSERT_EQ(m[1].str(), "http");
  ASSERT_EQ(m[2].str(), "www.ics.uci.edu");
  ASSERT_EQ(m[3].str(), "/pub/ietf/uri/");
  ASSERT_EQ(m[4].str(), "");
  std::string_view uri3 = "http://www.ics.uci.edu/pub/ietf/uri";
  ASSERT_TRUE(std::regex_match(uri3.begin(), uri3.end(), absolute_uri_re));
  ASSERT_TRUE(std::regex_search(uri3.begin(), uri3.end(), m, absolute_uri_re));
  ASSERT_EQ(m[1].str(), "http");
  ASSERT_EQ(m[2].str(), "www.ics.uci.edu");
  ASSERT_EQ(m[3].str(), "/pub/ietf/uri");
  ASSERT_EQ(m[4].str(), "");
}

int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
};
