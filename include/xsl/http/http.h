#pragma once
#ifndef _XSL_NET_HTTP_CONFIG_H_
#  define _XSL_NET_HTTP_CONFIG_H_
#  include "xsl/http/def.h"
#  include "xsl/http/helper/helper.h"
#  include "xsl/http/msg.h"
#  include "xsl/http/router.h"
#  include "xsl/http/server.h"
namespace xsl::http {
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
