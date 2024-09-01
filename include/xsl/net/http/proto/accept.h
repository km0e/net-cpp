/**
 * @file accept.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_PROTO_ACCEPT
#  define XSL_NET_HTTP_PROTO_ACCEPT
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/proto/base.h"
#  include "xsl/net/http/proto/media-type.h"

#  include <vector>
XSL_HTTP_NB
// using Accept = std::vector<std::pair<MediaType, Weight>>;
using AcceptView = std::vector<std::pair<MediaTypeView, WeightView>>;

AcceptView parse_accept(std::string_view accept);

using AcceptEncodingView = std::vector<std::pair<TokenView, WeightView>>;

AcceptEncodingView parse_accept_encoding(std::string_view accept_encoding);
XSL_HTTP_NE
#endif
