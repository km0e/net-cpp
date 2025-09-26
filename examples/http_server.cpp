/**
 * @file http_server.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief A simple HTTP server that serves static files
 * @version 0.2.0
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/log.h>

std::string ip = "0.0.0.0";
std::string port = "8080";
std::string doc_root = ".";

using namespace xsl::asio;
using namespace xsl;
/**
 * @brief run http server
 *
 * @param poller
 * @param ip
 * @param port
 * @return Task<void>
 * @note this example all use static call
 */
Task<void> run(std::string_view ip, std::string_view port) {
  auto util = HttpUtil(AsyncSocketCreatorCompose<TcpIpv4>());
  auto service = util.make_service2();
  service.add_static("/", {doc_root, {}});

  auto& io = co_await CurrentIOContext;
  auto creator = util.cl(io, ip.data(), port.data());
  Defer defer([&]() {
    log_info("Server stopped");
    io.shutdown();
  });
  if (creator) {
    co_await creator->serve_connection(std::move(service).build());
  } else {
    log_error("Failed to create server: {}", creator.error());
  }
  co_return;
}

int main(int argc, char* argv[]) {
  CLI::App app{"Http static server"};
  app.add_option("-i,--ip", ip, "IP address")->capture_default_str();
  app.add_option("-p,--port", port, "Port")->check(CLI::Range(1, 65535))->capture_default_str();
  app.add_option("-d,--doc-root", doc_root, "Document root directory")
      ->check(CLI::ExistingDirectory)
      ->capture_default_str();
  CLI11_PARSE(app, argc, argv);
  log_info("Start http server at {}:{}", ip, port);
  MUST(asio_ctx(NewThreadExecutor{}), ctx);
  run(ip, port).detach(ctx);
  static_cast<sys::IOContext*>(ctx->get_reserved())->run();
  return 0;
}
