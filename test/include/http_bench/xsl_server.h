/**
 * @file xsl_server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Minimal HTTP/1.1 hello server built on xsl::asio::HttpServer, used to
 *        compare with a standalone asio server (see test/benches/http and
 *        test/integration/http_compare)
 * @version 0.1.0
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef HTTP_BENCH_XSL_SERVER
#  define HTTP_BENCH_XSL_SERVER
#  include <xsl/asio.h>
#  include <xsl/coro.h>
#  include <xsl/log.h>

#  include <http_bench/common.h>

#  include <cstdint>
#  include <string>
#  include <string_view>

namespace http_bench {

/**
 * @brief GET /hello handler, 200 with a fixed plain text body
 *
 * @note the route table only matches GET /hello; other paths/methods fall
 *       back to the library default 404 (no body, connection kept alive)
 */
inline xsl::asio::HandleResult hello_handler(auto& ctx) {
  ctx.easy_resp(xsl::http::Status::OK, std::string(HELLO_BODY));
  ctx._response->set_header("Content-Type", "text/plain");
  co_return std::nullopt;
}

/**
 * @brief run the hello server on the current IOContext, never returns unless
 *        an error occurs
 *
 * @param ip  listen address
 * @param port listen port
 * @return xsl::Task<void>
 * @note the caller must create the coroutine context (asio_ctx) and run the
 *       reserved IOContext, e.g.
 *       @code
 *         MUST(asio_ctx(NewThreadExecutor{}), ctx);
 *         http_bench::run_xsl_hello_server(ip, port).detach(ctx);
 *         static_cast<xsl::sys::IOContext*>(ctx->get_reserved())->run();
 *       @endcode
 */
inline xsl::Task<void> run_xsl_hello_server(std::string_view ip, std::uint16_t port) {
  auto util = xsl::asio::HttpUtil(
      xsl::asio::AsyncSocketCreatorCompose<xsl::TcpIpv4>());
  auto service = util.make_service2();
  service.add_route(xsl::asio::Method::GET, HELLO_PATH,
                    [](auto& ctx) -> xsl::asio::HandleResult { return hello_handler(ctx); });
  auto& io = co_await xsl::asio::CurrentIOContext;
  auto creator = util.cl(io, ip, port);
  xsl::Defer defer([&]() {
    log_info("xsl hello server stopped");
    io.shutdown();
  });
  if (!creator) {
    log_error("Failed to create xsl server: {}", std::make_error_code(creator.error()).message());
    co_return;
  }
  log_info("xsl hello server listening on {}:{}", ip, port);
  co_await creator->serve_connection(std::move(service).build());
}

}  // namespace http_bench
#endif
