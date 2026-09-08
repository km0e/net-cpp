/**
 * @file server_xsl.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief xsl::asio HTTP server used for the HTTP-vs-asio benchmark
 * @version 0.1.0
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <CLI/CLI.hpp>
#include <http_bench/xsl_server.h>

int main(int argc, char* argv[]) {
  std::string ip = "127.0.0.1";
  std::string port = "8080";
  int pollers = 0;
  CLI::App app{"xsl hello http server (bench)"};
  app.add_option("-i,--ip", ip, "Listen address")->capture_default_str();
  app.add_option("-p,--port", port, "Listen port")->check(CLI::Range(1, 65535))->capture_default_str();
  app.add_option("-n,--pollers", pollers, "Poller count (0 = thread-pool executor, N = N inline pollers)")
      ->capture_default_str();
  CLI11_PARSE(app, argc, argv);
  if (pollers > 0) {
    // multi-poller model: one listen socket + IOContext + NoopExecutor ctx
    // per poller, coroutines resume inline on their poller thread
    xsl::asio::PollerGroup group(static_cast<unsigned>(pollers));
    MUST(group.start([&](xsl::CoroContext&, xsl::sys::IOContext&) -> xsl::Task<void> {
      return http_bench::run_xsl_hello_server(ip, static_cast<std::uint16_t>(std::atoi(port.c_str())));
    }));
    log_info("xsl hello server: {} pollers on port {}", group.size(), port);
    group.join();
    return 0;
  }
  MUST(xsl::asio::asio_ctx(xsl::coro::ThreadPoolExecutor{4}), ctx);
  http_bench::run_xsl_hello_server(ip, static_cast<std::uint16_t>(std::atoi(port.c_str())))
      .detach(*ctx);
  static_cast<xsl::sys::IOContext*>(ctx->get_reserved())->run();
  return 0;
}
