#pragma once
#ifndef XSL_APP_DNS_CACHE_SQLITE_H
#  define XSL_APP_DNS_CACHE_SQLITE_H

#  include <cache/def.h>
#  include <sqlite3.h>
#  include <xsl/net.h>

#  include <expected>
#  include <memory>

class sqlite3_category : public std::error_category {
public:
  constexpr sqlite3_category(const char *msg) : _msg(msg) {}
  constexpr sqlite3_category() : _msg("SQLite error") {}
  const char *name() const noexcept override { return "sqlite3"; }
  std::string message(int ev) const override {
    if (ev == 0) return _msg;
    return std::string("SQLite error code: ") + std::to_string(ev);
  }
  std::error_condition default_error_condition(int ev) const noexcept override {
    return std::error_condition(ev, *this);
  }

private:
  std::string _msg;
};
constexpr std::error_condition make_error_condition(sqlite3 *db) {
  return std::error_condition(sqlite3_extended_errcode(db), sqlite3_category(sqlite3_errmsg(db)));
}
constexpr std::error_condition make_error_condition(int ev, sqlite3 *db) {
  return std::error_condition(ev, sqlite3_category(sqlite3_errmsg(db)));
}
constexpr std::error_condition make_error_condition(int ev, const char *msg) {
  return std::error_condition(ev, sqlite3_category(msg));
}

class SqliteCache : public DnsCache {
  auto error_handler() const;

public:
  SqliteCache();
  SqliteCache(const SqliteCache &) = delete;
  SqliteCache &operator=(const SqliteCache &) = delete;
  SqliteCache(SqliteCache &&) = default;
  SqliteCache &operator=(SqliteCache &&) = default;
  ~SqliteCache() override = default;

  std::expected<void, std::error_condition> open(const char *db_path);
  constexpr bool is_valid() const;

  std::expected<RR, std::error_condition> get(std::string_view name, Type type,
                                              Class class_) override;
  std::expected<void, std::error_condition> put(std::string_view name, RRView v) override;

private:
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db{nullptr, &sqlite3_close};
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> query{nullptr, &sqlite3_finalize},
      del{nullptr, &sqlite3_finalize}, insert{nullptr, &sqlite3_finalize};
};
#endif
