#pragma once
#ifndef XSL_NET_HTTP_PROTO_BASE
#  define XSL_NET_HTTP_PROTO_BASE
#  include "xsl/net/http/def.h"

#  include <string>
#  include <string_view>
#  include <vector>
XSL_HTTP_NB
using Token = std::string;
struct Parameter {
  Token name;
  Token value;
};
using TokenView = std::string_view;
struct ParameterView {
  TokenView name;
  TokenView value;
};
using Parameters = std::vector<Parameter>;
using ParametersView = std::vector<ParameterView>;
// using Weight = double;
using WeightView = std::string_view;
XSL_HTTP_NE
#endif
