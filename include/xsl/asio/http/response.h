/**
 * @file response.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP response definitions
 * @version 0.1.0
 * @date 2025-07-11
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP_RESPONSE
#  define XSL_ASIO_HTTP_RESPONSE
#  include <xsl/asio/http/common.h>
#  include <xsl/asio/http/def.h>
#  include <xsl/net.h>
#  include <xsl/regex.h>

#  include <array>
#  include <cstdint>
#  include <cstring>
#  include <string>
#  include <vector>
XSL_ASIO_NB
using xsl::http::Status;
using xsl::http::Version;

struct StatusLineView {
  std::string_view version = {};         ///< the HTTP version
  Status status_code = Status::UNKNOWN;  ///< the status code
  std::string_view status_message = {};  ///< the status message, default is empty

  constexpr std::tuple<size_t, errc> parse(std::string_view view) {
    errc res = errc::illegal_byte_sequence;
    size_t pos = 0;
    size_t end = view.find("\r\n", pos);
    /// @note if the end is not found, it means the request is not complete
    if (end == std::string_view::npos) return {0, errc::resource_unavailable_try_again};
    auto line = view.substr(pos, end - pos);
    size_t _1sp = line.find(' ');
    size_t _2sp = line.find(' ', _1sp + 1);
    /// @note if the separator is not found, it means the request line is invalid
    if (_1sp == std::string_view::npos || _2sp == std::string_view::npos) return {0, res};
    auto tmp_version = line.substr(0, _1sp);
    if (!std::regex_match(tmp_version.begin(), tmp_version.end(), regex::http_version_re))
      return {0, res};
    version = tmp_version;
    status_code = Status::from_string_view(line.substr(_1sp + 1, _2sp - _1sp - 1));
    if (status_code == Status::UNKNOWN) return {0, res};
    status_message = line.substr(_2sp + 1);
    pos = end + 2;
    return {pos, {}};
  }
};
/// @brief the response
class Response : public Message {  // TODO: abstract the ard
public:
  constexpr Response() : Message(), line() {}

  StatusLineView line;
};

/**
 * @brief fixed-capacity inline header storage with linear scan and POD slots
 *
 * Real responses carry a handful of headers; a hash map costs 6-8 heap
 * allocations plus hashing per request. This container keeps up to
 * inline_capacity entries in an inline POD array: zero allocations on the
 * hot path and a cheap trivially-copyable move. Keys longer than
 * key_buffer_size, values longer than value_buffer_size, or entries beyond
 * inline_capacity spill into a vector (never reached for the built-in
 * components).
 *
 * @note emplace replicates unordered_map::emplace first-wins semantics: an
 *       existing key is left untouched (set_header relies on this for
 *       idempotence). Lookups are case-sensitive, like the code that used
 *       the previous unordered_map.
 */
class SmallHeaderMap {
public:
  static constexpr std::size_t inline_capacity = 8;
  static constexpr std::size_t key_buffer_size = 40;
  static constexpr std::size_t value_buffer_size = 64;

  struct EntryView {
    std::string_view key;
    std::string_view value;
  };

  /// @brief insert unless the key already exists (first-wins); keys/values
  ///        may be anything convertible to string_view
  template <class K, class V>
    requires std::convertible_to<K, std::string_view>
                 && std::convertible_to<V, std::string_view>
  bool emplace(K&& key, V&& value) {
    std::string_view k(key), v(value);
    if (this->contains(k)) return false;
    if (this->count_ < inline_capacity && k.size() <= key_buffer_size
        && v.size() <= value_buffer_size) {
      auto& entry = this->slots_[this->count_++];
      std::memcpy(entry.key, k.data(), k.size());
      std::memcpy(entry.value, v.data(), v.size());
      entry.key_len = static_cast<std::uint16_t>(k.size());
      entry.value_len = static_cast<std::uint16_t>(v.size());
      return true;
    }
    this->overflow_.emplace_back(std::string(k), std::string(v));
    return true;
  }
  /// @brief whether the (case-sensitive) key is present
  bool contains(std::string_view key) const {
    for (std::size_t i = 0; i < this->count_; ++i) {
      if (this->slots_[i].key_view() == key) return true;
    }
    for (const auto& entry : this->overflow_) {
      if (entry.first == key) return true;
    }
    return false;
  }
  /// @brief number of entries (inline + spilled)
  [[nodiscard]] std::size_t size() const { return this->count_ + this->overflow_.size(); }
  /// @brief entry access in insertion order (inline slots first, then spilled)
  EntryView entry(std::size_t index) const {
    if (index < this->count_) {
      return {this->slots_[index].key_view(), this->slots_[index].value_view()};
    }
    auto& entry = this->overflow_[index - this->count_];
    return {entry.first, entry.second};
  }

private:
  struct Entry {
    std::uint16_t key_len = 0;
    std::uint16_t value_len = 0;
    char key[key_buffer_size];
    char value[value_buffer_size];

