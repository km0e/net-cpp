/**
 * @file http_server.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief A simple HTTP server that serves static files
 * @version 0.2
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/wheel.h"

#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/logctl.h>

std::string ip = "0.0.0.0";
std::string port = "8080";
// std::string doc_root = ".";
std::string doc_root = "../../../build/html";

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
Task<void> run(std::shared_ptr<xsl::Poller> poller, std::string_view ip, std::string_view port) {
  auto util = HttpUtil(make_socket_io_utils<Tcp<Ip<4>>>());
  auto service = util.make_service2();
  service.add_static("/", {doc_root, {}});
  auto creator = util.make_creator(poller, ip, port);
  Defer defer([&]() {
    log_info("Server stopped");
    poller->shutdown();
  });
  if (creator) {
    co_await creator->serve_connection(std::move(service).build());
  } else {
    log_error("Failed to create server: {}", creator.error().message());
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
  log_info("start http server at {}:{}", ip, port);

  auto poller = std::make_shared<xsl::Poller>();
  auto executor = std::make_shared<coro::NewThreadExecutor>();
  run(poller, ip, port).detach(std::move(executor));
  poller->run();
  return 0;
}
