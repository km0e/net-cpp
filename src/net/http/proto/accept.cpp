/**
 * @file accept.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/net/http/proto/accept.h"
#include "xsl/net/http/proto/base.h"
#include "xsl/regex.h"

#include <ranges>
#include <regex>
XSL_HTTP_NB
AcceptView parse_accept(std::string_view accept) {
  AcceptView result;
  while (true) {
    auto slash = accept.find('/');
    if (slash == std::string_view::npos) {
      break;
    }
    MediaTypeView media{};
    WeightView weight{};
    media.start = accept.data();
    media.slash = accept.data() + slash;
    auto comma = accept.find(',', slash);
    if (comma == std::string_view::npos) {
      comma = accept.size();
    }
    auto media_str = accept.substr(0, comma);
    auto semicolon = media_str.find(';', slash);
    if (semicolon == std::string_view::npos) {
      media.end = media_str.data() + comma;
    } else {
      media.end = media_str.data() + semicolon;
      auto begin = std::cregex_iterator(media_str.data() + semicolon, media_str.data() + comma,
                                        regex::parameter_re);
      auto end = std::cregex_iterator();
      for (auto& match : std::ranges::subrange{begin, end}) {
        auto name = std::string_view{match[1].first, match[1].second};
        if (name == "q") {
          weight = std::string_view{match[2].first, match[2].second};
          continue;
        }
        media.parameters.emplace_back(name, std::string_view{match[2].first, match[2].second});
      }
    }
    result.emplace_back(media, weight);
    if (comma == accept.size()) {
      break;
    }
    auto next = accept.find_first_not_of(" \t", comma + 1);
    accept = accept.substr(next);
  }
  return result;
}

AcceptEncodingView parse_accept_encoding(std::string_view accept_encoding) {
  if (accept_encoding.empty()) {
    return {};
  }
  AcceptEncodingView result;
  while (true) {
    auto comma = accept_encoding.find(',');
    if (comma == std::string_view::npos) {
      comma = accept_encoding.size();
    }
    auto encoding_str = accept_encoding.substr(0, comma);
    auto semicolon = encoding_str.find(';');
    TokenView encoding{};
    WeightView weight{};
    if (semicolon == std::string_view::npos) {
      encoding = encoding_str;
    } else {
      encoding = encoding_str.substr(0, semicolon);
      std::cmatch m;
      if (std::regex_search(encoding_str.data() + semicolon, encoding_str.data() + comma, m,
                            regex::parameter_re)) {
        weight = std::string_view{m[2].first, m[2].second};
      }
    }
    result.emplace_back(encoding, weight);
    if (comma == accept_encoding.size()) {
      break;
    }
    auto next = accept_encoding.find_first_not_of(" \t", comma + 1);
    accept_encoding = accept_encoding.substr(next);
  }
  return result;
}

XSL_HTTP_NE
