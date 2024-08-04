#pragma once

#ifndef XSL_NET_HTTP_PARSE
#  define XSL_NET_HTTP_PARSE
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/io/buffer.h"

#  include <cstddef>
#  include <expected>
#  include <memory>
#  include <string_view>
#  include <tuple>
#  include <utility>
HTTP_NB
using ParseResult = std::tuple<std::size_t, std::expected<RequestView, std::errc>>;

class HttpParseUnit {
public:
  HttpParseUnit();
  HttpParseUnit(HttpParseUnit&&) = default;
  ~HttpParseUnit() = default;
  /**
   @brief parse the http request
   @param data: the data to be parsed
   @param len: the length of the data, will be updated to the last position of the correct request
   @return the parsed request or the error
   @note if the return value is an error and the kind is InvalidFormat, will update the len to the
   last correct position
   @par Example
   @details
    "GET / HTTP/1.1"
    "Host= localhost:8080" # should be ":", not "=", so the len will be updated to 16
   */
  ParseResult parse(const char* data, size_t len);
  /**
   @brief parse the http request
   @param data: the data to be parsed
   @return the parsed request or the error
   @note if the return value is an error and the kind is InvalidFormat, will update the len to the
   last correct position
   @par Example
   @details
   "GET / HTTP/1.1"
   "Host= localhost:8080" # should be ":", not "=", so the len will be updated to 16
   */
  ParseResult parse(std::string_view data);
  RequestView view;
  void clear();
};

const std::size_t HTTP_BUFFER_BLOCK_SIZE = 1024;

struct HttpParseTrait {
  using parse_unit = HttpParseUnit;
};

template <class Trait = HttpParseTrait>
class HttpParser {
public:
  using parse_unit = typename Trait::parse_unit;
  using request_type = Request;

  HttpParser(sys::io::AsyncDevice<feature::In<std::byte>>&& ard)
      : _ard(std::make_shared<sys::io::AsyncDevice<feature::In<std::byte>>>(std::move(ard))),
        buffer(),
        used_size(0),
        parsed_size(0),
        parser() {
    buffer.append(HTTP_BUFFER_BLOCK_SIZE);
  }
  HttpParser(HttpParser&&) = default;
  ~HttpParser() {}
  template <class Executor = coro::ExecutorBase>
  coro::Task<std::expected<request_type, std::errc>, Executor> recv() {
    while (true) {
      auto [sz, err] = co_await sys::net::immediate_recv<Executor>(
          *this->_ard, this->buffer.front().span(this->used_size));
      if (err) {
        LOG3("recv error: {}", std::make_error_code(*err).message());
        continue;
      }
      this->used_size += sz;
      auto req = this->parse_request(sz);
      if (req || req.error() != std::errc::resource_unavailable_try_again) {
        co_return std::move(req);
      }
    }
  }

private:
  std::shared_ptr<sys::io::AsyncDevice<feature::In<std::byte>>> _ard;
  io::Buffer buffer;
  std::size_t used_size;
  std::size_t parsed_size;
  parse_unit parser;

  std::expected<request_type, std::errc> parse_request(std::size_t sz) {
    auto& front = this->buffer.front();
    auto [len, req] = this->parser.parse(
        reinterpret_cast<const char*>(front.data.get() + this->parsed_size), sz);
    if (req) {
      auto body = BodyStream(
          this->_ard,
          std::string_view(
              reinterpret_cast<const char*>(front.data.get() + this->parsed_size + len), sz - len));
      this->used_size = 0;
      this->parsed_size = 0;
      return request_type{std::exchange(this->buffer, {}), std::move(*req), std::move(body)};
    }
    if (req.error() == std::errc::resource_unavailable_try_again) {
      this->parsed_size += len;
      if (this->used_size == HTTP_BUFFER_BLOCK_SIZE) {
        io::Block block{HTTP_BUFFER_BLOCK_SIZE};
        std::copy(front.data.get() + this->parsed_size, front.data.get() + HTTP_BUFFER_BLOCK_SIZE,
                  block.data.get());
        this->buffer.append(std::move(block));
        this->used_size = HTTP_BUFFER_BLOCK_SIZE - this->parsed_size;
        this->parsed_size = 0;
      }
    }
    LOG3("parse error: {}", std::make_error_code(req.error()).message());
    return std::unexpected{req.error()};
  }
};

HTTP_NE
#endif
