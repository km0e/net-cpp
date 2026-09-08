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
#  include <xsl/asio/http/def.h>
#  include <xsl/byte.h>
#  include <xsl/def.h>
#  include <xsl/io.h>
#  include <xsl/net.h>

#  include <cstring>

XSL_ASIO_NB
const int DEFAULT_HEADER_COUNT = 16;

const std::size_t HTTP_BUFFER_BLOCK_SIZE = 4096;

/**
 * @brief HTTP message head reader (request line/status line + headers)
 *
 * The read buffer is a chain of fixed-size blocks owned by the Message and
 * reused across reads: `buffer[win_block]` is the active block, the parse
 * window is `[win_begin, win_end)` inside it. When the active block fills
 * up, the unconsumed tail is carried over into the next block (rotate), so
 * the parse window stays contiguous.
 *
 * View lifetime: the parsed fields (request line, header map) are views
 * INTO the blocks and must stay valid until the next read. Therefore no
 * byte is moved between the end of one read and the start of the next:
 * at read() entry the previous request's views are dead, and the still
 * unconsumed window (a pipelined next head) is compacted to the front of
 * `buffer[0]`; fully consumed blocks become scratch and are recycled by
 * later rotates. The block count is bounded by the largest head seen on
 * this connection, never by the number of requests — O(1) per request.
 */
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
  std::size_t win_block = 0;  ///< index of the active block in the chain
  std::size_t win_begin = 0;  ///< parse window start in the active block
  std::size_t win_end = 0;    ///< filled-data end in the active block

  template <AsyncRead R>
  Task<errc> read(this auto&& self, R& reader) {
    // Entry compaction: the previous request's views are no longer live, so
    // the unconsumed tail (a pipelined next head) can be moved to the front
    // of buffer[0] and the chain reset; consumed blocks become scratch
    {
      std::size_t leftover = self.win_end - self.win_begin;
      if (self.win_block != 0) {
        // move the leftover to the front of the ACTIVE block, then make that
        // block buffer[0] (the old buffer[0] becomes recycled scratch)
        if (leftover > 0 && self.win_begin != 0) {
          std::memmove(self.buffer[self.win_block].get(),
                       self.buffer[self.win_block].get() + self.win_begin, leftover);
        }
        std::swap(self.buffer[0], self.buffer[self.win_block]);
        self.win_block = 0;
      } else if (leftover > 0 && self.win_begin != 0) {
        std::memmove(self.buffer[0].get(), self.buffer[0].get() + self.win_begin, leftover);
      }
      self.win_begin = 0;
      self.win_end = leftover;
    }
    auto window = [&]() -> std::string_view {
      return std::string_view(
          reinterpret_cast<const char*>(self.buffer[self.win_block].get() + self.win_begin),
          self.win_end - self.win_begin);
    };
    /// carry the unconsumed tail into the next block, keeping the window
    /// contiguous; called only when the active block is full
    auto rotate = [&]() -> errc {
      std::size_t tail = self.win_end - self.win_begin;
      if (tail == HTTP_BUFFER_BLOCK_SIZE) {
        // a single line filled a whole block without terminating: reject
        // (the previous implementation silently corrupted such input)
        return errc::illegal_byte_sequence;
      }
      if (self.win_block + 1 == self.buffer.size()) {
        self.buffer.emplace_back(std::make_unique<byte[]>(HTTP_BUFFER_BLOCK_SIZE));
      }
      ++self.win_block;
      std::memcpy(self.buffer[self.win_block].get(),
                  self.buffer[self.win_block - 1].get() + self.win_begin, tail);
      self.win_begin = 0;
      self.win_end = tail;
      return errc{};
    };
    /// fetch more bytes into the window; rotate first if the block is full
    auto fill = [&]() -> Task<errc> {
      if (self.win_end == HTTP_BUFFER_BLOCK_SIZE) {
        errc ec = rotate();
        if (ec != errc{}) co_return ec;
      }
      auto res = co_await reader->read(self.buffer[self.win_block].get() + self.win_end,
                                       HTTP_BUFFER_BLOCK_SIZE - self.win_end);
      if (!res) co_return std::move(res.ec);
      if (res.size == 0) {
        // stream closed without delivering more data: avoid spinning forever
        // (connection-based readers already report this as not_connected)
        co_return errc::not_connected;
      }
      self.win_end += res.size;
      co_return errc{};
    };

    self.rest.clear();
    while (true) {
      auto [len, ec] = self.line.parse(window());
      if (ec == errc{}) {
        self.win_begin += len;
        break;
      }
      if (ec != errc::resource_unavailable_try_again) co_return ec;
      self.win_begin += len;  // len = start of the incomplete line (0 for line parsers)
      errc ec2 = co_await fill();
      if (ec2 != errc{}) co_return ec2;
    }
    while (true) {
      auto [len, ec] = self.rest.parse(window());
      if (ec == errc{}) {
        self.win_begin += len;
        break;
      }
      if (ec != errc::resource_unavailable_try_again) co_return ec;
      self.win_begin += len;  // len = start of the incomplete header line
      errc ec2 = co_await fill();
      if (ec2 != errc{}) co_return ec2;
    }
    // no compaction here: the views handed out for this head (request line,
    // headers) point into the blocks and must stay valid until the next read
    co_return errc{};
  }
};

XSL_ASIO_NE
#endif
