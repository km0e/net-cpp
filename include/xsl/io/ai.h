/**
 * @file ai.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Async IO abstraction interface
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO_AI
#  define XSL_IO_AI
#  include "xsl/byte.h"
#  include "xsl/feature.h"
#  include "xsl/io/def.h"
XSL_IO_NB
template <class T>
class AsyncReadDevice;

template <class T>
class AsyncWriteDevice;

template <class T>
class AsyncReadWriteDevice;

template <class T, class... Flags>
struct AsyncDeviceSelector;

template <class T, class... Flags>
struct AsyncDeviceSelector<In<T>, Flags...> {
  using type = AsyncReadDevice<T>;
};

template <class T, class... Flags>
struct AsyncDeviceSelector<Out<T>, Flags...> {
  using type = AsyncWriteDevice<T>;
};

template <class T, class... Flags>
struct AsyncDeviceSelector<InOut<T>, Flags...> {
  using type = AsyncReadWriteDevice<T>;
};

template <class... Flags>
using AsyncDeviceCompose = organize_feature_flags_t<
    AsyncDeviceSelector<Item<is_same_pack, In<void>, Out<void>, InOut<void>>>, Flags...>;

template <class T>
class AsyncReadDevice {
public:
  using value_type = T;
  virtual ~AsyncReadDevice() = default;
  /// @brief Read from the device
  virtual Task<io::Result> read(std::span<value_type> buf [[maybe_unused]]) { std::unreachable(); }
};

template <class T>
class AsyncWriteDevice {
public:
  using value_type = T;
  virtual ~AsyncWriteDevice() = default;
  /// @brief Write to the device
  virtual Task<io::Result> write(std::span<const value_type> buf [[maybe_unused]]) {
    std::unreachable();
  }
};

template <class T>
class AsyncReadWriteDevice : public AsyncReadDevice<T>, public AsyncWriteDevice<T> {
public:
  template <template <class> class InOut = InOut>
  using rebind = AsyncDeviceCompose<InOut<T>>::type;
};

template <class T>
class AsyncReadBuffer {
public:
  using value_type = T;
  constexpr virtual ~AsyncReadBuffer() {}
  /// @brief Read from the buffer
  constexpr virtual Task<io::Result> write(AsyncWriteDevice<value_type> &awd) = 0;
};

template <class T>
class AsyncWriteBuffer {
public:
  using value_type = T;
  virtual ~AsyncWriteBuffer() {}
  /// @brief Write to the buffer
  virtual Task<io::Result> read(AsyncReadDevice<value_type> &ard) = 0;
};

using ABR = AsyncReadDevice<byte>;

using ABW = AsyncWriteDevice<byte>;

using ABRW = AsyncReadWriteDevice<byte>;

XSL_IO_NE
#endif
