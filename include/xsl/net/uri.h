/**
 * @file uri.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief URI handling
 * @version 0.1
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_NET_URI
#  define XSL_NET_URI
#  include "xsl/net/def.h"

#  include <ranges>
#  include <regex>
#  include <string_view>
#  include <unordered_map>

XSL_NET_NB

struct KVQuery {
  static constexpr std::string_view regex_str = R"(([^&=]+)=([^&=]+))";
  static const std::regex regex_re;
  std::unordered_map<std::string_view, std::string_view> map;
  KVQuery(std::string_view sv) : map{} {
    std::cregex_iterator query_start(sv.begin(), sv.end(), KVQuery::regex_re);
    std::cregex_iterator query_end;
    while (query_start != query_end) {
      map[std::string_view((*query_start)[1].first, (*query_start)[1].length())]
          = std::string_view((*query_start)[2].first, (*query_start)[2].length());
      ++query_start;
    }
  }
};

/**
 * @class AbsoluteUri
 * @brief Represents an absolute URI as defined in RFC 3986.
 * @see https://datatracker.ietf.org/doc/html/rfc3986#section-4.3
 *
 */
struct AbsoluteUri {
  static constexpr std::string_view regex_str
      = R"(^([^:/?#]+)://(?:(?:[^/?#@]*@)?([^/?#:]*)(?::([^/?#]*))?)((?:/[^/?#]*)*)(?:\?([^#]*))?)";  /// <4
  static const std::regex regex_re;
  std::string_view scheme = {};  ///< URI scheme, e.g., "http", "https"
  std::string_view host = {};    ///< authority host, may be empty
  std::string_view port = {};    ///< authority port, 0 means not specified
  std::string_view path = {};    ///< path component, may be empty
  std::string_view query = {};   ///< query component, may be empty

  AbsoluteUri() = default;
  AbsoluteUri(AbsoluteUri&&) = default;
  template <std::ranges::random_access_range Range>
    requires std::is_same_v<std::ranges::range_value_t<Range>, std::sub_match<const char*>>
  AbsoluteUri(Range&& output)
      : scheme{output[0].first, output[0].second},
        host{output[1].first, output[1].second},
        port{output[2].first, output[2].second},
        path{output[3].first, output[3].second},
        query{output[4].first, output[4].second} {}
  AbsoluteUri(std::string_view uri)
      : AbsoluteUri([&]() -> AbsoluteUri {
          std::cmatch output;
          if (std::regex_match(uri.begin(), uri.end(), output, AbsoluteUri::regex_re)) {
            return {std::ranges::subrange(output.begin() + 1, output.end())};
          }
          return {};
        }()) {}
  AbsoluteUri& operator=(AbsoluteUri&&) = default;

  std::string_view origin_form() const {
    return std::string_view{path.data(), path.size() + (query.empty() ? 0 : query.size() + 1)};
  }
};

XSL_NET_NE

#endif
