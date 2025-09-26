/**
 * @file conn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Connection class for HTTP server
 * @version 0.2.2
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP_CONN
#  define XSL_ASIO_HTTP_CONN
#  include <xsl/asio/http/def.h>
#  include <xsl/asio/http/request.h>
#  include <xsl/asio/http/response.h>
#  include <xsl/coro.h>
#  include <xsl/coro/guard.h>
#  include <xsl/log.h>

#  include <memory>
#  include <type_traits>
#  include <utility>
XSL_ASIO_NB
using namespace xsl::io;
/**
 * @brief serve the connection
 *
 * @tparam AsyncRead the async reader
 * @tparam AsyncWrite the async writer
 * @tparam Service the service type
 * @param ard
 * @param awd
 * @param service the service
 * @return Task<void>
 * @note this function would not save the state of the arguments, do not directly call it
 */
template <AsyncRead R, AsyncWrite W, class Service>
Task<void> imm_serve_connection(R& ard, W& awd, Service& service) {
  Request req;
  while (true) {
    {
      log_debug("Start to read request");
      errc res = co_await req.read(ard);
      if (res == errc::not_connected) break;
      if (res != errc{}) {
        log_error("read request error: {}", std::make_error_code(res).message());
        break;
      }
    }
    log_debug("ready to serve request: {}", req.line.path);
    ResponseBuilder<W> resp = co_await service(req, ard);
    log_debug("ready to send response: {}", resp._part.status_code.to_reason_phrase());
    //@see https://datatracker.ietf.org/doc/html/rfc9112#name-tear-down
    bool is_close = req.get_header("Connection") == "close";
    if (is_close) {
      resp.set_header("Connection", "close");
    }
    IOResult res = co_await resp.sendto(awd);
    if (!res) {
      log_warning("send error: {}", res.message());
    }
    if (is_close) {
      log_debug("Connection closed by client");
      break;  // if the client requested to close the connection, break the loop
    }
  }
}

/**
 * @brief serve the connection
 *
 * @tparam ABIO the async byte reader and writer
 * @tparam Service the service type
 * @param _ab
 * @param service
 * @return Task<void>
 */
template <AsyncReadWrite RW, class Service>
decltype(auto) serve_connection(RW&& _ab, std::shared_ptr<Service>&& service) {
  using l_io_dev_type = std::decay_t<RW>;
  using l_in_dev_type = typename l_io_dev_type::in_dev_type;
  using l_out_dev_type = typename l_io_dev_type::out_dev_type;
  using r_in_dev_type = typename Service::in_dev_type;
  using r_out_dev_type = typename Service::out_dev_type;
  static_assert(std::is_same_v<l_in_dev_type, r_in_dev_type>,
                "Input device type mismatched with the service");
  static_assert(std::is_same_v<l_out_dev_type, r_out_dev_type>,
                "Output device type mismatched with the service");
  if constexpr (std::is_same_v<l_io_dev_type, r_in_dev_type>
                && std::is_same_v<l_io_dev_type, r_out_dev_type>) {
    return coro::ArgGuard(
        [](auto& _ab, auto& _service) { return imm_serve_connection(_ab, _ab, *_service); },
        std::move(_ab), std::move(service));
  } else if constexpr (requires { _ab.split(); }) {
    auto [r, w] = std::move(_ab).split();
    return serve_connection(std::move(r), std::move(w), std::move(service));
  }
}

template <AsyncReadWrite ABIO, class Service>
decltype(auto) serve_connection(ABIO&& _ab, std::shared_ptr<Service> service) {
  using io_dev_type = std::decay_t<ABIO>;
  using r_in_dev_type = typename Service::in_dev_type;
  using r_out_dev_type = typename Service::out_dev_type;
  static_assert(std::is_same_v<io_dev_type, r_in_dev_type>,
                "Input device type mismatched with the service");
  static_assert(std::is_same_v<io_dev_type, r_out_dev_type>,
                "Output device type mismatched with the service");
  if constexpr (std::is_same_v<io_dev_type, r_in_dev_type>
                && std::is_same_v<io_dev_type, r_out_dev_type>) {
    return coro::ArgGuard(
        [](auto& _ab, auto& _service) { return imm_serve_connection(_ab, _ab, *_service); },
        std::move(_ab), std::move(service));
  } else if constexpr (requires { std::move(*_ab).split(); }) {
    auto [r, w] = std::move(*_ab).split();
    return serve_connection(std::move(r), std::move(w), std::move(service));
  }
}

XSL_ASIO_NE
#endif
