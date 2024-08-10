#include "xsl/logctl.h"
#include "xsl/net.h"
#include "xsl/net/http/router.h"

#include <gtest/gtest.h>

#include <memory>

using namespace std;
using namespace xsl;
TEST(http_router, add_route) {
  using namespace xsl::http;
  auto router = make_unique<Router>();
  router->add_route(Method::GET, "/hello", 11);
  router->add_route(Method::POST, "/hello", 12);
  router->add_route(Method::POST, "/hello/world", 13);
  router->add_route(Method::POST, "/", 14);
}

TEST(http_router, route) {
  using namespace xsl::http;
  auto router = make_unique<Router>();
  router->add_route(Method::GET, "/hello", 11);
  router->add_route(Method::GET, "/world/", 12);
  router->add_route(Method::GET, "/world/name", 13);

  auto ctx = RouteContext{Method::GET, "/hello"};
  auto res5 = router->route(ctx);
  ASSERT_TRUE(res5.has_value());
  ASSERT_EQ(**res5, 11);

  ctx = RouteContext{Method::GET, "/world/abc"};
  auto res7 = router->route(ctx);
  ASSERT_TRUE(res7.has_value());
  ASSERT_EQ(**res7, 12);

  ctx = RouteContext{Method::GET, "/world/name"};
  auto res9 = router->route(ctx);
  ASSERT_TRUE(res9.has_value());
  ASSERT_EQ(**res9, 13);
}

TEST(http_router, route_fallback) {
  using namespace xsl::http;
  auto router = make_unique<Router>();
  router->add_route(Method::GET, "/hello", 11);
  router->add_route(Method::GET, "/world/", 12);
  router->add_route(Method::GET, "/world/name", 13);
  router->add_fallback(Method::GET, "/world/", 14);

  auto ctx = RouteContext{Method::GET, "/world/abc"};
  auto res7 = router->route(ctx);
  ASSERT_TRUE(res7.has_value());
  EXPECT_EQ(**res7, 12);

  ctx = RouteContext{Method::GET, "/world/name"};
  auto res9 = router->route(ctx);
  ASSERT_TRUE(res9.has_value());
  EXPECT_EQ(**res9, 13);

  ctx = RouteContext{Method::GET, "/world/"};
  auto res11 = router->route(ctx);
  ASSERT_TRUE(res11.has_value());
  EXPECT_EQ(**res11, 14);
}

int main() {
  xsl::no_log();
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
