#pragma once
#ifndef _XSL_NET_HTTP_HELPER_H_
#  define _XSL_NET_HTTP_HELPER_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/helper/static.h"
HTTP_NAMESPACE_BEGIN
using helper::detail::StaticCreateResult;
using helper::detail::create_static_handler;
HTTP_NAMESPACE_END
#endif  // _XSL_NET_HTTP_HELPER_H_
