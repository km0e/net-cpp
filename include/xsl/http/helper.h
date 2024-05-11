#pragma once
#ifndef _XSL_NET_HTTP_HELPER_H_
#  define _XSL_NET_HTTP_HELPER_H_
#  include "xsl/http/context.h"
#  include "xsl/http/http.h"
#  include "xsl/http/msg.h"
#  include "xsl/utils/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
class StaticRouteHandler {
public:
  // path format: /path/to/file or /path/to/dir/
  StaticRouteHandler(wheel::string&& path);
  ~StaticRouteHandler();
  Response operator()(Context& request);

private:
  wheel::string path;
};
HTTP_NAMESPACE_END
#endif  // _XSL_NET_HTTP_HELPER_H_
