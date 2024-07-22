#include "xsl/net/http.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
using namespace std;
static string tmp_dir = "";
static void init() {
  error_code ec;
  tmp_dir = filesystem::temp_directory_path(ec).string();
  if (ec) {
    throw runtime_error("failed to get temp directory");
  }
  tmp_dir += "/xsl_http_test";
  filesystem::create_directory(tmp_dir, ec);
  if (ec) {
    throw runtime_error("failed to create temp directory");
  }
}
static void cleanup() {
  error_code ec;
  filesystem::remove_all(tmp_dir, ec);
  if (ec) {
    throw runtime_error("failed to remove temp directory");
  }
}
TEST(http_component_static, file_route_handler) {
  using namespace xsl::net;
  string file_path = tmp_dir + "/file_route_handler_test.txt";
  ofstream file(file_path);
  file << "hello world";
  file.close();
  auto res = create_static_handler(std::move(file_path));
  ASSERT_TRUE(res.has_value());
  auto handler = res.value();
  http::RouteContext ctx{Request{"", HttpRequestView{}}};
  ctx.current_path = "file_route_handler_test.txt";
  auto result = handler(ctx);

}

int main() {
  init();
  ::testing::InitGoogleTest();
  int ret = RUN_ALL_TESTS();
  cleanup();
  return ret;
}
