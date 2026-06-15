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
#  include <xsl/asio/dev.h>
#  include <xsl/asio/io.h>
#  include <xsl/asio/net/socket.h>
#  include <xsl/coro.h>
#  include <xsl/io/def.h>
#  include <xsl/net.h>

#  include <concepts>

XSL_ASIO_NB

using xsl::net::SockAddr;

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
    err_msg.resize_and_overwrite(256, [ec](char* buf, size_t) {
      ERR_error_string_n(ec, buf, 256);
      return std::strlen(buf);
    });
    return StringError{ec, std::move(err_msg)};
  };
}

class TLSContext {
public:
  SSL_CTX* ctx_ = nullptr;

public:
  TLSContext() = default;
  TLSContext(SSL_CTX* ctx) : ctx_(ctx) {}
  TLSContext(const TLSContext&) = delete;
  TLSContext& operator=(const TLSContext&) = delete;
  TLSContext(TLSContext&& rhs) noexcept : ctx_(std::exchange(rhs.ctx_, nullptr)) {}
  TLSContext& operator=(TLSContext&& rhs) noexcept {
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

  constexpr Expected<SSL*> new_ssl() {
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
  SSL_CTX* ctx_ = nullptr;
  StringError se{0, ""};

public:
  TLSContextBuilder(TLSContextBuilder&& rhs) noexcept
      : ctx_(std::move(rhs.ctx_)), se(std::move(rhs.se)) {
    rhs.ctx_ = nullptr;
  }
  TLSContextBuilder(const TLSContextBuilder&) = delete;
  TLSContextBuilder& operator=(const TLSContextBuilder&) = delete;
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
                                           int (*callback)(int, x509_store_ctx_st*) = nullptr) {
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
  TLSContextBuilder(const SSL_METHOD* m)
      : ctx_(SSL_CTX_new(m)), se(ctx_ ? StringError{0, ""} : tls_err_gen()()) {}
  template <std::invocable Func>
  constexpr inline TLSContextBuilder& safe_chain(Func&& func) {
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
  Task<io::Result> read(this auto&& self, byte* data, std::size_t size) {
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
  Task<io::Result> write(this auto&& self, const byte* data, std::size_t size) {
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
};

template <class... Utils>
struct TLSStorage : Utils..., std::unique_ptr<SSL, decltype(&SSL_free)>, asio::TLSRx, asio::TlsTx {
  constexpr bool set_tls_ext_hostname(this auto&& self, const char* hostname) {
    return SSL_set_tlsext_host_name(self.ssl(), hostname) == 1;
  }
  constexpr bool set1_host(this auto&& self, const char* hostname) {
    return SSL_set1_host(self.ssl(), hostname) == 1;
  }
  constexpr Expected<void> connect(this auto&& self) {
    if (SSL_connect(self.ssl()) < 1) {
      auto res = SSL_get_verify_result(self.ssl());
      ENSURE(res == X509_V_OK, errc::permission_denied, X509_verify_cert_error_string(res));
    }
    return {};
  }
  SSL* ssl() { return std::unique_ptr<SSL, decltype(&SSL_free)>::get(); }
};

template <class... Utils>
using TLSLayer = shared_memory<LocalCompose<DirectAsyncReadWriteUtils, RawOwner,
                                            IOSignalStorage<IOM_EVENTS::IN, IOM_EVENTS::OUT>,
                                            TLSStorage<AsyncDeviceUtil, Utils...>>>;

template <class... Utils>
using DynTLSLayer
    = shared_memory<LocalCompose<DirectAsyncReadWriteUtils, AsyncReadWriteBase, RawOwner,
                                 IOSignalStorage<IOM_EVENTS::IN, IOM_EVENTS::OUT>,
                                 TLSStorage<AsyncDeviceUtil, Utils...>>>;
template <class... Flags>
using TLSAsyncSocket = TLSLayer<sys::net::SocketTraits<Flags...>>;

XSL_ASIO_NE
XSL_ASIO_NB

template <class SockTraits, long Mode>
struct TLSTraits : public SockTraits {
  using sock_traits_type = SockTraits;
};

struct TLSUtils {
  template <sys::net::SocketTraitsCompatible<sys::net::SocketTraits<TcpIp>> Traits>
  Task<Expected<TLSAsyncSocket<Traits>>> ca2(TLSContext& tls_ctx,
                                             const SockAddr<Traits>& addr) noexcept {
    TLSLayer<Traits> tls_sock;
    co_await AsyncSocketCreatorCompose<Traits>{}.a2(tls_sock, addr);
    CO_TRV(ssl, tls_ctx.new_ssl());
    CO_ENSURE(SSL_set_fd(ssl, tls_sock->raw()), tls_err_gen()());
    std::construct_at<std::unique_ptr<SSL, decltype(&SSL_free)>>(
        static_cast<std::unique_ptr<SSL, decltype(&SSL_free)>*>(tls_sock.get()), ssl, SSL_free);
    co_return tls_sock;
  }
  template <class... Flags, class... Args,
            sys::net::SocketTraitsCompatible<TcpIp> Traits = sys::net::SocketTraits<Flags...>>
    requires requires(Args&&... args) {
      sys::net::make_sockaddr<Traits>(std::forward<Args>(args)...);
    }
  Task<Expected<TLSAsyncSocket<Traits>>> ca2(TLSContext& tls_ctx, Args&&... args) noexcept {
    CO_TRV(addr, sys::net::make_sockaddr<Traits>(std::forward<Args>(args)...));
    TLSLayer<Traits> tls_sock;
    co_await AsyncSocketCreatorCompose<Traits>{}.a2(tls_sock, addr);
    CO_TRV(ssl, tls_ctx.new_ssl());
    CO_ENSURE(SSL_set_fd(ssl, tls_sock->raw()), tls_err_gen()());
    std::construct_at<std::unique_ptr<SSL, decltype(&SSL_free)>>(
        static_cast<std::unique_ptr<SSL, decltype(&SSL_free)>*>(tls_sock.get()), ssl, SSL_free);
    co_return tls_sock;
  }
  template <class SockTraits>
  static decltype(auto) ca2(TLSContext& tls_ctx, sys::net::AddrInfos<SockTraits>& ais) {
    return ac2_impl<TLSLayer<SockTraits>>(ais, tls_ctx);
  }
  template <class SockTraits>
  static decltype(auto) ca2_dyn(TLSContext& tls_ctx, sys::net::AddrInfos<SockTraits>& ais) {
    return ac2_impl<DynTLSLayer<SockTraits>>(ais, tls_ctx);
  }

private:
  template <class TLSSocket, class SockTraits>
  static Task<Expected<TLSSocket>> ac2_impl(sys::net::AddrInfos<SockTraits>& ais,
                                            TLSContext& tls_ctx) {
    TLSSocket tls_sock;
    errc ec = {};
    for (addrinfo& ai : ais) {
      CONTV(sock, sys::net::socket<SockTraits>(ai));
      auto res = co_await asio::async_connect(tls_sock, std::move(sock).into_raw(), ai.ai_addr,
                                               ai.ai_addrlen);
      if (res) {
        ec = errc{};
        break;
      }
      ec = res.error();
    }
    CO_ENSURE(ec, "Failed to connect to any address");
    CO_TRV(ssl, tls_ctx.new_ssl());
    CO_ENSURE(SSL_set_fd(ssl, tls_sock->raw()), tls_err_gen()());
    std::construct_at<std::unique_ptr<SSL, decltype(&SSL_free)>>(
        static_cast<std::unique_ptr<SSL, decltype(&SSL_free)>*>(tls_sock.get()), ssl, SSL_free);
    co_return tls_sock;
  }
};

XSL_ASIO_NE
#endif
