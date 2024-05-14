#pragma once
#ifndef _XSL_NET_HTTP_CONTEXT_H_
#  define _XSL_NET_HTTP_CONTEXT_H_
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
class Context {
public:
  Context(Request&& request);
  ~Context();
  wheel::string_view current_path;
  Request request;
  bool is_ok;
};

HTTP_NAMESPACE_END
#endif
