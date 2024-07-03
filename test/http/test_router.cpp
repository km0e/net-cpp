#include "xsl/logctl.h"
#include "xsl/net/http.h"
#include "xsl/net/http/msg.h"
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
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res.is_ok());
  auto res2 = router->add_route(HttpMethod::GET, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res2.is_err() && res2.unwrap_err().kind == http::AddRouteErrorKind::Conflict);
  auto res3 = router->add_route(HttpMethod::GET, "hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res3.is_err() && res3.unwrap_err().kind == http::AddRouteErrorKind::InvalidPath);
  auto res4 = router->add_route(HttpMethod::POST, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res4.is_ok());
  auto res5 = router->add_route(HttpMethod::POST, "/hello/world", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res5.is_ok());
  auto res6 = router->add_route(HttpMethod::POST, "/", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res6.is_ok());
  auto res7 = router->add_route(HttpMethod::POST, "", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res7.is_err() && res7.unwrap_err().kind == http::AddRouteErrorKind::InvalidPath);
}

TEST(http_router, route) {
  using namespace xsl::net;
  auto router = make_unique<http::HttpRouter>();
  auto res1 = router->add_route(HttpMethod::GET, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res1.is_ok());
  auto res2 = router->add_route(HttpMethod::GET, "/world/", [](http::RouteContext& ctx) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, string{ctx.current_path})};
  });
  ASSERT_TRUE(res2.is_ok());
  auto res3 = router->add_route(HttpMethod::GET, "/world/name", [](http::RouteContext& ctx) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, string{ctx.current_path})};
  });
  ASSERT_TRUE(res3.is_ok());
  HttpParser parser;
  string_view data = "GET /hello HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto res4 = parser.parse(data);
  ASSERT_TRUE(res4.is_ok());
  auto view = res4.unwrap();
  auto req = http::Request{string{}, view};
  auto ctx = http::RouteContext{std::move(req)};
  auto res5 = router->route(ctx);
  ASSERT_TRUE(res5.is_ok());
  auto tasks = res5.unwrap();
  auto response = dynamic_cast<http::HttpResponse<string>*>(&*tasks);
  ASSERT_EQ(response->body, "hello");

  data = "GET /world/abc HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto res6 = parser.parse(data);
  ASSERT_TRUE(res6.is_ok());
  view = res6.unwrap();
  req = http::Request{string{}, view};
  ctx = http::RouteContext{std::move(req)};
  auto res7 = router->route(ctx);
  ASSERT_TRUE(res7.is_ok());
  tasks = res7.unwrap();
  response = dynamic_cast<http::HttpResponse<string>*>(&*tasks);
  ASSERT_EQ(response->body, "/abc");

  data = "GET /world/name HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto res8 = parser.parse(data);
  ASSERT_TRUE(res8.is_ok());
  view = res8.unwrap();
  req = http::Request{string{}, view};
  ctx = http::RouteContext{std::move(req)};
  auto res9 = router->route(ctx);
  ASSERT_TRUE(res9.is_ok());
  tasks = res9.unwrap();
  response = dynamic_cast<http::HttpResponse<string>*>(&*tasks);
  ASSERT_EQ(response->body, "");
}

int main() {
  xsl::no_log();
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
