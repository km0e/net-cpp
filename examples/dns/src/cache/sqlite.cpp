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
#include <cache/sqlite.h>
#include <xsl/def.h>
#include <xsl/error.h>
#include <xsl/wheel.h>

using namespace xsl;

Expected<std::unique_ptr<SqliteCache>> SqliteCache::create(const char *uri) {
  auto cache = std::make_unique<SqliteCache>();
  EXPECT(sqlite3_open_v2(uri, std::out_ptr(cache->db),
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, nullptr),
         SQLITE_OK, Error(e, sqlite3_errmsg(cache->db.get())));

  const char *sql
      = "CREATE TABLE IF NOT EXISTS dns_cache ("
        "name TEXT NOT NULL, "
        "type INTEGER NOT NULL, "
        "class INTEGER NOT NULL, "
        "ttl INTEGER NOT NULL, "
        "rddata BLOB NOT NULL, "
        "timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')), "
        "UNIQUE(name, type, class)"
        ");";
  std::string_view sql_query
      = "SELECT rowid, rddata, ttl, timestamp FROM dns_cache WHERE name = ? AND type = ? AND "
        "class = ?";
  std::string_view sql_insert
      = "INSERT OR REPLACE INTO dns_cache (name, type, class, ttl, rddata) VALUES (?, ?, ?, ?, "
        "?)";
  std::string_view sql_delete = "DELETE FROM dns_cache WHERE rowid = ?";
  char *err_msg = nullptr;
  Defer defer{[&err_msg]() {
    if (err_msg) sqlite3_free(err_msg);
  }};

  auto db = cache->db.get();
  EXPECT(sqlite3_exec(db, sql, nullptr, nullptr, &err_msg), SQLITE_OK,
         Error(e, std::string(err_msg)));
  EXPECT_N(SQLITE_OK, Error(e, sqlite3_errmsg(db)),
           sqlite3_prepare_v3(db, sql_query.data(), static_cast<int>(sql_query.size()),
                              SQLITE_PREPARE_PERSISTENT, std::out_ptr(cache->query), nullptr),
           sqlite3_prepare_v3(db, sql_delete.data(), static_cast<int>(sql_delete.size()),
                              SQLITE_PREPARE_PERSISTENT, std::out_ptr(cache->del), nullptr),
           sqlite3_prepare_v3(db, sql_insert.data(), static_cast<int>(sql_insert.size()),
                              SQLITE_PREPARE_PERSISTENT, std::out_ptr(cache->insert), nullptr));
  return cache;
}

SqliteCache::SqliteCache() {}

constexpr bool SqliteCache::is_valid() const { return db != nullptr && query != nullptr; }

Expected<RR> SqliteCache::get(std::string_view name, Type type = Type::A,
                              Class class_ = Class::IN) {
  auto eh = error_handler();
  EXPECT_N(
      SQLITE_OK, eh(e),
      sqlite3_bind_text(query.get(), 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC),
      sqlite3_bind_int(query.get(), 2, type._type),
      sqlite3_bind_int(query.get(), 3, class_._class));
  xsl::Defer defer{[this]() { sqlite3_reset(query.get()); }};
  auto ec = sqlite3_step(query.get());
  if (ec == SQLITE_DONE) {
    log_debug("No record found for name: {}, type: {}, class: {}", name, type, class_);
    return RR{nullptr};              // No record found
  } else if (ec != SQLITE_ROW) {     // TODO: handle SQLITE_BUSY
    return std::unexpected(eh(ec));  // Handle other errors
  }
  TRY(data, sqlite3_column_blob(query.get(), 1),
      Error(sqlite3_extended_errcode(db.get()), sqlite3_errmsg(db.get())));
  TRY(data_size, sqlite3_column_bytes(query.get(), 1),
      Error(sqlite3_extended_errcode(db.get()), sqlite3_errmsg(db.get())));
  int ttl = sqlite3_column_int(query.get(), 2);
  int timestamp = sqlite3_column_int(query.get(), 3);
  auto now = std::chrono::system_clock::now().time_since_epoch().count();
  if (ttl <= 0 || (now - timestamp) > ttl) {
    auto id = sqlite3_column_int(query.get(), 0);
    EXPECT(sqlite3_bind_int(del.get(), 1, id), SQLITE_OK, eh(e));
    Defer defer_del{[this]() { sqlite3_reset(del.get()); }};
    EXPECT(sqlite3_step(del.get()), SQLITE_DONE, eh(e));  // TODO: handle SQLITE_BUSY
    return RR{nullptr};
  } else {
    auto rr = RR(data_size);
    rr.type(type);
    rr.class_(class_);
    rr.ttl(ttl - (now - timestamp));
    rr.rdata(static_cast<const byte *>(data), data_size);
    log_debug("Retrieved record for name: {}, type: {}, class: {}", name, type, class_);
    return rr;
  }
}
Expected<void> SqliteCache::put(std::string_view name, RRView v) {
  auto eh = error_handler();
  EXPECT_N(
      SQLITE_OK, eh(e),
      sqlite3_bind_text(insert.get(), 1, name.data(), static_cast<int>(name.size()),
                        SQLITE_TRANSIENT),
      sqlite3_bind_int(insert.get(), 2, v.type()._type),
      sqlite3_bind_int(insert.get(), 3, v.class_()._class),
      sqlite3_bind_int(insert.get(), 4, v.ttl()),
      sqlite3_bind_blob(insert.get(), 5, v.rdata(), static_cast<int>(v.rdlength()), SQLITE_STATIC));
  Defer defer{[this]() { sqlite3_reset(insert.get()); }};
  EXPECT(sqlite3_step(insert.get()), SQLITE_DONE, eh(e));  // TODO: handle SQLITE_BUSY
  return {};
}
