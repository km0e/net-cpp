/**
 * @file tls.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-06-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_ASIO_TLS
#  define XSL_ASIO_TLS
#  include <openssl/err.h>
#  include <openssl/ssl.h>
#  include <openssl/ssl3.h>
#  include <xsl/asio/def.h>
#  include <xsl/asio/socket.h>
#  include <xsl/coro.h>
#  include <xsl/io/def.h>
#  include <xsl/net.h>

#  include <concepts>

XSL_ASIO_NB

enum class TLSVersion : long {
  TLS1_0 = TLS1_VERSION,
  TLS1_1 = TLS1_1_VERSION,
  TLS1_2 = TLS1_2_VERSION,
  TLS1_3 = TLS1_3_VERSION,
  DTLS1 = DTLS1_VERSION,
  DTLS1_2 = DTLS1_2_VERSION,
};

enum class TLSMode : long {
  ENABLE_PARTIAL_WRITE = SSL_MODE_ENABLE_PARTIAL_WRITE,
};

inline auto tls_err_gen() {
  return []() {
    auto ec = ERR_get_error();
    std::string err_msg;
    err_msg.resize_and_overwrite(256, [ec](char *buf, size_t) {
      ERR_error_string_n(ec, buf, 256);
      return std::strlen(buf);
    });
    return StringError{ec, std::move(err_msg)};
  };
}

class TLSContext {
public:
  SSL_CTX *ctx_ = nullptr;

public:
  TLSContext() = default;
  TLSContext(SSL_CTX *ctx) : ctx_(ctx) {}
  TLSContext(const TLSContext &) = delete;
  TLSContext &operator=(const TLSContext &) = delete;
  TLSContext(TLSContext &&rhs) noexcept : ctx_(std::exchange(rhs.ctx_, nullptr)) {}
  TLSContext &operator=(TLSContext &&rhs) noexcept {
    if (this != &rhs) {
      this->~TLSContext();
      ctx_ = std::exchange(rhs.ctx_, nullptr);
    }
    return *this;
  }
  ~TLSContext() {
    if (ctx_ != nullptr) {
      SSL_CTX_free(ctx_);
    }
  }

  constexpr Expected<SSL *> new_ssl() {
    TRV(ssl, SSL_new(ctx_), tls_err_gen()());
    return ssl;
  }

  constexpr bool is_valid() const { return ctx_ != nullptr; }
};

/**
 * @class TLSContextBuilder
 * @brief A builder for creating and configuring an SSL_CTX object.
 * @note General usage:
 * ```cpp
 * auto ctx = TLSContextBuilder::client()
 *             .set_verify_mode()
 *             .default_verify_paths()
 *             .set_min_version()
 *             .build();
 * ```
 *
 */
class TLSContextBuilder {
public:
  SSL_CTX *ctx_ = nullptr;
  StringError se{0, ""};

public:
  TLSContextBuilder(TLSContextBuilder &&rhs) noexcept
      : ctx_(std::move(rhs.ctx_)), se(std::move(rhs.se)) {
    rhs.ctx_ = nullptr;
  }
  TLSContextBuilder(const TLSContextBuilder &) = delete;
  TLSContextBuilder &operator=(const TLSContextBuilder &) = delete;
  ~TLSContextBuilder() {
    if (ctx_) {
      SSL_CTX_free(ctx_);
    }
  }
  static TLSContextBuilder client() {
    TLSContextBuilder context(TLS_client_method());
    if (!context) return context;
    return context;
  }

  constexpr operator bool() const { return se.code() == 0 && ctx_ != nullptr; }

  constexpr decltype(auto) set_verify_mode(int mode = SSL_VERIFY_PEER,
                                           int (*callback)(int, x509_store_ctx_st *) = nullptr) {
    return safe_chain([&]() {
      SSL_CTX_set_verify(ctx_, mode, callback);
      return true;
    });
  }
  constexpr decltype(auto) default_verify_paths() {
    return safe_chain([&]() { return SSL_CTX_set_default_verify_paths(ctx_) == 1; });
  }

  constexpr decltype(auto) set_min_version(TLSVersion version = TLSVersion::TLS1_2) {
    return safe_chain(
        [&]() { return SSL_CTX_set_min_proto_version(ctx_, static_cast<long>(version)) == 1; });
  }
  constexpr decltype(auto) add_mode(TLSMode mode) {
    return safe_chain([&]() {
      SSL_CTX_set_mode(ctx_, static_cast<long>(mode));
      return true;
    });
  }

  constexpr Expected<TLSContext> build() {
    ENSURE(*this, std::move(se));
    return TLSContext{std::exchange(ctx_, nullptr)};
  }

private:
  TLSContextBuilder(const SSL_METHOD *m)
      : ctx_(SSL_CTX_new(m)), se(ctx_ ? StringError{0, ""} : tls_err_gen()()) {}
  template <std::invocable Func>
  constexpr inline TLSContextBuilder &safe_chain(Func &&func) {
    if (!*this) return *this;
    if constexpr (std::is_same_v<std::invoke_result_t<Func>, bool>) {
      if (!func()) {
        se = tls_err_gen()();
      }
    } else {
      func();
    }
    return *this;
  }
};

struct TLSRx {
  Task<io::Result> read(this auto &&self, byte *data, std::size_t size) {
    do {
      std::size_t read = 0;
      int ret = SSL_read_ex(self.ssl(), data, size, &read);
      if (ret == 1) {
        co_return io::Result{read};
      }
      auto err = SSL_get_error(self.ssl(), ret);
      log_debug("SSL_read_ex error: {}", err);
      if (err == SSL_ERROR_WANT_READ) {
        if (co_await self.read_signal()) {
          continue;
        } else {
          co_return io::Result{0, errc::operation_canceled};
        }
      } else if (err == SSL_ERROR_WANT_WRITE) {
        if (co_await self.write_signal()) {
          continue;
        } else {
          co_return io::Result{0, errc::operation_canceled};
        }
      }
      co_return io::Result{read, errc::io_error};
    } while (true);
  }
};

