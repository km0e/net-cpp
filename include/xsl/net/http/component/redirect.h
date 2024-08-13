#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_REDIRECT
#  define XSL_NET_HTTP_COMPONENT_REDIRECT
#  include "xsl/ai/dev.h"
#  include "xsl/net/http/component/def.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/msg.h"

#  include <optional>

XSL_NET_HTTP_COMPONENT_NB
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
Handler<ByteReader, ByteWriter> create_redirect_handler(std::string_view path) {
  return [path](HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
    ResponsePart part{Status::MOVED_PERMANENTLY};
    part.headers.emplace("Location", std::string(path));
    ctx.resp(std::move(part));
    co_return std::nullopt;
  };
}
XSL_NET_HTTP_COMPONENT_NE
#endif
