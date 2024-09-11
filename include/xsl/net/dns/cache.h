/**
 * @file cache.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS cache
 * @version 0.1
 * @date 2024-09-10
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_CACHE
#  define XSL_NET_DNS_CACHE
#  include "xsl/net/dns/def.h"
#  include "xsl/net/dns/proto/rr.h"
#  include "xsl/sync.h"
#  include "xsl/wheel.h"

#  include <forward_list>
#  include <memory>
XSL_NET_DNS_NB
class MemoryCache {
public:
  MemoryCache() : cache{} {}
  ~MemoryCache() = default;
  /**
   * @brief insert the resource record
   *
   * @param dn domain name
   * @param rr resource record
   * @return const std::forward_list<RR>*
   */
  const std::forward_list<RR> *insert(const std::string_view &dn,
                                      std::convertible_to<RR> auto &&...rr) {
    auto rrs = std::make_unique<std::forward_list<RR>>();
    auto inserter = std::front_inserter(*rrs);
    ((*(inserter++) = std::forward<decltype(rr)>(rr)), ...);
    auto ptr = rrs.get();
    auto lock = this->cache.lock();
    lock->insert_or_assign(std::string(dn), std::move(rrs));
    return ptr;
  }
  /**
   * @brief insert the resource record
   *
   * @param dn domain name
   * @param rrs resource records
   * @return const std::forward_list<RR>*
   */
  const std::forward_list<RR> *insert(const std::string_view &dn, std::forward_list<RR> &&rrs) {
    auto rrs_ptr = std::make_unique<std::forward_list<RR>>(std::move(rrs));
    auto ptr = rrs_ptr.get();
    this->cache.lock()->insert_or_assign(std::string(dn), std::move(rrs_ptr));
    return ptr;
  }
  /// @brief get the resource record
  const std::forward_list<RR> *get(const std::string_view &dn) {
    auto lock = this->cache.lock_shared();
    auto iter = lock->find(dn);
    if (iter == lock->end()) {
      return nullptr;
    }
    return iter->second.get();
  }

protected:
  ShardRes<us_map<std::unique_ptr<std::forward_list<RR>>>> cache;
};
XSL_NET_DNS_NE
#endif
