#pragma once
#ifndef XSL_NET_HTTP_HELPER
#  define XSL_NET_HTTP_HELPER
#  include "xsl/net/http/component/redirect.h"
#  include "xsl/net/http/component/static.h"
#  include "xsl/net/http/def.h"
XSL_HTTP_NB
using component::create_redirect_handler;
using component::create_static_handler;
using component::StaticFileConfig;
XSL_HTTP_NE
#endif  // XSL_NET_HTTP_HELPER
