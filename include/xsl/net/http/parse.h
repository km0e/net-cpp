/**
 * @file parse.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Parse the http request
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_NET_HTTP_PARSE
#  define XSL_NET_HTTP_PARSE
#  include "xsl/io/def.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/io/buffer.h"

#  include <cstddef>
#  include <expected>
#  include <memory>
#  include <string_view>
#  include <tuple>
#  include <utility>
XSL_HTTP_NB
using namespace xsl::io;

using ParseResult = std::tuple<std::size_t, std::expected<RequestView, std::errc>>;

class ParseUnit {
public:
  ParseUnit();
  ParseUnit(ParseUnit&&) = default;
  ParseUnit& operator=(ParseUnit&&) = default;
  ~ParseUnit() = default;
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

private:
  void parse_request_target(std::string_view target);
};

const std::size_t HTTP_BUFFER_BLOCK_SIZE = 1024;

struct HttpParseTraits {
  using parser_type = ParseUnit;
};
/// @brief the parsed data
struct ParseData {
  io::Buffer<> buffer;
  RequestView request;
  std::string_view content_part;
};

template <class Traits = HttpParseTraits>
class Parser {
public:
  using parser_type = typename Traits::parser_type;
  using request_type = RequestView;

  constexpr Parser() : buffer{}, used_size(0), parsed_size(0), parser{} {
    buffer.append(HTTP_BUFFER_BLOCK_SIZE);
  }

  constexpr Parser(Parser&&) = default;
  constexpr Parser& operator=(Parser&&) = default;
  constexpr ~Parser() {}
  /**
   * @brief read the request
   *
   * @tparam Reader the reader type
   * @param reader the reader
   * @param buf the buffer to store the parsed data
   * @return Task<std::errc>
   */
  template <ABRL Reader>
  Task<std::errc> read(Reader& reader, ParseData& buf) {
    while (true) {
      auto [sz, err] = co_await reader.read(this->buffer.front().span(this->used_size));
      if (err) {
        co_return std::move(*err);
      }
      this->used_size += sz;
      if (auto err = this->parse_request(sz, buf);
          err == std::errc{} || err != std::errc::resource_unavailable_try_again) {
        this->used_size = 0;
        this->parsed_size = 0;
        co_return err;
      }
    }
  }
  constexpr void reset() {
    this->used_size = 0;
    this->parsed_size = 0;
    this->buffer.clear();
    this->buffer.append(HTTP_BUFFER_BLOCK_SIZE);
  }

private:
  io::Buffer<> buffer;
  std::size_t used_size;
  std::size_t parsed_size;
  parser_type parser;

  constexpr std::errc parse_request(std::size_t sz, ParseData& buf) {
    auto& front = this->buffer.front();
    auto [len, req] = this->parser.parse(
        reinterpret_cast<const char*>(front.data.get() + this->parsed_size), sz);
    if (req) {
      front.valid_size = this->parsed_size + sz;
      auto content_part = std::string_view(
          reinterpret_cast<const char*>(front.data.get() + this->parsed_size + len), sz - len);
      buf = ParseData{std::exchange(this->buffer, {}), std::move(*req), std::move(content_part)};
      this->reset();
      LOG6("parsed request: {} {}", buf.request.method, buf.request.path);
      return {};
    }
    if (req.error() == std::errc::resource_unavailable_try_again) {
      this->parsed_size += len;
      if (this->used_size == HTTP_BUFFER_BLOCK_SIZE) {
        io::Block block{HTTP_BUFFER_BLOCK_SIZE};
        std::copy(front.data.get() + this->parsed_size, front.data.get() + HTTP_BUFFER_BLOCK_SIZE,
                  block.data.get());
        front.valid_size = this->parsed_size;
        this->buffer.append(std::move(block));
        this->used_size = HTTP_BUFFER_BLOCK_SIZE - this->parsed_size;
        this->parsed_size = 0;
      }
    } else {
      this->reset();
    }
    LOG5("parse error: {}", std::make_error_code(req.error()).message());
    return req.error();
  }
};

XSL_HTTP_NE
XSL_IO_NB
template <class Traits>
struct AIOTraits<_net::http::Parser<Traits>> {
  using value_type = _net::http::ParseData;
  using device_type = _net::http::Parser<Traits>;
  template <ABRL Reader>
  static constexpr Task<std::errc> read(device_type& dev, Reader& reader, value_type& buf) {
    return dev.read(reader, buf);
  }
};
XSL_IO_NE
#endif
