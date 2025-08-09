/**
 * @file server.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS Server
 * @version 0.2.0
 * @date 2025-07-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <CLI/CLI.hpp>
#include <listener/normal.h>
#include <resolver/normal.h>
#include <toml++/toml.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/feature.h>
#include <xsl/log.h>

#include <forward_list>
#include <string>

using namespace xsl::coro;
using namespace xsl::asio;
using namespace xsl;
using namespace xsl::dns;

struct RawConfig {
  std::vector<std::string> resolvers = {};         // List of resolvers
  std::vector<std::string> listeners = {};         // List of listeners
  std::optional<std::string> cache_url = {};       // Cache URL
  std::optional<std::size_t> cache_mem_size = {};  // Cache size
};

struct Config {
  std::vector<std::pair<std::string, uint16_t>> resolvers;  // List of resolvers
  std::vector<std::pair<std::string, uint16_t>> listeners;  // List of listeners
  std::string cache_url;                                    // Cache URL
  std::size_t cache_mem_size;                               // Cache size
};

Expected<void> load_config(std::string_view path, RawConfig &config) {
  toml::table tbl;
  try {
    tbl = toml::parse_file(path);
  } catch (const toml::parse_error &err) {
    ENSURE(false, errc::illegal_byte_sequence,
           std::format("Failed to parse TOML file {}: {}", path, err.what()));
  }
  TRY(ups, tbl["upstream"].as_array(), errc::invalid_argument, "No upstreams found");
  for (const auto &up : *ups) {
    TRY(up_tbl, up.as_table(), errc::invalid_argument, "Invalid upstream entry");
    TRY(uri, up_tbl->get_as<std::string>("uri"), errc::invalid_argument,
        "No host found in upstream entry");
    config.resolvers.emplace_back(*uri);
  }
  auto lns = tbl["listener"].as_array();
  ENSURE(lns, errc::invalid_argument, "No listeners found");
  for (const auto &ln : *lns) {
    TRY(ln_tbl, ln.as_table(), errc::invalid_argument, "Invalid listener entry");
    TRY(address, ln_tbl->get_as<std::string>("address"), errc::invalid_argument,
        "No host found in listener entry");
    config.listeners.emplace_back(*address);
  }
  if (auto cache = tbl["cache"].as_table();
      cache && cache->get_as<bool>("enabled")->value_or(true)) {
    config.cache_url = cache->get("uri")->value<std::string>();
    config.cache_mem_size = cache->get("mem_size")->value<std::size_t>();  // Default 10MB
  }
  return {};
}

class Server {
public:
  static Expected<Server> create(Context &ctx, RawConfig &config) {
    Server server;
    for (const auto &uri : config.resolvers) {
      if (auto udp_us = http::AuthorityForm::match(uri); udp_us) {
        std::string host{udp_us->host};
        TRV(resolver, UdpResolver::create(
                          ctx, host, udp_us->port.empty() ? 53 : std::atoi(udp_us->port.data())));
        server.add_resolver("normal", std::move(resolver));
      } else {
        log_warning("Not implemented resolver type: {}", uri);
      }
    }
    ENSURE(!config.resolvers.empty(), errc::invalid_argument, "No valid upstreams found");
    for (const auto &ln : config.listeners) {
      if (auto addr = http::AuthorityForm::match(ln); addr) {
        std::string host{addr->host};
        TRV(listener,
            UdpListener::create(
                ctx, host,
                addr->port.empty()
                    ? 53
                    : std::atoi(addr->port.data())));  // Create a resolver for each upstream
        server.add_listener(std::make_unique<UdpListener>(std::move(listener)));
      } else {
        log_warning("Not implemented listener type: {}", ln);
      }
    }
    ENSURE(!config.listeners.empty(), errc::invalid_argument, "No valid listeners found");
    TRV(cache, SqliteCache::create(config.cache_url.value_or(":memory:").c_str()));
    server.set_cache(std::move(cache));
    return server;
  }
  Server() {}
  template <class CacheType>
  void set_cache(std::unique_ptr<CacheType> &&cache) {
    core.set_cache(std::move(cache));
  }
  template <std::derived_from<Resolver> ResolverType>
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
  std::string path = "", upstream = "223.5.5.5", listen = "127.0.0.1:8080";
  CLI::App app{"DNS Lookup"};
  app.add_option("-c, --config", path, "Configuration file path")->check(CLI::ExistingFile);
  app.add_option("-u, --upstream", upstream, "Upstream DNS server address");
  app.add_option("-l, --listen", listen, "Listening address and port for the DNS server");
  CLI11_PARSE(app, argc, argv);

  RawConfig raw_config;
  if (!path.empty()) {
    MUST(load_config(path, raw_config));
  }
  raw_config.resolvers.emplace_back(upstream);
  raw_config.listeners.emplace_back(listen);

  auto ctx = std::make_shared<Context>();
  auto executor = std::make_shared<NoopExecutor>();
  MUST(Server::create(*ctx, raw_config), server);
  server.run().detach(std::move(executor));
  ctx->run();
  return 0;
}
