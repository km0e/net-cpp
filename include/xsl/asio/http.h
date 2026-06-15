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
#  include <xsl/asio/tls.h>
XSL_ASIO_NB

template <class IOUtils>
struct HttpUtil : public IOUtils {
  consteval HttpUtil(IOUtils&& utils) noexcept(std::is_nothrow_move_constructible_v<IOUtils>)
      : IOUtils(std::move(utils)) {}

  using io_dev_type = typename IOUtils::io_dev_type;

  template <class... Args>
  constexpr auto cl(sys::IOContext& ctx,
                    Args&&... args) noexcept(noexcept(IOUtils::cl(ctx,
                                                                  std::forward<Args>(args)...))) {
    return IOUtils::cl(ctx, std::forward<Args>(args)...).transform([](auto&& creator) {
      return HttpServer{std::forward<decltype(creator)>(creator)};
    });
  }

  template <RouterLike<_detail::Id> R = Router<_detail::Id>>
  constexpr ServiceBuilder<io_dev_type, io_dev_type, R> make_service2() {
    return {};
  }
};

class HttpClient {
public:
  constexpr HttpClient() : tls_ctx_() {}
  ~HttpClient() = default;

  void set_tls_context(TLSContext&& ctx) { this->tls_ctx_ = std::move(ctx); }
  Task<Expected<std::tuple<std::unique_ptr<Response>, shared_memory<AsyncReadWriteBase>>>> get(
      std::string_view url);

private:
  TLSContext tls_ctx_;
};

XSL_ASIO_NE
#endif
