/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_ASIO_DEF
#  define XSL_ASIO_DEF

#  define XSL_ASIO_NB \
    XSL_NB            \
    namespace _asio {
#  define XSL_ASIO_NE \
    XSL_NE            \
    }  // namespace _asio
#  include <xsl/byte.h>
#  include <xsl/concept.h>
#  include <xsl/coro.h>
#  include <xsl/def.h>
#  include <xsl/io.h>
#  include <xsl/io/def.h>
#  include <xsl/type_traits.h>

#  include <concepts>

XSL_ASIO_NB

using io::Result;

/**
 * @brief Define the abstract async read device
 */
template <class Device>
concept AsyncRead = requires(Device t, xsl::byte* data, std::size_t size) {
  { t.read(data, size) } -> coro::Awaitable;
  requires std::same_as<typename decltype(t.read(data, size))::result_type, Result>;
};

/**
 * @brief Define the abstract async write device
 */
template <class Device>
concept AsyncWrite = requires(Device t, const byte* data, std::size_t size) {
  { t.write(data, size) } -> coro::Awaitable;
  requires std::same_as<typename decltype(t.write(data, size))::result_type, Result>;
};

/**
 * @brief Define the abstract async read and write device
 * @note This concept requires both AsyncRead and AsyncWrite to be satisfied
 */
template <class Device>
concept AsyncReadWrite = AsyncRead<Device> && AsyncWrite<Device>;

struct AsyncReadBase {
  virtual ~AsyncReadBase() = default;
  virtual coro::Task<io::Result> read(byte* data, std::size_t size) = 0;
};

struct AsyncWriteBase {
  virtual ~AsyncWriteBase() = default;
  virtual coro::Task<io::Result> write(const byte* data, std::size_t size) = 0;
};

struct AsyncReadWriteBase : AsyncReadBase, AsyncWriteBase {
  ~AsyncReadWriteBase() override = default;
};

template <AsyncReadWrite Inner>
class AsyncReadWriteWrapper : public AsyncReadWriteBase {
  Inner inner_;

public:
  explicit AsyncReadWriteWrapper(Inner&& inner) : inner_(std::move(inner)) {}
  ~AsyncReadWriteWrapper() override = default;
  coro::Task<io::Result> read(byte* data, std::size_t size) override {
    return inner_.read(data, size);
  }
  coro::Task<io::Result> write(const byte* data, std::size_t size) override {
    return inner_.write(data, size);
  }
  Inner* get() { return &inner_; }
};

class DirectAsyncReadWriteUtils {
public:
  /**
   * @brief Get the read signal
   *
   * @return SPSCSignal2<1>&
   */
  constexpr auto read_signal(this auto&& self) noexcept -> like_t<decltype(self), SPSCSignal2<1>>
    requires requires { self->template signal<IOM_EVENTS::IN>(); }
  {
    return *self->template signal<IOM_EVENTS::IN>();
  }

  /**
   * @brief Get the write signal
   *
   * @return SPSCSignal2<1>&
   */
  constexpr auto write_signal(this auto&& self) noexcept -> like_t<decltype(self), SPSCSignal2<1>>
    requires requires { self->template signal<IOM_EVENTS::OUT>(); }
  {
    return *self->template signal<IOM_EVENTS::OUT>();
  }
  coro::Task<io::Result> read(this auto&& self, byte* data, std::size_t size) {
    return self->read(data, size);
  }
  coro::Task<io::Result> write(this auto&& self, const byte* data, std::size_t size) {
    return self->write(data, size);
  }
  coro::Task<io::Result> write(this auto&& self, const char* data, std::size_t size) {
    return self->write(reinterpret_cast<const byte*>(data), size);
  }
  coro::Task<io::Result> write_file(this auto&& self, WriteFileHint&& hint) {
    return self->write_file(std::move(hint));
  }
};

XSL_ASIO_NE

#endif
