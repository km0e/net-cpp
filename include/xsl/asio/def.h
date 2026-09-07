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
    namespace asio {
#  define XSL_ASIO_NE \
    }                 \
    XSL_NE
#  include <xsl/byte.h>
#  include <xsl/concept.h>
#  include <xsl/coro.h>
#  include <xsl/def.h>
#  include <xsl/io.h>
#  include <xsl/io/def.h>
#  include <xsl/sys.h>
#  include <xsl/type_traits.h>

#  include <concepts>

XSL_ASIO_NB

using sys::IOContext;
using sys::IOM_EVENTS;

using io::Result;
using IOSignal = MPSCSignal;
using coro::ExecutorBase;

inline const Reserved<IOContext> CurrentIOContext{};

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
class AsyncReadWriteWrapper : public Inner, public AsyncReadWriteBase {
public:
  explicit AsyncReadWriteWrapper(Inner&& inner) : Inner(std::forward<Inner>(inner)) {}
  AsyncReadWriteWrapper() = default;
  ~AsyncReadWriteWrapper() override = default;
  coro::Task<io::Result> read(byte* data, std::size_t size) override {
    return this->Inner::read(data, size);
  }
  coro::Task<io::Result> write(const byte* data, std::size_t size) override {
    return this->Inner::write(data, size);
  }
};

template <class Inner>
  requires AsyncReadWrite<std::decay_t<decltype(*std::declval<Inner>())>>
class DirectAsyncReadWriteWrapper : public Inner {
public:
  decltype(auto) read(byte* data, std::size_t size) { return (*this)->read(data, size); }
  decltype(auto) write(const byte* data, std::size_t size) { return (*this)->write(data, size); }
};

template <class Inner>
// requires AsyncRead<std::decay_t<decltype(*std::declval<Inner>())>>
class DirectAsyncReadWrapper : public Inner {
public:
  decltype(auto) read(byte* data, std::size_t size) { return (*this)->read(data, size); }
};

template <class Inner>
// requires AsyncWrite<std::decay_t<decltype(*std::declval<Inner>())>>
class DirectAsyncWriteWrapper : public Inner {
public:
  decltype(auto) write(const byte* data, std::size_t size) { return (*this)->write(data, size); }
};

/// @brief create a coroutine context bound to a fresh IOContext: the poller
///        becomes the reserved object, and the poller's stop source becomes
///        the context's cancellation domain — IOContext::shutdown() then
///        cancels every auto-cancellable IO await in the domain
template <class E>
  requires std::is_constructible_v<CoroContext, std::stop_source, E, void*, void (*)(void*)>
Expected<Rc<CoroContext>, errc> asio_ctx(E&& e) noexcept {
  TRVEC(ctx, IOContext::create());
  return Rc<CoroContext>(ctx->stop_source(), std::forward<E>(e), ctx,
                         [](void* p) { delete reinterpret_cast<IOContext*>(p); });
}

/// @brief shared-executor overload — for executors that are not
///        move-constructible (e.g. ThreadPoolExecutor)
inline Expected<Rc<CoroContext>, errc> asio_ctx(std::shared_ptr<ExecutorBase> e) noexcept {
  TRVEC(ctx, IOContext::create());
  return Rc<CoroContext>(ctx->stop_source(), std::move(e), ctx,
                         [](void* p) { delete reinterpret_cast<IOContext*>(p); });
}

XSL_ASIO_NE

#endif
