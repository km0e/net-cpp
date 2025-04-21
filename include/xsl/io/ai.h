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
class AsyncReadDevice;

class AsyncWriteDevice;

class AsyncReadWriteDevice;

template <class T, class... Flags>
struct AsyncDeviceSelector;

template <class T, class... Flags>
struct AsyncDeviceSelector<In<T>, Flags...> {
  using type = AsyncReadDevice;
};

template <class T, class... Flags>
struct AsyncDeviceSelector<Out<T>, Flags...> {
  using type = AsyncWriteDevice;
};

template <class T, class... Flags>
struct AsyncDeviceSelector<InOut<T>, Flags...> {
  using type = AsyncReadWriteDevice;
};

template <class... Flags>
using AsyncDeviceCompose = organize_feature_flags_t<
    AsyncDeviceSelector<Item<is_same_pack, In<void>, Out<void>, InOut<void>>>, Flags...>;

class AsyncReadDevice {
public:
  virtual ~AsyncReadDevice() = default;
  /// @brief Read from the device
  virtual Task<io::Result> read(std::span<byte> buf [[maybe_unused]]) { std::unreachable(); }
};

class AsyncWriteDevice {
public:
  virtual ~AsyncWriteDevice() = default;
  /// @brief Write to the device
  virtual Task<io::Result> write(std::span<const byte> buf [[maybe_unused]]) { std::unreachable(); }
};

class AsyncReadWriteDevice : public AsyncReadDevice, public AsyncWriteDevice {
public:
  template <template <class> class InOut = InOut>
  using rebind = AsyncDeviceCompose<InOut<void>>::type;
};

class AsyncReadBuffer {
public:
  constexpr virtual ~AsyncReadBuffer() {}
  /// @brief Read from the buffer
  constexpr virtual Task<io::Result> write(AsyncWriteDevice &awd) = 0;
};

class AsyncWriteBuffer {
public:
  virtual ~AsyncWriteBuffer() {}
  /// @brief Write to the buffer
  virtual Task<io::Result> read(AsyncReadDevice &ard) = 0;
};

XSL_IO_NE
#endif
