#pragma once
#ifndef XSL_NET_HTTP_PROTO_ACCEPT
#  define XSL_NET_HTTP_PROTO_ACCEPT
#  include "xsl/net/http/proto/base.h"
#  include "xsl/net/http/proto/def.h"
#  include "xsl/net/http/proto/media-type.h"

#  include <vector>
XSL_NET_HTTP_PROTO_NB
// using Accept = std::vector<std::pair<MediaType, Weight>>;
using AcceptView = std::vector<std::pair<MediaTypeView, WeightView>>;

AcceptView parse_accept(std::string_view accept);

using AcceptEncodingView = std::vector<std::pair<TokenView, WeightView>>;

AcceptEncodingView parse_accept_encoding(std::string_view accept_encoding);
XSL_NET_HTTP_PROTO_NE
#endif
