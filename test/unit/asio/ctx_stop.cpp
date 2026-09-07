/**
 * @file ctx_stop.cpp
 * @brief asio_ctx binds the IOContext's stop source: poller shutdown cancels
 *        the whole coroutine cancellation domain (Phase 4)
 */
#include <gtest/gtest.h>
#include <xsl/asio.h>

#include <stop_token>

using namespace xsl;

TEST(AsioCtxStop, ShutdownRequestsStop) {
  auto res = asio::asio_ctx(coro::NewThreadExecutor{});
  ASSERT_TRUE(res);
  auto& ctx = *res;
  auto* ioc = static_cast<sys::IOContext*>(ctx->get_reserved());
  ASSERT_NE(ioc, nullptr);
  ASSERT_FALSE(ctx->stop_requested());

  ioc->shutdown();

  EXPECT_TRUE(ctx->stop_requested()) << "poller shutdown must cancel the bound coroutine domain";
}

TEST(AsioCtxStop, CancelContextsWithoutTeardown) {
  auto res = asio::asio_ctx(coro::NewThreadExecutor{});
  ASSERT_TRUE(res);
  auto& ctx = *res;
  auto* ioc = static_cast<sys::IOContext*>(ctx->get_reserved());
  ASSERT_FALSE(ioc->stopped());

  ioc->cancel_contexts();

  EXPECT_TRUE(ctx->stop_requested());
  EXPECT_FALSE(ioc->stopped()) << "cancel_contexts must not tear down the poller";
  ioc->shutdown();  // clean up the epoll fd
}
