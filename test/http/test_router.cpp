#include "xsl/net/http.h"
#include "xsl/net/http/router.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
using namespace std;
TEST(http_router, add_route) {
  using namespace xsl::net;
  auto router = make_unique<http::HttpRouter>();
  auto res = router->add_route(HttpMethod::GET, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{200, "OK", http::HttpVersion::HTTP_1_1}, "hello")};
  });
  ASSERT_TRUE(res.is_ok());
  auto res2 = router->add_route(HttpMethod::GET, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{200, "OK", http::HttpVersion::HTTP_1_1}, "hello")};
  });
  ASSERT_TRUE(res2.is_err() && res2.unwrap_err().kind == http::AddRouteErrorKind::Conflict);
  auto res3 = router->add_route(HttpMethod::GET, "hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{200, "OK", http::HttpVersion::HTTP_1_1}, "hello")};
  });
  ASSERT_TRUE(res3.is_err() && res3.unwrap_err().kind == http::AddRouteErrorKind::InvalidPath);
  auto res4 = router->add_route(HttpMethod::POST, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{200, "OK", http::HttpVersion::HTTP_1_1}, "hello")};
  });
  ASSERT_TRUE(res4.is_ok());
}

int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
