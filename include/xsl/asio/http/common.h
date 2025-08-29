/**
 * @file common.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP common definitions
 * @version 0.1.0
 * @date 2025-07-10
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_ASIO_HTTP_COMMON
#  define XSL_ASIO_HTTP_COMMON
#  include <xsl/asio/buf.h>
#  include <xsl/asio/http/def.h>
#  include <xsl/byte.h>
#  include <xsl/def.h>
#  include <xsl/io.h>
#  include <xsl/net.h>

XSL_ASIO_HTTP_NB
const int DEFAULT_HEADER_COUNT = 16;

const std::size_t HTTP_BUFFER_BLOCK_SIZE = 4096;

struct Message {
public:
  constexpr Message() : rest(), buffer() {
    this->buffer.emplace_back(std::make_unique<byte[]>(HTTP_BUFFER_BLOCK_SIZE));
  }
  Message(Message&&) = default;
  Message& operator=(Message&&) = default;
  ~Message() {}

  /// @brief check if the request has the header
  [[nodiscard]]
  constexpr bool has_header(std::string_view key) {
    return this->rest.headers.find(key) != this->rest.headers.end();
  }
  /// @brief get the header
  [[nodiscard]]
  constexpr std::optional<std::string_view> get_header(std::string_view key) {
    auto iter = this->rest.headers.find(key);
    if (iter == this->rest.headers.end()) {
      return std::nullopt;
    }
    return iter->second;
  }

  xsl::http::MessageRestView rest;

  std::vector<std::unique_ptr<byte[]>> buffer;
  template <AsyncRead R>
  Task<errc> read(this auto&& self, R& reader) {
    Buffer<HTTP_BUFFER_BLOCK_SIZE> buf;
    buf.buffers().reserve(self.buffer.size());

    std::size_t parsed_end = 0;
    auto data_view = [&]() -> std::string_view {
      return std::string_view(
          reinterpret_cast<const char*>(buf.buffers().back().get() + parsed_end),
          buf.current_size() - parsed_end);
    };
    while (true) {
      auto res = co_await buf.write(reader);
      if (!res) co_return std::move(res.ec);
      auto [len, ec] = self.line.parse(data_view());
      if (ec == errc::resource_unavailable_try_again) continue;
      if (ec != errc{}) co_return ec;
      parsed_end += len;
      break;
    }
    self.rest.clear();
    while (true) {
      auto [len, ec] = self.rest.parse(data_view());
      if (ec == errc{}) break;
      if (ec != errc::resource_unavailable_try_again) co_return ec;
      parsed_end += len;
      if (buf.is_full()) {
        if (!self.buffer.empty()) {
          std::copy_n(buf.buffers().back().get() + parsed_end, HTTP_BUFFER_BLOCK_SIZE - parsed_end,
                      self.buffer.back().get());
          buf.buffers().emplace_back(std::move(self.buffer.back()));
          self.buffer.pop_back();
          buf.current_size() -= parsed_end;
        } else {
          buf.write(buf.buffers().back().get() + parsed_end, HTTP_BUFFER_BLOCK_SIZE - parsed_end);
        }
        parsed_end = 0;
      }

      auto res = co_await buf.write(reader);
      if (!res) co_return std::move(res.ec);
    }
    buf.buffers().insert(buf.buffers().begin(), std::move_iterator(self.buffer.begin()),
                         std::move_iterator(self.buffer.end()));
    self.buffer.swap(buf.buffers());
    co_return {};
  }
};

XSL_ASIO_HTTP_NE
#endif
