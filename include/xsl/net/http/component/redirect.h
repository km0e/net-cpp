/**
 * @file redirect.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Redirect component
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_REDIRECT
#  define XSL_NET_HTTP_COMPONENT_REDIRECT
#  include "xsl/logctl.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"

#  include <optional>

XSL_HTTP_NB
using namespace xsl::io;

template <ABRL ByteReader, ABWL ByteWriter>
Handler<ByteReader, ByteWriter> create_redirect_handler(std::string_view path) {
  return [path](HandleContext<ByteReader, ByteWriter>& ctx) -> HandleResult {
    DEBUG("redirect to {}", path);
    ResponsePart part{Status::MOVED_PERMANENTLY};
    part.headers.emplace("Location", std::string(path));
    ctx.resp(std::move(part));
    co_return std::nullopt;
  };
}
XSL_HTTP_NE
#endif
