#pragma once
#ifndef XSL_REGEX
#  define XSL_REGEX
#  include <regex>
#  include <string_view>
namespace xsl::regex {
  const std::string_view authority = R"(^(?:([^@]*)@)?([^:]*)(?::(\d*))?)";  //<3
  const std::regex authority_re(authority.data());
  const std::string_view absolute_uri
      = R"(^([^:/?#]+)://([^/?#]*)((?:/[^/?#]*)*)(?:\?([^#]*))?)";  //<4
  const std::regex absolute_uri_re(absolute_uri.data());

  const std::string_view origin_form = R"(^((?:/[^/?#]*)*)(?:\?([^#]*))?)";  ///< 2
  const std::regex origin_form_re(origin_form.data());
  const std::string_view absolute_form = absolute_uri;  //
  const std::regex absolute_form_re(absolute_form.data());
  const std::string_view authority_form = authority;  //
  const std::regex authority_form_re(authority_form.data());
  const std::string_view asterisk_form = R"(^(\*)?)";  //<1
  const std::regex asterisk_form_re(asterisk_form.data());

  const std::regex parameter_re(R"(\s*;\s*([^=]+)=(?:\"?([^;,\r\n]+)\"?))");
}  // namespace xsl::regex
#endif
