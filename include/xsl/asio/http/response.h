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

#  include <cstdint>
XSL_ASIO_HTTP_NB
using namespace xsl::http;

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
  us_map<std::string> headers;
  std::string to_string();
};
/// @brief the response
template <AsyncWrite W>
class ResponseBuilder {  // TODO: abstract the body
public:
  constexpr ResponseBuilder(ResponsePart&& part, auto&&... args)
      : _part(std::move(part)), _body(std::forward<decltype(args)>(args)...) {}
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
    auto str = this->_part.to_string();  // TODO:write directly
    log_trace("response: {}", str);
    auto res = co_await awd.write(str.data(), str.size());
    if (!res) co_return res;
    if (!_body) co_return {res.size};
    auto body_res = co_await this->_body(awd);
    body_res.size += res.size;
    co_return body_res;
  }
  ResponsePart _part;
  std::function<Task<io::Result>(W&)> _body;
};
XSL_ASIO_HTTP_NE
#endif
