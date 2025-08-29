/**
 * @file router.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <gtest/gtest.h>
#include <xsl/log.h>
#include <xsl/net.h>
#include <xsl/net/http/router.h>

using namespace xsl;

TEST(route, exact) {
  using namespace xsl::http;
  Router<int> r;
  auto h1 = &r.add_exact("/hello");
  r.add_exact("/world/");
  auto h3 = &r.add_exact("/world/name");

  auto ctx = RouteContext{Method::GET, "/hello"};
  auto res5 = r.route(ctx);
  ASSERT_TRUE(res5 == h1);

  ctx = RouteContext{Method::GET, "/world/abc"};
  auto res7 = r.route(ctx);
  ASSERT_TRUE(res7 == nullptr);

  ctx = RouteContext{Method::GET, "/world/name"};
  auto res9 = r.route(ctx);
  ASSERT_TRUE(res9 == h3);
}

TEST(route, prefix) {
  using namespace xsl::http;
  Router<int> r;
  auto h1 = &r.add_exact("/hello/world");
  auto h2 = &r.add_prefix("/hello/");
  *h2 = 1;
  auto h3 = &r.add_exact("/hello/name");

  auto ctx = RouteContext{Method::GET, "/hello/world"};
  auto res5 = r.route(ctx);
  ASSERT_TRUE(res5 == h1);

  ctx = RouteContext{Method::GET, "/hello/abc"};
  auto res7 = r.route(ctx);
  ASSERT_TRUE(res7 == h2);

  ctx = RouteContext{Method::GET, "/hello/name"};
  auto res9 = r.route(ctx);
  ASSERT_TRUE(res9 == h3);
}

int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
