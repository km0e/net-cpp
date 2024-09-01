/**
 * @file regex.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once
#ifndef XSL_REGEX
#  define XSL_REGEX
#  include <regex>
#  include <string_view>
namespace xsl::regex {
  const std::string_view authority = R"(^(?:([^@]*)@)?([^:]*)(?::(\d*))?)";  //<3
  const std::regex authority_re(authority.data(), authority.size());
  const std::string_view absolute_uri
      = R"(^([^:/?#]+)://([^/?#]*)((?:/[^/?#]*)*)(?:\?([^#]*))?)";  //<4
  const std::regex absolute_uri_re(absolute_uri.data(), absolute_uri.size());

  const std::string_view origin_form = R"(^((?:/[^/?#]*)*)(?:\?([^#]*))?)";  ///< 2
  const std::regex origin_form_re(origin_form.data(), origin_form.size());
  const std::string_view absolute_form = absolute_uri;  //
  const std::regex absolute_form_re(absolute_form.data(), absolute_form.size());
  const std::string_view authority_form = authority;  //
  const std::regex authority_form_re(authority_form.data(), authority_form.size());
  const std::string_view asterisk_form = R"(^(\*)?)";  //<1
  const std::regex asterisk_form_re(asterisk_form.data(), asterisk_form.size());

  const std::string_view http_version = R"(HTTP/(\d)\.(\d))";
  const std::regex http_version_re(http_version.data(), http_version.size());

  const std::regex parameter_re(R"(\s*;\s*([^=]+)=(?:\"?([^;,\r\n]+)\"?))");
}  // namespace xsl::regex
#endif
