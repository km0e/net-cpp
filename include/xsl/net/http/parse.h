#pragma once
#ifndef _XSL_NET_HTTP_PARSE_H_
#  define _XSL_NET_HTTP_PARSE_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/wheel.h"

#  include <array>
#  include <string>
HTTP_NAMESPACE_BEGIN
enum class ParseErrorKind {
  Unknown,
  Partial,
  InvalidFormat,
};
const int PARSE_ERROR_COUNT = 3;
const std::array<std::string_view, PARSE_ERROR_COUNT> PARSE_ERROR_STRINGS = {
    "Unknown",
    "Partial",
    "InvalidFormat",
};
class ParseError {
public:
  ParseError(ParseErrorKind kind);
  ParseError(ParseErrorKind kind, std::string message);
  ~ParseError();
  ParseErrorKind kind;
  std::string message;
  std::string to_string() const;
};
using ParseResult = Result<RequestView, ParseError>;

class HttpParser {
public:
  HttpParser();
  HttpParser(HttpParser&&) = default;
  ~HttpParser();
  /**
   * @brief parse the http request
   * @param data: the data to be parsed
   * @param len: the length of the data, will be updated to the last position of the correct request
   * @return the parsed request or the error
   * @note if the return value is an error and the kind is InvalidFormat, will update the len to the
   * last correct position
   * @par Example
   * @details
   * "GET / HTTP/1.1"
   * "Host= localhost:8080" # should be ":", not "=", so the len will be updated to 16
   */
  ParseResult parse(const char* data, size_t& len);
  /**
   * @brief parse the http request
   * @param data: the data to be parsed
   * @return the parsed request or the error
   * @note if the return value is an error and the kind is InvalidFormat, will update the len to the
   * last correct position
   * @par Example
   * @details
   * "GET / HTTP/1.1"
   * "Host= localhost:8080" # should be ":", not "=", so the len will be updated to 16
   */
  ParseResult parse(std::string_view& data);
  RequestView view;
  void clear();
};
HTTP_NAMESPACE_END
#endif
