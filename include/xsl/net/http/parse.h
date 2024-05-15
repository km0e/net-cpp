#pragma once
#ifndef _XSL_NET_HTTP_PARSE_H_
#  define _XSL_NET_HTTP_PARSE_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/wheel.h"
HTTP_NAMESPACE_BEGIN
enum class ParseErrorKind {
  Unknown,
  Partial,
  InvalidFormat,
};
const int PARSE_ERROR_COUNT = 3;
const array<string_view, PARSE_ERROR_COUNT> PARSE_ERROR_STRINGS = {
    "Unknown",
    "Partial",
    "InvalidFormat",
};
class ParseError {
public:
  ParseError(ParseErrorKind kind);
  ParseError(ParseErrorKind kind, string message);
  ~ParseError();
  ParseErrorKind kind;
  string message;
  std::string to_string() const;
};
using ParseResult = Result<RequestView, ParseError>;

class HttpParser {
public:
  HttpParser();
  HttpParser(HttpParser&&) = default;
  ~HttpParser();
  // brief: parse the http request
  // you should keep the memory of the view until the next parse
  // @param data: the data to be parsed
  // @param len: the length of the data, if there is a complete request, the len will be updated to
  // the end of the request
  // @return: the parsed request or the error
  ParseResult parse(const char* data, size_t& len);
  RequestView view;
};
HTTP_NAMESPACE_END
#endif
