/**
 * @file http.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2025-06-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "xsl/log.h"

#include <xsl/asio.h>
#include <xsl/asio/http.h>
#include <xsl/asio/http/request.h>
#include <xsl/asio/http/response.h>
#include <xsl/asio/socket.h>
#include <xsl/asio/tls.h>
#include <xsl/coro.h>
#include <xsl/io.h>
#include <xsl/net.h>

#include <memory>
#include <tuple>
#include <utility>

XSL_ASIO_NB

auto HttpClient::get(std::string_view url)
    -> Task<Expected<std::tuple<std::unique_ptr<Response>, std::shared_ptr<AsyncReadWriteBase>>>> {
  RequestPartBuilder builder;
  auto _uri = net::AbsoluteUri::match(url);
  CO_ENSURE(_uri && !_uri->scheme.empty() && !_uri->host.empty(), errc::invalid_argument,
            "Invalid URL");
  auto uri = std::move(*_uri);
  CO_ENSURE(uri.scheme == "http" || uri.scheme == "https", errc::not_supported,
            "Only http and https are supported");
  builder.set_method(Method::GET);
  log_info("Requesting URL: {}", url);
  if (uri.path.empty()) {
    builder.append(' ');
    builder.append('/');
    builder.append(uri.origin_form());
  } else {
    builder.set_target(uri.origin_form());
  }
  builder.set_version(http::Version::HTTP_1_1);
  builder.add_header("Host", uri.host);
  builder.add_header("User-Agent", API_VERSION);
  builder.add_header("Accept", "*/*");
  if (uri.scheme == "https") {
    CO_TRV(ais, sys::net::getaddrinfo<TcpIp>(uri.host.data(),
                                             uri.port.empty() ? "443" : uri.port.data()));
    CO_TRV(tls_sock, co_await TlsUtils{}.ac2_dyn(*ctx_, tls_ctx_, ais));
    CO_ENSURE(tls_sock->set_tls_ext_hostname(uri.host.data()), errc::invalid_argument,
              "Failed to set SNI hostname");
    CO_ENSURE(tls_sock->set1_host(uri.host.data()), errc::invalid_argument,
              "Failed to set hostname for verification");
    CO_ENSURE(tls_sock->connect());
    errc ec = co_await builder.write(tls_sock);
    CO_ENSURE(ec == errc{}, ec, "Failed to write request");
    log_info("Request sent successfully, waiting for response...");
    auto resp = std::make_unique<Response>();
    ec = co_await resp->read(tls_sock);
    CO_ENSURE(ec == errc{}, ec, "Failed to read response");
    co_return {std::make_tuple(std::move(resp), std::move(tls_sock).into_dyn())};
  } else if (uri.scheme == "http") {
    log_info("Resolving {}:{}", uri.host, uri.port.empty() ? "80" : uri.port);
    CO_TRV(ais, sys::net::getaddrinfo<TcpIp>(uri.host.data(),
                                             uri.port.empty() ? "80" : uri.port.data()));
    log_debug("Resolved {} to addresses", uri.host);
    CO_TRV(socket, co_await asio::make_async_socket_utils<TcpIp>().ac2_dyn(*ctx_, ais));
    log_debug("Connected to {}:{}", uri.host, uri.port.empty() ? "80" : uri.port);
    errc ec = co_await builder.write(socket);
    CO_ENSURE(ec == errc{}, ec, "Failed to write request");
    log_info("Request sent successfully, waiting for response...");
    auto resp = std::make_unique<Response>();
    ec = co_await resp->read(socket);
    CO_ENSURE(ec == errc{}, ec, "Failed to read response");
    co_return {std::make_tuple(std::move(resp), std::move(socket).into_dyn())};
  }
  std::unreachable();
}

XSL_ASIO_NE
