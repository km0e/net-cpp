#pragma once

#ifndef XSL_NET_HTTP_PARSE
#  define XSL_NET_HTTP_PARSE
#  include "xsl/ai.h"
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

struct ParseData {
  ParseData() noexcept : buffer{}, request{}, content_part{} {}
  ParseData(io::Buffer<>&& buffer, RequestView&& request, std::string_view content_part)
      : buffer(std::move(buffer)), request(std::move(request)), content_part(content_part) {}
  ParseData(ParseData&&) = default;
  ParseData& operator=(ParseData&&) = default;
  io::Buffer<> buffer;
  RequestView request;
  std::string_view content_part;
};

template <class Traits = HttpParseTraits>
class Parser {
public:
  using parser_type = typename Traits::parser_type;
  using request_type = RequestView;

  Parser() : buffer{}, used_size(0), parsed_size(0), parser{} {
    buffer.append(HTTP_BUFFER_BLOCK_SIZE);
  }

  Parser(Parser&&) = default;
  Parser& operator=(Parser&&) = default;
  ~Parser() {}
  template <class Executor = coro::ExecutorBase, ai::ABRL Reader>
  Task<std::expected<void, std::errc>, Executor> read(Reader& reader, ParseData& buf) {
    while (true) {
      auto [sz, err] = co_await ai::read_poly_resolve<Executor>(
          reader, this->buffer.front().span(this->used_size));
      if (err) {
        co_return std::unexpected{*err};
      }
      this->used_size += sz;
      auto req = this->parse_request(sz, buf);
      if (req || req.error() != std::errc::resource_unavailable_try_again) {
        this->used_size = 0;
        this->parsed_size = 0;
        co_return std::move(req);
      }
    }
  }
  Task<std::expected<void, std::errc>> read(ABR& reader, ParseData& buf) {
    return this->read<coro::ExecutorBase>(reader, buf);
  }
  void reset() {
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

  std::expected<void, std::errc> parse_request(std::size_t sz, ParseData& buf) {
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
    return std::unexpected{req.error()};
  }
};

XSL_HTTP_NE
#endif
