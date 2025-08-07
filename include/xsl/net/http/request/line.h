/**
 * @file line.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP request line definitions
 * @version 0.2.0
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_NET_HTTP_REQUEST_LINE
#  define XSL_NET_HTTP_REQUEST_LINE
#  include <xsl/def.h>
#  include <xsl/log.h>
#  include <xsl/net/http/def.h>
#  include <xsl/net/http/proto.h>
#  include <xsl/net/http/request/target.h>
#  include <xsl/regex.h>

#  include <regex>

XSL_HTTP_NB

struct RequestLine {
  std::string _raw = {};            ///< the raw request line if percent-encoded
  Method method = Method::UNKNOWN;  ///< the request method
  std::string_view scheme = {};     ///< the request scheme, default is http

  std::string_view host = {};  ///< the request authority host, default is empty
  std::string_view port = {};  /// authority port, 0 means not specified

  std::string_view path = "/";  ///< the request path, default is "/"
  std::string_view query = {};  ///< the request query, default is empty

  std::string_view version = {};  ///< the request version, default is empty

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
    method = Method::from_string_view(line.substr(0, _1sp));
    log_debug("method: {}", method);
    auto target = line.substr(_1sp + 1, _2sp - _1sp - 1);

    std::cmatch match;
    if (std::regex_match(target.begin(), target.end(), match, RequestTarget::regex_re)) {
#  define PERCENT_DECODE(field)              \
    {                                        \
      res = percent_decode(_raw, f.field);   \
      if (!res) return {0, res.error()};     \
      field = res->empty() ? f.field : *res; \
    }

      if (match[1].matched) {
        OriginForm f(std::ranges::subrange(match.begin() + 1, match.begin() + 3, 2));
        decltype(percent_decode(_raw, f.path)) res;
        PERCENT_DECODE(path);
        PERCENT_DECODE(query);
        log_debug("OriginForm: path={}, query={}", path, query);
      } else if (match[3].matched) {  /// absolute_form
        AbsoluteForm f(std::ranges::subrange(match.begin() + 3, match.begin() + 7, 4));
        decltype(percent_decode(_raw, f.path)) res;
        scheme = f.scheme;
        host = f.host;
        port = f.port;
        PERCENT_DECODE(path);
        path = path.empty() ? "/" : path;
        PERCENT_DECODE(query);
      } else if (match[8].matched) {
        AuthorityForm f(std::ranges::subrange(match.begin() + 8, match.begin() + 10, 2));
        host = f.host;
        port = f.port;
      } else if (match[10].matched) {
        path = "*";
      }
#  undef PERCENT_DECODE
    }

    auto tmp_version = line.substr(_2sp + 1);
    if (!std::regex_match(tmp_version.begin(), tmp_version.end(), regex::http_version_re))
      return {0, res};
    version = tmp_version;
    pos = end + 2;
    return {pos, {}};
  }
};

XSL_HTTP_NE
#endif  // XSL_NET_HTTP_REQUEST_LINE
