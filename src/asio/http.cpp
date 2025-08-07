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
#include <xsl/asio/gai.h>
#include <xsl/asio/http.h>
#include <xsl/asio/http/request.h>
#include <xsl/asio/http/response.h>
#include <xsl/coro.h>
#include <xsl/io.h>
#include <xsl/net.h>

#include <expected>
#include <memory>
#include <tuple>

XSL_ASIO_NB

auto get(Context& ctx, std::string_view url)
    -> Task<std::expected<std::tuple<std::unique_ptr<Response>, AsyncSocketCompose<TcpIp>>, errc>> {
  RequestPartBuilder builder;
  auto uri = net::AbsoluteUri(url);
  if (uri.scheme.empty() || uri.host.empty()) {
    log_error("Invalid URL: {}", url);
    co_return std::unexpected(errc::invalid_argument);  // Return an empty builder
  }
  if (uri.scheme != "http" && uri.scheme != "https") {
    log_error("Unsupported scheme: {}", uri.scheme);
    co_return std::unexpected(errc::not_supported);  // Return an empty builder
  }
  builder.set_method(Method::GET);
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
  auto res = co_await gai_async_connect<TcpIp>(
      ctx, uri.host.data(),
      uri.port.empty() ? uri.scheme == "http" ? "80" : "443" : uri.port.data());
  if (!res) {
    log_error("Failed to connect: {}", res.error().message());
    co_return std::unexpected(errc::connection_refused);  // Return an empty builder
  }
  auto socket = std::move(res.value());
  errc ec = co_await builder.write(socket);
  if (ec != errc{}) {
    log_error("Failed to write request: {}", std::make_error_code(ec).message());
    co_return std::unexpected(ec);  // Return an empty builder
  }
  log_info("Request sent successfully, waiting for response...");
  auto resp = std::make_unique<Response>();
  ec = co_await resp->read(socket);
  if (ec != errc{}) {
    log_error("Failed to read response: {}", std::make_error_code(ec).message());
    co_return std::unexpected(ec);  // Return an empty builder
  }
  co_return {std::make_tuple(std::move(resp), std::move(socket))};
}
XSL_ASIO_NE
