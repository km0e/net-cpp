/**
 * @file http.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP server and client utilities
 * @version 0.1.0
 * @date 2025-06-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP
#  define XSL_ASIO_HTTP
#  include <xsl/asio/def.h>
#  include <xsl/asio/http/response.h>
#  include <xsl/asio/http/server.h>
#  include <xsl/asio/http/service.h>
#  include <xsl/asio/socket.h>
#  include <xsl/asio/tls.h>
XSL_ASIO_NB
using namespace xsl::_asio::http;

template <class IOUtils>
struct HttpUtil : public IOUtils {
  consteval HttpUtil(IOUtils&& utils) noexcept(std::is_nothrow_move_constructible_v<IOUtils>)
      : IOUtils(std::move(utils)) {}

  using io_dev_type = typename IOUtils::io_dev_type;

  template <class Ctx, class... Args>
  constexpr auto c_creator(const std::shared_ptr<Ctx>& ctx, Args&&... args) noexcept(
      noexcept(IOUtils::c_creator(ctx, std::forward<Args>(args)...))) {
    return IOUtils::c_creator(ctx, std::forward<Args>(args)...).transform([](auto&& creator) {
      return Server{std::forward<decltype(creator)>(creator)};
    });
  }

  template <RouterLike<_impl_service::Id> R = Router<_impl_service::Id>>
  constexpr ServiceBuilder<io_dev_type, io_dev_type, R> make_service2() {
    return {};
  }
};

class HttpClient {
public:
  constexpr HttpClient(const std::shared_ptr<Context>& ctx) : tls_ctx_(), ctx_(ctx) {}
  ~HttpClient() = default;

  void set_tls_context(TLSContext&& ctx) { this->tls_ctx_ = std::move(ctx); }
  Task<Expected<std::tuple<std::unique_ptr<Response>, std::shared_ptr<AsyncReadWriteBase>>>> get(
      std::string_view url);

private:
  TLSContext tls_ctx_;
  std::shared_ptr<Context> ctx_;
};

XSL_ASIO_NE
#endif
