/**
 * @file msg.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP message rest view
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_NET_HTTP_MSG
#  define XSL_NET_HTTP_MSG
#  include "xsl/def.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"

#  include <unordered_map>
XSL_HTTP_NB
const int DEFAULT_HEADER_COUNT = 16;

const std::size_t HTTP_BUFFER_BLOCK_SIZE = 4096;

struct MessageRestView {
  std::unordered_map<std::string_view, std::string_view> headers
      = {};                            /// the headers of the message
  std::string_view content_part = {};  /// the content part of the message

  constexpr void clear() { headers.clear(); }

  constexpr std::tuple<size_t, errc> parse(std::string_view view) {
    errc res = errc::illegal_byte_sequence;
    size_t pos = 0;
    while (pos < view.size()) {
      size_t end = view.find("\r\n", pos);
      if (end == std::string_view::npos) {
        return {pos, errc::resource_unavailable_try_again};
      } else if (end == pos) {
        pos += 2;
        content_part = view.substr(pos);
        return {pos, {}};
      }
      size_t colon = view.find(':', pos);
      if (colon == std::string_view::npos) {
        break;
      }
      auto key = view.substr(pos, colon - pos);
      size_t vstart = view.find_first_not_of(' ', colon + 1);
      size_t vend = view.find("\r\n", vstart);
      if (vend == std::string_view::npos) {
        break;
      }
      auto value = view.substr(vstart, vend - vstart);
      headers[key] = value;
      log_trace("header: {}={}", key, value);
      pos = vend + 2;
    }
    return {pos, res};
  }
};

XSL_HTTP_NE
#endif  // XSL_HTTP_MSG
