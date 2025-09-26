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
#ifndef XSL_ASIO_HTTP_COMPONENT_REDIRECT
#  define XSL_ASIO_HTTP_COMPONENT_REDIRECT
#  include <xsl/asio/http/context.h>
#  include <xsl/asio/http/def.h>
#  include <xsl/log.h>

#  include <optional>

XSL_ASIO_NB

template <AsyncRead ABI, AsyncWrite ABO>
constexpr Handler<ABI, ABO> create_redirect_handler(std::string_view path) {
  return [path](HandleContext<ABI, ABO>& ctx) -> HandleResult {
    log_debug("redirect to {}", path);
    ResponsePart part{Status::MOVED_PERMANENTLY};
    part.headers.emplace("Location", std::string(path));
    ctx.resp(std::move(part));
    co_return std::nullopt;
  };
}
XSL_ASIO_NE
#endif
