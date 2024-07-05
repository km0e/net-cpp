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
  ASSERT_TRUE(res.has_value());
  auto res2 = router->add_route(HttpMethod::GET, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE((!res2.has_value()) && res2.error().kind == http::AddRouteErrorKind::Conflict);
  auto res3 = router->add_route(HttpMethod::GET, "hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE((!res3.has_value()) && res3.error().kind == http::AddRouteErrorKind::InvalidPath);
  auto res4 = router->add_route(HttpMethod::POST, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res4.has_value());
  auto res5 = router->add_route(HttpMethod::POST, "/hello/world", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res5.has_value());
  auto res6 = router->add_route(HttpMethod::POST, "/", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res6.has_value());
  auto res7 = router->add_route(HttpMethod::POST, "", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE((!res7.has_value()) && res7.error().kind == http::AddRouteErrorKind::InvalidPath);
}

TEST(http_router, route) {
  using namespace xsl::net;
  auto router = make_unique<http::HttpRouter>();
  auto res1 = router->add_route(HttpMethod::GET, "/hello", [](http::RouteContext&) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, "hello")};
  });
  ASSERT_TRUE(res1.has_value());
  auto res2 = router->add_route(HttpMethod::GET, "/world/", [](http::RouteContext& ctx) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, string{ctx.current_path})};
  });
  ASSERT_TRUE(res2.has_value());
  auto res3 = router->add_route(HttpMethod::GET, "/world/name", [](http::RouteContext& ctx) {
    return http::RouteHandleResult{make_unique<http::HttpResponse<string>>(
        http::ResponsePart{http::HttpVersion::HTTP_1_1, 200, "OK"}, string{ctx.current_path})};
  });
  ASSERT_TRUE(res3.has_value());
  HttpParser parser;
  string_view data = "GET /hello HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto res4 = parser.parse(data);
  ASSERT_TRUE(res4.has_value());
  auto view = std::move(res4.value());
  auto req = http::Request{string{}, view};
  auto ctx = http::RouteContext{std::move(req)};
  auto res5 = router->route(ctx);
  ASSERT_TRUE(res5.has_value());
  auto tasks = std::move(res5.value());
  auto response = dynamic_cast<http::HttpResponse<string>*>(&*tasks);
  ASSERT_EQ(response->body, "hello");

  data = "GET /world/abc HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto res6 = parser.parse(data);
  ASSERT_TRUE(res6.has_value());
  view = res6.value();
  req = http::Request{string{}, view};
  ctx = http::RouteContext{std::move(req)};
  auto res7 = router->route(ctx);
  ASSERT_TRUE(res7.has_value());
  tasks = std::move(res7.value());
  response = dynamic_cast<http::HttpResponse<string>*>(&*tasks);
  ASSERT_EQ(response->body, "/abc");

  data = "GET /world/name HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto res8 = parser.parse(data);
  ASSERT_TRUE(res8.has_value());
  view = res8.value();
  req = http::Request{string{}, view};
  ctx = http::RouteContext{std::move(req)};
  auto res9 = router->route(ctx);
  ASSERT_TRUE(res9.has_value());
  tasks = std::move(res9.value());
  response = dynamic_cast<http::HttpResponse<string>*>(&*tasks);
  ASSERT_EQ(response->body, "");
}

int main() {
  xsl::no_log();
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
