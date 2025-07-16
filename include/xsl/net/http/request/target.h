/**
 * @file target.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP request target definitions
 * @version 0.1
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_REQUEST_TARGET
#  define XSL_NET_HTTP_REQUEST_TARGET

#  include "xsl/net/http/def.h"
#  include "xsl/net/uri.h"

#  include <regex>

XSL_HTTP_NB
/**
 * @brief OriginForm represents the origin form of an HTTP request target.
 * @see https://datatracker.ietf.org/doc/html/rfc9112#section-3.2.1
 */
struct OriginForm {
  static constexpr std::string_view regex_str = R"(^((?:/[^/?#]*)*)(?:\?([^#]*))?)";  //  <2

  std::string_view path;
  std::string_view query;

  template <std::ranges::random_access_range Range>
    requires std::is_same_v<std::ranges::range_value_t<Range>, std::sub_match<const char*>>
  OriginForm(Range&& output)
      : path{output[0].first, output[0].second}, query{output[1].first, output[1].second} {}
};

/**
 * @brief AbsoluteForm represents the absolute form of an HTTP request target.
 * @see https://datatracker.ietf.org/doc/html/rfc9112#section-3.2.2
 */
using AbsoluteForm = AbsoluteUri;

/**
 * @brief AuthorityForm represents the authority form of an HTTP request target.
 * @see https://datatracker.ietf.org/doc/html/rfc9112#section-3.2.3
 */
struct AuthorityForm {
  static constexpr std::string_view regex_str = R"(^([^:*]*)(?::(\d*))?)";  ///< 2
  static const std::regex regex_re;
  /// FIX:with host implementation

  std::string_view host = {};  ///< authority host, may be empty
  std::string_view port = {};  ///< authority port, 0 means not specified
  AuthorityForm() = default;
  template <std::ranges::random_access_range Range>
    requires std::is_same_v<std::ranges::range_value_t<Range>, std::sub_match<const char*>>
  AuthorityForm(Range&& output)
      : host{output[0].first, output[0].second}, port{output[1].first, output[1].second} {}
  AuthorityForm(std::string_view sv)
      : AuthorityForm([&]() -> AuthorityForm {
          std::cmatch match;
          if (std::regex_match(sv.begin(), sv.end(), match, regex_re)) {
            return {std::ranges::subrange(match.begin() + 1, match.end())};
          }
          return {};
        }()) {}  ///< for convenience, use string_view
};

/**
 * @brief AsteriskForm represents the asterisk form of an HTTP request target.
 * @see https://datatracker.ietf.org/doc/html/rfc9112#section-3.2.4
 */
struct AsteriskForm {
  static constexpr std::string_view regex_str = R"(^(\*))";  /// <1
};

struct RequestTarget {
  static const std::regex regex_re;
  using TargetType
      = std::variant<std::monostate, OriginForm, AbsoluteForm, AuthorityForm, AsteriskForm>;

  TargetType target = std::monostate{};  ///< default to empty target

  RequestTarget(std::string_view target) {
    std::cmatch match;
    if (std::regex_match(target.begin(), target.end(), match, regex_re)) {
      if (match[1].matched) {
        this->target = OriginForm(std::ranges::subrange(match.begin() + 1, match.begin() + 3, 2));
      } else if (match[3].matched) {  /// absolute_form
        this->target = AbsoluteForm(std::ranges::subrange(match.begin() + 3, match.begin() + 8, 4));
      } else if (match[8].matched) {
        this->target
            = AuthorityForm(std::ranges::subrange(match.begin() + 8, match.begin() + 10, 2));
      } else if (match[10].matched) {
        this->target = AsteriskForm();
      }
    }
  }
#  define GET_METHOD(type, name)                        \
    type* name() {                                      \
      if (std::holds_alternative<type>(this->target)) { \
        return std::get_if<type>(&this->target);        \
      }                                                 \
      return nullptr;                                   \
    }
  GET_METHOD(OriginForm, origin_form)
  GET_METHOD(AbsoluteForm, absolute_form)
  GET_METHOD(AuthorityForm, authority_form)
  GET_METHOD(AsteriskForm, asterisk_form)
#  undef GET_METHOD
};

XSL_HTTP_NE
#endif
