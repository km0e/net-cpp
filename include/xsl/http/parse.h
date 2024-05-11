#pragma once
#ifndef _XSL_NET_HTTP_PARSE_H_
#  define _XSL_NET_HTTP_PARSE_H_
#  include "xsl/http/http.h"
#  include "xsl/http/msg.h"
#  include "xsl/utils/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
enum class ParseErrorKind {
  Unknown,
  Partial,
  InvalidFormat,
};
class ParseError {
public:
  ParseError(ParseErrorKind kind);
  ParseError(ParseErrorKind kind, wheel::string message);
  ~ParseError();
  ParseErrorKind kind;
  wheel::string message;
};
using ParseResult = wheel::Result<RequestView, ParseError>;

class HttpParser {
public:
  HttpParser();
  ~HttpParser();
  ParseResult parse(const char* data, size_t& len);
  RequestView view;
};
HTTP_NAMESPACE_END
#endif
