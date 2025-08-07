#pragma once
#ifndef XSL_APP_DNS_CACHE_DEF_H
#  define XSL_APP_DNS_CACHE_DEF_H
#  include <xsl/def.h>
#  include <xsl/net.h>

#  include <expected>

using namespace xsl::dns;

struct DnsCache {
  virtual ~DnsCache() = default;

  /**
   * @brief Get a resource record from the cache
   *
   * @param name domain name
   * @param type type of the resource record
   * @param class_ class of the resource record
   * @return std::error_condition if the resource record an error occurs
   * @return RR the resource record if rr.is_valid()
   */
  virtual std::expected<RR, std::error_condition> get(std::string_view name, Type type,
                                                      Class class_)
      = 0;
  virtual std::expected<void, std::error_condition> put(std::string_view name, RRView v) = 0;
};

#endif
