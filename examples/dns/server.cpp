/**
 * @file dns_server.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS Server
 * @version 0.1
 * @date 2025-07-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <CLI/CLI.hpp>
#include <listener/normal.h>
#include <resolver/normal.h>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/feature.h>
#include <xsl/log.h>

#include <forward_list>
#include <string>
std::string ip = "8.8.8.8";

std::string port = "53";

using namespace xsl::coro;
using namespace xsl::asio;
using namespace xsl;
using namespace xsl::dns;

class Server {
public:
  Server() {}
  template <class CacheType>
  void set_cache(std::unique_ptr<CacheType> &&cache) {
    core.set_cache(std::move(cache));
  }
  template <class ResolverType>
  void add_resolver(std::string_view name, std::unique_ptr<ResolverType> &&resolver) {
    core.add_resolver(name, std::move(resolver));
  }

  template <class ListenerType>
  void add_listener(std::unique_ptr<ListenerType> &&listener) {
    listeners.emplace_front(std::move(listener));
  }

  Task<void> run() {
    for (auto &listener : listeners) {
      co_yield listener->run(core);
    }
  }

  DnsCore core = {};
  std::forward_list<std::unique_ptr<Listener>> listeners = {};
};

int main(int argc, char *argv[]) {
  CLI::App app{"DNS Lookup"};
  app.add_option("-i,--ip", ip, "IP address");
  app.add_option("-p,--port", port, "Port");
  CLI11_PARSE(app, argc, argv);

  auto ctx = std::make_shared<Context>();
  auto executor = std::make_shared<NewThreadExecutor>();
  Server server{};
  SqliteCache cache;
  // open in memory
  MUST(cache.open(":memory:"));
  server.set_cache(std::make_unique<SqliteCache>(std::move(cache)));
  auto raddr = "192.168.5.1";
  MUST(UdpResolver::create(*ctx, std::move(raddr)), resolver);
  server.add_resolver("normal", std::move(resolver));
  std::string laddr = "0.0.0.0:8081";
  MUST(UdpListener::create(*ctx, laddr.data()), listener);
  server.add_listener(std::make_unique<UdpListener>(std::move(listener)));
  server.run().detach(std::move(executor));
  ctx->run();
  return 0;
}
