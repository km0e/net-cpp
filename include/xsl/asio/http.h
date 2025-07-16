/**
 * @file http.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP server and client utilities
 * @version 0.1
 * @date 2025-06-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP
#  define XSL_ASIO_HTTP
#  include "xsl/asio/def.h"
#  include "xsl/asio/http/response.h"
#  include "xsl/asio/http/server.h"
#  include "xsl/asio/http/service.h"
#  include "xsl/asio/socket.h"
XSL_ASIO_NB
using namespace xsl::_asio::http;

template <class IOUtils>
struct HttpUtil : public IOUtils {
  consteval HttpUtil(IOUtils&& utils) : IOUtils(std::move(utils)) {}

  using io_dev_type = typename IOUtils::io_dev_type;

  template <class Poller, class... Args>
  constexpr auto make_creator(Poller& poller, Args&&... args) {
    return IOUtils::make_creator(poller, std::forward<Args>(args)...).transform([](auto&& creator) {
      return Server{std::forward<decltype(creator)>(creator)};
    });
  }

  template <RouterLike<std::size_t> R = Router>
  constexpr Service<io_dev_type, io_dev_type, R> make_service() {
    return {};
  }
};

Task<std::expected<std::tuple<std::unique_ptr<Response>, AsyncSocketCompose<TcpIp>>, errc>> get(
    Poller& poller, std::string_view url);
XSL_ASIO_NE
#endif
