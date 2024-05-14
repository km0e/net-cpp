#pragma once
#ifndef _XSL_NET_HTTP_CONFIG_H_
#  define _XSL_NET_HTTP_CONFIG_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/helper/helper.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/router.h"
#  include "xsl/net/http/server.h"
namespace xsl::net::http {
  using detail::DefaultHandlerGenerator;
  using detail::DefaultResponse;
  using detail::DefaultRouter;
  using detail::DefaultServer;
  using detail::DefaultServerConfig;
  using detail::HttpMethod;
  using detail::HttpVersion;
  using detail::Request;
  using detail::ResponsePart;
  using detail::RouteErrorKind;
  using helper::create_static_handler;
  using helper::StaticCreateResult;
}  // namespace xsl::http
#endif
