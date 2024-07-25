#pragma once
#ifndef XSL_NET_HTTP_PARSE
#  define XSL_NET_HTTP_PARSE
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"

#  include <string_view>
HTTP_NB
using ParseResult = std::expected<RequestView, std::errc>;

class HttpParser {
public:
  HttpParser();
  HttpParser(HttpParser&&) = default;
  ~HttpParser();
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
  ParseResult parse(const char* data, size_t& len);
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
  ParseResult parse(std::string_view& data);
  RequestView view;
  void clear();
};
HTTP_NE
#endif
