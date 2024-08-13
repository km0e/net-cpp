#include "xsl/logctl.h"
#include "xsl/net/http/proto/accept.h"

#include <gtest/gtest.h>
using namespace xsl::_net::http::proto;

TEST(HttpProto, parse_accept) {
  std::string_view accept0 = "text/html, application/xhtml+xml, application/xml;q=0.9, */*;q=0.8";
  auto result1 = xsl::_net::http::proto::parse_accept(accept0);
  EXPECT_EQ(result1.size(), 4);
  EXPECT_EQ(result1[0].first.main_type(), "text");
  EXPECT_EQ(result1[0].first.sub_type(), "html");
  EXPECT_EQ(result1[0].second, std::string_view{});
  EXPECT_EQ(result1[1].first.main_type(), "application");
  EXPECT_EQ(result1[1].first.sub_type(), "xhtml+xml");
  EXPECT_EQ(result1[1].second, std::string_view{});
  EXPECT_EQ(result1[2].first.main_type(), "application");
  EXPECT_EQ(result1[2].first.sub_type(), "xml");
  EXPECT_EQ(result1[2].second, "0.9");
  EXPECT_EQ(result1[3].first.main_type(), "*");
  EXPECT_EQ(result1[3].first.sub_type(), "*");
  EXPECT_EQ(result1[3].second, "0.8");

  std::string_view accept1 = "audio/*; q=0.2, audio/basic";
  auto result2 = xsl::_net::http::proto::parse_accept(accept1);
  EXPECT_EQ(result2.size(), 2);
  EXPECT_EQ(result2[0].first.main_type(), "audio");
  EXPECT_EQ(result2[0].first.sub_type(), "*");
  EXPECT_EQ(result2[0].second, "0.2");
  EXPECT_EQ(result2[1].first.main_type(), "audio");
  EXPECT_EQ(result2[1].first.sub_type(), "basic");
  EXPECT_EQ(result2[1].second, std::string_view{});

  std::string_view accept2 = "text/plain; q=0.5, text/html, text/x-dvi; q=0.8, text/x-c";
  auto result3 = xsl::_net::http::proto::parse_accept(accept2);
  EXPECT_EQ(result3.size(), 4);
  EXPECT_EQ(result3[0].first.main_type(), "text");
  EXPECT_EQ(result3[0].first.sub_type(), "plain");
  EXPECT_EQ(result3[0].second, "0.5");
  EXPECT_EQ(result3[1].first.main_type(), "text");
  EXPECT_EQ(result3[1].first.sub_type(), "html");
  EXPECT_EQ(result3[1].second, std::string_view{});
  EXPECT_EQ(result3[2].first.main_type(), "text");
  EXPECT_EQ(result3[2].first.sub_type(), "x-dvi");
  EXPECT_EQ(result3[2].second, "0.8");
  EXPECT_EQ(result3[3].first.main_type(), "text");
  EXPECT_EQ(result3[3].first.sub_type(), "x-c");
  EXPECT_EQ(result3[3].second, std::string_view{});

  std::string_view accept3
      = "text/*;q=0.3, text/plain;q=0.7, text/plain;format=flowed,text/plain;format=fixed;q=0.4, "
        "*/*;q=0.5";
  auto result4 = xsl::_net::http::proto::parse_accept(accept3);
  EXPECT_EQ(result4.size(), 5);
  EXPECT_EQ(result4[0].first.main_type(), "text");
  EXPECT_EQ(result4[0].first.sub_type(), "*");
  EXPECT_EQ(result4[0].second, "0.3");
  EXPECT_EQ(result4[1].first.main_type(), "text");
  EXPECT_EQ(result4[1].first.sub_type(), "plain");
  EXPECT_EQ(result4[1].second, "0.7");
  EXPECT_EQ(result4[2].first.main_type(), "text");
  EXPECT_EQ(result4[2].first.sub_type(), "plain");
  EXPECT_EQ(result4[2].first.parameters.size(), 1);
  EXPECT_EQ(result4[2].first.parameters[0].name, "format");
  EXPECT_EQ(result4[2].first.parameters[0].value, "flowed");
  EXPECT_EQ(result4[2].second, std::string_view{});
  EXPECT_EQ(result4[3].first.main_type(), "text");
  EXPECT_EQ(result4[3].first.sub_type(), "plain");
  EXPECT_EQ(result4[3].first.parameters.size(), 1);
  EXPECT_EQ(result4[3].first.parameters[0].name, "format");
  EXPECT_EQ(result4[3].first.parameters[0].value, "fixed");
  EXPECT_EQ(result4[3].second, "0.4");
  EXPECT_EQ(result4[4].first.main_type(), "*");
  EXPECT_EQ(result4[4].first.sub_type(), "*");
  EXPECT_EQ(result4[4].second, "0.5");
}

TEST(HttpProto, parse_accept_encoding) {
  std::string_view accept_encoding0 = "compress, gzip";
  auto result1 = xsl::_net::http::proto::parse_accept_encoding(accept_encoding0);
  EXPECT_EQ(result1.size(), 2);
  EXPECT_EQ(result1[0].first, "compress");
  EXPECT_EQ(result1[0].second, std::string_view{});
  EXPECT_EQ(result1[1].first, "gzip");
  EXPECT_EQ(result1[1].second, std::string_view{});

  std::string_view accept_encoding1 = "";
  auto result2 = xsl::_net::http::proto::parse_accept_encoding(accept_encoding1);
  EXPECT_EQ(result2.size(), 0);

  std::string_view accept_encoding2 = "compress;q=0.5, gzip;q=1.0";
  auto result3 = xsl::_net::http::proto::parse_accept_encoding(accept_encoding2);
  EXPECT_EQ(result3.size(), 2);
  EXPECT_EQ(result3[0].first, "compress");
  EXPECT_EQ(result3[0].second, "0.5");
  EXPECT_EQ(result3[1].first, "gzip");
  EXPECT_EQ(result3[1].second, "1.0");

  std::string_view accept_encoding3 = "gzip;q=1.0, identity; q=0.5, *;q=0";
  auto result4 = xsl::_net::http::proto::parse_accept_encoding(accept_encoding3);
  EXPECT_EQ(result4.size(), 3);
  EXPECT_EQ(result4[0].first, "gzip");
  EXPECT_EQ(result4[0].second, "1.0");
  EXPECT_EQ(result4[1].first, "identity");
  EXPECT_EQ(result4[1].second, "0.5");
  EXPECT_EQ(result4[2].first, "*");
  EXPECT_EQ(result4[2].second, "0");
}

int main(int argc, char **argv) {
  xsl::no_log();
  // xsl::set_log_level(xsl::LogLevel::TRACE);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
