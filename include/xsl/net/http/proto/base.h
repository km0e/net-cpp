#pragma once
#ifndef XSL_NET_HTTP_PROTO_BASE
#  define XSL_NET_HTTP_PROTO_BASE
#  include "xsl/net/http/proto/def.h"

#  include <regex>
#  include <vector>
XSL_NET_HTTP_PROTO_NB
using Token = std::string;
using TokenView = std::string_view;
static const std::regex token_regex("^[!#$%&'*+\\-.^_`|~0-9A-Za-z]+$");
struct ParameterView {
  TokenView name;
  TokenView value;
};
using Parameter = std::pair<Token, Token>;
using Parameters = std::vector<Parameter>;
using ParametersView = std::vector<ParameterView>;
// using Weight = double;
using WeightView = std::string_view;
XSL_NET_HTTP_PROTO_NE
#endif
