/**
 * @file sqlite.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief SQLite-based DNS cache implementation
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_APP_DNS_CACHE_SQLITE_H
#  define XSL_APP_DNS_CACHE_SQLITE_H

#  include <cache/def.h>
#  include <sqlite3.h>
#  include <xsl/error.h>
#  include <xsl/net.h>

#  include <memory>
using namespace xsl;

class SqliteCache : public DnsCache {
  inline auto error_handler() const {
    return [this](auto e) { return Error(e, sqlite3_errmsg(db.get())); };
  }

public:
  static Expected<std::unique_ptr<SqliteCache>> create(const char *uri);
  SqliteCache();
  SqliteCache(const SqliteCache &) = delete;
  SqliteCache &operator=(const SqliteCache &) = delete;
  SqliteCache(SqliteCache &&) = default;
  SqliteCache &operator=(SqliteCache &&) = default;
  ~SqliteCache() override = default;

  constexpr bool is_valid() const;

  Expected<RR> get(std::string_view name, Type type, Class class_) override;
  Expected<void> put(std::string_view name, RRView v) override;

private:
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db{nullptr, &sqlite3_close};
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> query{nullptr, &sqlite3_finalize},
      del{nullptr, &sqlite3_finalize}, insert{nullptr, &sqlite3_finalize};
};
#endif
