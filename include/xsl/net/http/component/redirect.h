#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_REDIRECT
#  define XSL_NET_HTTP_COMPONENT_REDIRECT
#  include "xsl/ai/dev.h"
#  include "xsl/net/http/component/def.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/msg.h"

HTTP_HELPER_NB
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
RouteHandler<ByteReader, ByteWriter> create_redirect_handler(std::string_view path) {
  return [path](HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
    ResponsePart part{Status::MOVED_PERMANENTLY};
    part.headers.emplace("Location", std::string(path));
    ctx.resp(std::move(part));
    return []() -> HandleResult { co_return; }();
  };
}
HTTP_HELPER_NE
#endif
