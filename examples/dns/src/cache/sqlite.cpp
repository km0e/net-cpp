#include <cache/sqlite.h>
#include <xsl/def.h>
#include <xsl/wheel.h>

using namespace xsl;

auto SqliteCache::error_handler() const {
  return [this](int err) -> std::error_condition {
    return make_error_condition(err, sqlite3_errmsg(db.get()));
  };
}

std::expected<void, std::error_condition> SqliteCache::open(const char *db_path) {
  EXPECT(sqlite3_open_v2(db_path, std::out_ptr(db),
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, nullptr),
         SQLITE_OK, [this](auto err) { return make_error_condition(err, db.get()); });
  auto db = this->db.get();
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
  EXPECT(sqlite3_exec(db, sql, nullptr, nullptr, &err_msg), SQLITE_OK, [&err_msg](auto err) {
    auto ec = make_error_condition(err, err_msg);
    sqlite3_free(err_msg);
    return ec;
  });

  EXPECT_N(
      SQLITE_OK, [&db](auto err) { return make_error_condition(err, db); },
      sqlite3_prepare_v3(db, sql_query.data(), static_cast<int>(sql_query.size()),
                         SQLITE_PREPARE_PERSISTENT, std::out_ptr(query), nullptr),
      sqlite3_prepare_v3(db, sql_delete.data(), static_cast<int>(sql_delete.size()),
                         SQLITE_PREPARE_PERSISTENT, std::out_ptr(del), nullptr),
      sqlite3_prepare_v3(db, sql_insert.data(), static_cast<int>(sql_insert.size()),
                         SQLITE_PREPARE_PERSISTENT, std::out_ptr(insert), nullptr));
  return {};
}
SqliteCache::SqliteCache() {}

constexpr bool SqliteCache::is_valid() const { return db != nullptr && query != nullptr; }

std::expected<RR, std::error_condition> SqliteCache::get(std::string_view name, Type type = Type::A,
                                                         Class class_ = Class::IN) {
  auto eh = error_handler();
  EXPECT_N(
      SQLITE_OK, eh,
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
  TRY(sqlite3_column_blob(query.get(), 1), data, make_error_condition(db.get()));
  TRY(sqlite3_column_bytes(query.get(), 1), data_size, make_error_condition(db.get()));
  int ttl = sqlite3_column_int(query.get(), 2);
  int timestamp = sqlite3_column_int(query.get(), 3);
  auto now = std::chrono::system_clock::now().time_since_epoch().count();
  if (ttl <= 0 || (now - timestamp) > ttl) {
    // log_debug("Record for name: {}, type: {}, class: {} has expired", name, type, class_);
    auto id = sqlite3_column_int(query.get(), 0);
    EXPECT(sqlite3_bind_int(del.get(), 1, id), SQLITE_OK, eh);
    Defer defer_del{[this]() { sqlite3_reset(del.get()); }};
    EXPECT(sqlite3_step(del.get()), SQLITE_DONE, eh);  // TODO: handle SQLITE_BUSY
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
std::expected<void, std::error_condition> SqliteCache::put(std::string_view name, RRView v) {
  auto eh = error_handler();
  EXPECT_N(
      SQLITE_OK, eh,
      sqlite3_bind_text(insert.get(), 1, name.data(), static_cast<int>(name.size()),
                        SQLITE_TRANSIENT),
      sqlite3_bind_int(insert.get(), 2, v.type()._type),
      sqlite3_bind_int(insert.get(), 3, v.class_()._class),
      sqlite3_bind_int(insert.get(), 4, v.ttl()),
      sqlite3_bind_blob(insert.get(), 5, v.rdata(), static_cast<int>(v.rdlength()), SQLITE_STATIC));
  Defer defer{[this]() { sqlite3_reset(insert.get()); }};
  EXPECT(sqlite3_step(insert.get()), SQLITE_DONE, eh);  // TODO: handle SQLITE_BUSY
  return {};
}
