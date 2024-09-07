/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.11
 * @date 2024-08-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO
#  define XSL_IO
#  include "xsl/def.h"
#  include "xsl/io/def.h"
#  include "xsl/io/splice.h"
#  include "xsl/sys/io.h"

#  include <fcntl.h>
XSL_NB
using io::AIOTraits;
using io::IOTraits;

using io::splice;
using io::splice_once;
using io::WriteFileHint;

template <class T>
class AsyncReadDevice {
public:
  using value_type = T;
  using dynamic_type = AsyncReadDevice;
  virtual ~AsyncReadDevice() = default;
  /**
  @brief read data from the device

  @param buf the buffer to indicate the data
  @return Task<io::Result> the io::Result of the read, the
  first element is the size of the data read, the second element is the error code, if there is
  error,
   */
  virtual Task<io::Result> read(std::span<value_type> buf [[maybe_unused]]) { std::unreachable(); }
};

template <class T>
class AsyncWriteDevice {
public:
  using value_type = T;
  using dynamic_type = AsyncWriteDevice;
  virtual ~AsyncWriteDevice() = default;
  virtual Task<io::Result> write(std::span<const value_type> buf [[maybe_unused]]) {
    std::unreachable();
  }
};

template <class T>
class AsyncReadWriteDevice : public AsyncReadDevice<T>, public AsyncWriteDevice<T> {};

template <class Dev>
class DynAsyncReadDevice : public AsyncReadDevice<typename Dev::value_type> {
private:
  Dev dev;

public:
  using value_type = typename Dev::value_type;
  using dynamic_type = AsyncReadDevice<value_type>;

  DynAsyncReadDevice(auto &&...args) : dev(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> read(std::span<value_type> buf) override {
    return AIOTraits<Dev>::read(dev, buf);
  }
};

template <class Dev>
class DynAsyncWriteDevice : public AsyncWriteDevice<typename Dev::value_type> {
private:
  Dev dev;

public:
  using value_type = typename Dev::value_type;
  using dynamic_type = AsyncWriteDevice<value_type>;

  DynAsyncWriteDevice(auto &&...args) : dev(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> write(std::span<const value_type> buf) override {
    return AIOTraits<Dev>::write(dev, buf);
  }
};

template <class Dev>
class DynAsyncReadWriteDevice : public AsyncReadWriteDevice<typename Dev::value_type> {
private:
  Dev dev;

public:
  using value_type = typename Dev::value_type;
  using dynamic_type = AsyncReadWriteDevice<value_type>;

  DynAsyncReadWriteDevice(auto &&...args) : dev(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> read(std::span<value_type> buf) override {
    return AIOTraits<Dev>::read(dev, buf);
  }

  Task<io::Result> write(std::span<const value_type> buf) override {
    return AIOTraits<Dev>::write(dev, buf);
  }
};

template <class T>
class AsyncReadBuffer {
public:
  using value_type = T;
  constexpr virtual ~AsyncReadBuffer() {}
  constexpr virtual Task<io::Result> write(AsyncWriteDevice<value_type> &awd) = 0;
};
template <class T>
class AsyncWriteBuffer {
public:
  using value_type = T;
  virtual ~AsyncWriteBuffer() {}
  virtual Task<io::Result> read(AsyncReadDevice<value_type> &ard) = 0;
};

using ABR = AsyncReadDevice<byte>;

using ABW = AsyncWriteDevice<byte>;

using ABRW = AsyncReadWriteDevice<byte>;

template <class T>
struct AIOTraits<AsyncReadDevice<T>> {
  using value_type = T;
  using device_type = AsyncReadDevice<T>;
  static constexpr Task<Result> read(device_type &dev, std::span<byte> buf) {
    return dev.read(buf);
  }
};

template <class T>
struct AIOTraits<AsyncWriteDevice<T>> {
  using value_type = T;
  using device_type = AsyncWriteDevice<T>;
  static constexpr Task<Result> write(device_type &dev, std::span<const byte> buf) {
    return dev.write(buf);
  }

  static Task<Result> write_file(device_type &dev, WriteFileHint &&hint) {
    return _sys::write_file(dev, std::move(hint));
  }
};

template <class T>
struct AIOTraits<AsyncReadWriteDevice<T>> {
  using value_type = byte;
  using device_type = AsyncReadWriteDevice<T>;

  static constexpr Task<Result> read(device_type &dev, std::span<byte> buf) {
    return dev.read(buf);
  }

  static constexpr Task<Result> write(device_type &dev, std::span<const byte> buf) {
    return dev.write(buf);
  }

  static constexpr Task<Result> write_file(device_type &dev, WriteFileHint &&hint) {
    return _sys::write_file(dev, std::move(hint));
  }
};

template <class Dev>
struct IODynGetChain {
  using type = typename Dev::io_dyn_chains;
};

XSL_NE
#endif
