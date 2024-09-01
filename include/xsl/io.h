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
#  include "xsl/sys/def.h"

#  include <fcntl.h>
XSL_NB
using io::AIOTraits;
using io::IOTraits;

using io::splice;
using io::splice_once;

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

  template <class... _Args>
  DynAsyncReadDevice(_Args &&...args) : dev(std::forward<_Args>(args)...) {}

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

  template <class... _Args>
  DynAsyncWriteDevice(_Args &&...args) : dev(std::forward<_Args>(args)...) {}

  Task<io::Result> write(std::span<const value_type> buf) override {
    return AIOTraits<Dev>::write(dev, buf);
  }
};

template <class T>
class AsyncWritable {
public:
  using value_type = T;
  virtual ~AsyncWritable() {}
  virtual Task<io::Result> write(AsyncWriteDevice<value_type> &awd) = 0;
};
template <class T>
class AsyncReadable {
public:
  using value_type = T;
  virtual ~AsyncReadable() {}
  virtual Task<io::Result> read(AsyncReadDevice<value_type> &ard) = 0;
};

using ABR = AsyncReadDevice<byte>;

using ABW = AsyncWriteDevice<byte>;

template <class T>
struct AIOTraits<AsyncReadDevice<T>> {
  using value_type = T;
  using device_type = AsyncReadDevice<T>;
  static Task<Result> read(device_type &dev, std::span<byte> buf) { return dev.read(buf); }
};

template <class T>
struct AIOTraits<AsyncWriteDevice<T>> {
  using value_type = T;
  using device_type = AsyncWriteDevice<T>;
  static Task<Result> write(device_type &dev, std::span<const byte> buf) { return dev.write(buf); }

  static Task<Result> write_file(device_type &dev, _sys::WriteFileHint hint) {
    int ffd = open(hint.path.c_str(), O_RDONLY | O_CLOEXEC);
    if (ffd == -1) {
      LOG2("open file failed");
      co_return io::Result{0, {std::errc(errno)}};
    }
    Defer defer{[ffd] { close(ffd); }};
    off_t offset = hint.offset;
    std::size_t map_size = hint.size;
    auto pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
    auto pa_size = map_size + (offset - pa_offset);
    auto *src = mmap(nullptr, pa_size, PROT_READ, MAP_PRIVATE, ffd, pa_offset);
    if (src == MAP_FAILED) {
      LOG2("mmap failed");
      co_return io::Result{0, {std::errc(errno)}};
    }
    Defer defer2{[src, pa_size] { munmap(src, pa_size); }};
    std::span<byte> data{reinterpret_cast<byte *>(src) + (offset - pa_offset), map_size};
    co_return co_await dev.write(data);
  }
};

template <class T>
struct AIOTraits<AsyncReadWriteDevice<T>> {
  using value_type = byte;
  using device_type = AsyncReadWriteDevice<T>;

  static Task<Result> read(device_type &dev, std::span<byte> buf) { return dev.read(buf); }

  static Task<Result> write(device_type &dev, std::span<const byte> buf) { return dev.write(buf); }
};

template <class Dev>
struct IODynGet {
  using type = typename Dev::io_dyn_chains;
};

XSL_NE
#endif