struct TlsTx {
  /// @brief Send data to a device
  Task<io::Result> write(this auto &&self, const byte *data, std::size_t size) {
    do {
      std::size_t written = 0;
      int ret = SSL_write_ex(self.ssl(), data, size, &written);
      if (ret == 1) {
        co_return io::Result{written};
      }
      auto err = SSL_get_error(self.ssl(), ret);
      if (err == SSL_ERROR_WANT_READ) {
        if (co_await self.read_signal()) {
          continue;
        } else {
          co_return io::Result{0, errc::operation_canceled};
        }
      } else if (err == SSL_ERROR_WANT_WRITE) {
        if (co_await self.write_signal()) {
          continue;
        } else {
          co_return io::Result{0, errc::operation_canceled};
        }
      } else if (err == SSL_ERROR_ZERO_RETURN) {
        co_return io::Result{0, errc::not_connected};
      }
      co_return io::Result{0, errc::io_error};
    } while (true);
  }

  /// @brief write file to device
  // Task<io::Result> write_file(this auto &&self, WriteFileHint &&hint) {
  //   return send_file(self.raw(), std::move(hint), self.write_signal());
  // }
};

template <class... Utils>
struct TLSUtil : Utils..., TLSRx, TlsTx {};

template <class... Utils>
struct TLSStorage {};

template <class... Utils>
using TLSSocket
    = SharedStorageCompose<RawOwner, BaseOn<DirectAsyncReadWriteUtils>,
                           IOSignalStorage<IOM_EVENTS::IN, IOM_EVENTS::OUT>, TLSStorage<Utils...>>;
template <class... Utils>
using DynTLSSocket
    = SharedStorageCompose<Wrapper<AsyncReadWriteWrapper>,
                           BaseOn<DirectAsyncReadWriteUtils, SharedDynamicUtil<AsyncReadWriteBase>>,
                           RawOwner, IOSignalStorage<IOM_EVENTS::IN, IOM_EVENTS::OUT>,
                           TLSStorage<Utils...>>;

XSL_ASIO_NE
XSL_NB
template <class... Utils>
struct StorageUtils<_asio::TLSStorage<Utils...>>
    : Utils..., std::unique_ptr<SSL, decltype(&SSL_free)>, _asio::TLSRx, _asio::TlsTx {
  StorageUtils(SSL *ssl) : unique_ptr(ssl, SSL_free) {}
  constexpr bool set_tls_ext_hostname(this auto &&self, const char *hostname) {
    return SSL_set_tlsext_host_name(self.ssl(), hostname) == 1;
  }

  constexpr bool set1_host(this auto &&self, const char *hostname) {
    return SSL_set1_host(self.ssl(), hostname) == 1;
  }

  constexpr Expected<void> connect(this auto &&self) {
    if (SSL_connect(self.ssl()) < 1) {
      auto res = SSL_get_verify_result(self.ssl());
      ENSURE(res == X509_V_OK, errc::permission_denied, X509_verify_cert_error_string(res));
    }
    return {};
  }

  constexpr auto ssl(this auto &&self) { return self.get(); }
};
XSL_NE
XSL_ASIO_NB

template <class SockTraits, long Mode>
struct TLSTraits : public SockTraits {
  using sock_traits_type = SockTraits;
};

struct TlsUtils {
  template <class SockTraits>
  static Task<Expected<TLSSocket<>>> ac2(io::Context &ctx, TLSContext &context,
                                         sys::net::AddrInfos<SockTraits> &ais) {
    return ac2_impl<TLSSocket<>>(ctx, context, ais);
  }
  template <class SockTraits>
  static Task<Expected<DynTLSSocket<AsyncDeviceUtil>>> ac2_dyn(
      io::Context &ctx, TLSContext &context, sys::net::AddrInfos<SockTraits> &ais) {
    return ac2_impl<DynTLSSocket<AsyncDeviceUtil>>(ctx, context, ais);
  }

private:
  template <class TLSSocket, class SockTraits>
  static Task<Expected<TLSSocket>> ac2_impl(io::Context &ctx, TLSContext &context,
                                            sys::net::AddrInfos<SockTraits> &ais) {
    TLSSocket tls_sock;
    std::construct_at<IOSignalStorage<IOM_EVENTS::IN, IOM_EVENTS::OUT>>(tls_sock.get());
    errc ec = {};
    for (addrinfo &ai : ais) {
      CONTV(sock, sys::net::socket<SockTraits>(ai));
      auto res = co_await _asio::async_connect(
          sock.raw(), ai.ai_addr, ai.ai_addrlen,
          [&] -> Expected<decltype(&tls_sock->write_signal()), errc> {
            std::construct_at<RawOwner>(tls_sock.get(), std::move(sock).into_raw());
            add_to_context(tls_sock, ctx,
                           typename sys::net::SocketTraits<SockTraits>::poll_traits_type{});
            return {&tls_sock->write_signal()};
          });
      if (res) {
        ec = errc{};
        break;
      }
      ec = res.error();
    }
    CO_ENSURE(ec == errc{}, ec);
    CO_TRV(ssl, context.new_ssl());
    CO_ENSURE(SSL_set_fd(ssl, tls_sock->raw()), tls_err_gen()());
    std::construct_at<StorageUtils<TLSStorage<AsyncDeviceUtil>>>(tls_sock.get(), ssl);
    co_return tls_sock;
  }
};

XSL_ASIO_NE
#endif
