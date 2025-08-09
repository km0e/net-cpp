/**
 * @file dns.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS core functionality
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_APP_DNS_CORE_H
#  define XSL_APP_DNS_CORE_H

#  include <cache/sqlite.h>
#  include <resolver/def.h>
#  include <xsl/coro.h>
#  include <xsl/def.h>
#  include <xsl/log.h>
#  include <xsl/net.h>

#  include <string_view>
#  include <unordered_map>
using namespace xsl::dns;
using namespace xsl::coro;

class DnsCore {
public:
  template <std::derived_from<DnsCache> CacheType>
  void set_cache(std::unique_ptr<CacheType> &&cache) {
    this->cache = std::move(cache);
  }
  template <std::derived_from<Resolver> ResolverType>
  void add_resolver(std::string_view name, std::unique_ptr<ResolverType> &&resolver) {
    resolvers.emplace(name, std::move(resolver));
  }
  Task<Expected<std::pair<std::vector<dns::RRView>, DnsBuf>>> get(std::string_view name,
                                                                  Question &q) {
    auto res = cache->get(name, q.type, q.class_);  // check cache first
    if (!res) {
      log_error("Cache error: {}", res.error().message());
    } else if (res->is_valid()) {
      log_info("Cache hit for domain: {}, type: {}, class: {}", name, q.type, q.class_);
      co_return std::make_pair(std::vector{RRView(res->data())},
                               DnsBuf(std::move(*res).into_underlying(), res->size()));
    }
    log_info("Cache miss for domain: {}, type: {}, class: {}", name, q.type, q.class_);
    // resolve the domain name
    std::string error_msg;
    for (const auto &[dn, resolver] : resolvers) {
      auto res = co_await resolver->query(name, q);
      if (!res) {
        auto msg = std::format("Failed to resolve domain: {}, type: {}, class: {}, error: {}", dn,
                               q.type, q.class_, std::make_error_condition(res.error()).message());
        log_error("{}", msg);
        error_msg += msg + "\n";
        continue;
      }
      auto [rrs, buf] = std::move(*res);
      for (const auto &rr : rrs) {
        cache->put(dn, rr);  // cache the result
      }
      log_debug("Resolved domain: {}, type: {}, class: {}, count: {}", dn, q.type, q.class_,
                rrs.size());
      if (rrs.empty()) {
        log_debug("No records found for domain: {}, type: {}, class: {}", dn, q.type, q.class_);
        continue;
      }
      co_return std::make_pair(std::move(rrs), std::move(buf));
    }
    co_return std::unexpected(Error(errc::result_out_of_range, std::move(error_msg)));
  }
  std::unique_ptr<DnsCache> cache;
  std::unordered_map<std::string, std::unique_ptr<Resolver>> resolvers;
};

#endif
