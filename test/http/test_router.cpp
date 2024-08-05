#include "xsl/logctl.h"
#include "xsl/net.h"
#include "xsl/net/http/msg.h"
#include "xsl/net/http/router.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace std;
using namespace xsl;
TEST(http_router, add_route) {
  using namespace xsl::http;
  auto router = make_unique<HttpRouter>();
  router->add_route(HttpMethod::GET, "/hello", [](RouteContext&) -> RouteHandleResult {
    co_return HttpResponse{{HttpVersion::HTTP_1_1, HttpStatus::OK}, "hello"};
  });
  router->add_route(HttpMethod::POST, "/hello", [](RouteContext&) -> RouteHandleResult {
    co_return HttpResponse{{HttpVersion::HTTP_1_1, HttpStatus::OK}, "hello"};
  });
  router->add_route(HttpMethod::POST, "/hello/world", [](RouteContext&) -> RouteHandleResult {
    co_return HttpResponse{{HttpVersion::HTTP_1_1, HttpStatus::OK}, "hello"};
  });
  router->add_route(HttpMethod::POST, "/", [](RouteContext&) -> RouteHandleResult {
    co_return HttpResponse{{HttpVersion::HTTP_1_1, HttpStatus::OK}, "hello"};
  });
}

TEST(http_router, route) {
  using namespace xsl::http;
  auto router = make_unique<HttpRouter>();
  router->add_route(HttpMethod::GET, "/hello", [](RouteContext&) -> RouteHandleResult {
    co_return HttpResponse{{HttpVersion::HTTP_1_1, HttpStatus::OK}, "hello"};
  });
  router->add_route(HttpMethod::GET, "/world/", [](RouteContext& ctx) -> RouteHandleResult {
    co_return HttpResponse{{HttpVersion::HTTP_1_1, HttpStatus::OK}, string{ctx.current_path}};
  });
  router->add_route(HttpMethod::GET, "/world/name", [](RouteContext& ctx) -> RouteHandleResult {
    co_return HttpResponse{{HttpVersion::HTTP_1_1, HttpStatus::OK}, string{ctx.current_path}};
  });
  HttpParseUnit parser;
  string_view data = "GET /hello HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz4, res4] = parser.parse(data);
  ASSERT_TRUE(res4.has_value());
  auto view = std::move(res4.value());
  auto req = Request{{}, std::move(view), {}};
  auto ctx = RouteContext{std::move(req)};
  auto res5 = router->route(ctx);
  ASSERT_TRUE(res5.has_value());
  auto tasks1 = (**res5)(ctx).block();
  // ASSERT_EQ(tasks1->_body, "hello");

  data = "GET /world/abc HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz6, res6] = parser.parse(data);
  ASSERT_TRUE(res6.has_value());
  req = Request{{}, std::move(*res6), {}};
  ctx = RouteContext{std::move(req)};
  auto res7 = router->route(ctx);
  ASSERT_TRUE(res7.has_value());
  auto tasks2 = (**res7)(ctx).block();
  // ASSERT_EQ(tasks2->_body, "/abc");

  data = "GET /world/name HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
  auto [sz8, res8] = parser.parse(data);
  ASSERT_TRUE(res8.has_value());
  req = Request{{}, std::move(*res8), {}};
  ctx = RouteContext{std::move(req)};
  auto res9 = router->route(ctx);
  ASSERT_TRUE(res9.has_value());
  auto tasks3 = (**res9)(ctx).block();
  // ASSERT_EQ(tasks3->_body, "");
}

int main() {
  xsl::no_log();
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
