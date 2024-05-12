#pragma once
#ifndef _XSL_NET_HTTP_CONTEXT_H_
#  define _XSL_NET_HTTP_CONTEXT_H_
#  include "xsl/http/http.h"
#  include "xsl/http/msg.h"
#  include "xsl/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
class Context {
public:
  Context(Request&& request);
  ~Context();
  wheel::string_view current_path;
  Request request;
};

HTTP_NAMESPACE_END
#endif
