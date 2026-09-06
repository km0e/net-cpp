/**
 * @file server_asio.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief standalone asio HTTP server used for the HTTP-vs-asio benchmark
 * @version 0.1.0
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <CLI/CLI.hpp>
#include <http_bench/asio_server.h>

int main(int argc, char* argv[]) {
  std::string ip = "127.0.0.1";
  int port = 8080;
  int threads = 1;
  CLI::App app{"asio hello http server (bench)"};
  app.add_option("-i,--ip", ip, "Listen address")->capture_default_str();
  app.add_option("-p,--port", port, "Listen port")->check(CLI::Range(1, 65535))->capture_default_str();
  app.add_option("-t,--threads", threads, "IO threads")->check(CLI::Range(1, 256))->capture_default_str();
  CLI11_PARSE(app, argc, argv);

  asio::io_context ioc;
  auto guard = asio::make_work_guard(ioc);
  http_bench::AsioHelloServer server(ioc, ip, static_cast<std::uint16_t>(port));
  asio::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto) { ioc.stop(); });

  std::vector<std::thread> workers;
  workers.reserve(threads - 1);
  for (int i = 1; i < threads; ++i) {
    workers.emplace_back([&ioc] { ioc.run(); });
  }
  ioc.run();
  for (auto& worker : workers) {
    worker.join();
  }
  return 0;
}
