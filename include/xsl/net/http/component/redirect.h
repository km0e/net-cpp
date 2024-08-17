#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_REDIRECT
#  define XSL_NET_HTTP_COMPONENT_REDIRECT
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"

#  include <optional>

XSL_HTTP_NB
template <ai::ABRL ByteReader, ai::ABWL ByteWriter>
Handler<ByteReader, ByteWriter> create_redirect_handler(std::string_view path) {
  return [path](HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
    ResponsePart part{Status::MOVED_PERMANENTLY};
    part.headers.emplace("Location", std::string(path));
    ctx.resp(std::move(part));
    co_return std::nullopt;
  };
}
XSL_HTTP_NE
#endif
