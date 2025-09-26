/**
 * @file tcp_echo.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief A simple echo server
 * @version 0.3.0
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/io.h>
#include <xsl/log.h>

std::string ip = "127.0.0.1";
std::string port = "8080";

using namespace xsl;
using namespace xsl::asio;

Task<void> talk(std::string_view ip, std::string_view port) {
  auto util = AsyncSocketCreatorCompose<Tcp, Ip>();
  auto& ctx = co_await CurrentIOContext;
  auto creator = *util.cl(ctx, ip, port);
  while (true) {
    auto task = co_await creator->accept_async(ctx).and_then(
        [&](auto&& skt) { return splice_bidirectional(skt, skt, ctx); });
    if (!task) {
      log_warning("splice error: {}", std::make_error_code(task.error()).message());
      break;
    }
    co_yield std::move(*task);
  }
  ctx.shutdown();
  co_return;
}

int main(int argc, char* argv[]) {
  CLI::App app{"Echo server"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);
  log_info("Starting echo server at {}:{}", ip, port);

  MUST(asio_ctx(NewThreadExecutor{}), ctx);
  talk(ip, port).detach(ctx);
  static_cast<sys::IOContext*>(ctx->get_reserved())->run();
  return 0;
}