    std::string_view key_view() const { return {key, key_len}; }
    std::string_view value_view() const { return {value, value_len}; }
  };

  std::size_t count_ = 0;
  std::array<Entry, inline_capacity> slots_{};
  std::vector<std::pair<std::string, std::string>> overflow_{};  ///< long or excessive entries
};

/// @brief the response part
class ResponsePart {
public:
  ResponsePart();
  ResponsePart(Version version, Status status_code, std::string_view&& status_message);
  ResponsePart(Version version, Status status_code);
  ResponsePart(Version version, uint16_t status_code);
  ResponsePart(Status status_code);
  ResponsePart(uint16_t status_code);
  ResponsePart(ResponsePart&&) = default;
  ResponsePart& operator=(ResponsePart&&) = default;
  ~ResponsePart();
  Status status_code;
  std::string_view status_message;
  Version version;
  SmallHeaderMap headers;
  std::string to_string();
  /// @brief render the head into the caller-provided buffer, zero-alloc
  /// @return the rendered head as a view into buf, empty if it does not fit
  ///        (callers fall back to to_string())
  std::string_view render_head(char* buf, std::size_t cap);
};
/// @brief the response
template <AsyncWrite W>
class ResponseBuilder {  // TODO: abstract the body
public:
  constexpr ResponseBuilder(ResponsePart&& part, auto&&... args)
      : _part(std::move(part)), _body(std::forward<decltype(args)>(args)...) {}
  /// @brief builder with a string body: sendto() writes head and body with a
  ///        single ::writev syscall (no std::function indirection)
  constexpr ResponseBuilder(ResponsePart&& part, std::string body)
      : _part(std::move(part)), _raw_body(std::move(body)) {}
  constexpr ResponseBuilder(ResponseBuilder&&) = default;
  constexpr ResponseBuilder& operator=(ResponseBuilder&&) = default;
  ~ResponseBuilder() {}

  /**
   * @brief Insert a header into the response part.
   *
   * @param key, the header key
   * @param value, the header value
   * @return ResponseBuilder&, a reference to the current ResponseBuilder instance
   */
  constexpr ResponseBuilder& set_header(std::string key, std::string value) {
    this->_part.headers.emplace(std::move(key), std::move(value));
    return *this;
  }

  Task<io::Result> sendto(W& awd) {
    char head_buf[1024];  // lives in this coroutine frame: no heap allocation
    std::string head_str; // fallback storage when the headers overflow the frame buffer
    std::string_view head = this->_part.render_head(head_buf, sizeof(head_buf));
    if (head.empty()) head = head_str = this->_part.to_string();
    std::span<const byte> head_span{reinterpret_cast<const byte*>(head.data()), head.size()};
    if (!this->_raw_body.empty()) {
      std::span<const byte> body_span{reinterpret_cast<const byte*>(this->_raw_body.data()),
                                      this->_raw_body.size()};
      if constexpr (requires { awd->writev(std::span<const std::span<const byte>>{}); }) {
        // head and body in a single ::writev syscall
        std::array<std::span<const byte>, 2> chunks{head_span, body_span};
        co_return co_await awd->writev(std::span<const std::span<const byte>>(chunks));
      } else {
        auto res = co_await awd->write(head_span);
        if (!res) co_return res;
        auto body_res = co_await awd->write(body_span);
        body_res.size += res.size;
        co_return body_res;
      }
    }
    auto res = co_await awd->write(head_span);
    if (!res) co_return res;
    if (!this->_body) co_return res;
    auto body_res = co_await this->_body(awd);
    body_res.size += res.size;
    co_return body_res;
  }
  ResponsePart _part;
  std::function<Task<io::Result>(W&)> _body;
  std::string _raw_body;
};
XSL_ASIO_NE
#endif
