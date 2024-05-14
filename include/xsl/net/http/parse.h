#pragma once
#ifndef _XSL_NET_HTTP_PARSE_H_
#  define _XSL_NET_HTTP_PARSE_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
enum class ParseErrorKind {
  Unknown,
  Partial,
  InvalidFormat,
};
const int PARSE_ERROR_COUNT = 3;
const wheel::array<wheel::string_view, PARSE_ERROR_COUNT> PARSE_ERROR_STRINGS = {
    "Unknown",
    "Partial",
    "InvalidFormat",
};
class ParseError {
public:
  ParseError(ParseErrorKind kind);
  ParseError(ParseErrorKind kind, wheel::string message);
  ~ParseError();
  ParseErrorKind kind;
  wheel::string message;
  std::string to_string() const;
};
using ParseResult = wheel::Result<RequestView, ParseError>;

class HttpParser {
public:
  HttpParser();
  HttpParser(HttpParser&&) = default;
  ~HttpParser();
  ParseResult parse(const char* data, size_t& len);
  RequestView view;
};
HTTP_NAMESPACE_END
#endif
