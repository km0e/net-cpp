/**
 * @file dyn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Dynamic IO utilities
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO_DYN
#  define XSL_IO_DYN
#  include "xsl/io/ai.h"
#  include "xsl/io/def.h"
XSL_IO_NB
/// @brief Dynamic AsyncReadDevice
template <class Dev>
class DynAsyncReadDevice : public io::AsyncReadDevice {
private:
  Dev dev;

public:
  using value_type = typename Dev::value_type;

  DynAsyncReadDevice(auto &&...args) : dev(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> read(std::span<value_type> buf) override { return dev.read(buf); }
};
/// @brief Dynamic AsyncWriteDevice
template <class Dev>
class DynAsyncWriteDevice : public AsyncWriteDevice {
private:
  Dev dev;

public:
  using value_type = typename Dev::value_type;

  DynAsyncWriteDevice(auto &&...args) : dev(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> write(std::span<const value_type> buf) override { return dev.write(buf); }
};
/// @brief Dynamic AsyncReadWriteDevice
template <class Dev>
class DynAsyncReadWriteDevice : public AsyncReadWriteDevice {
private:
  Dev dev;

public:
  using value_type = typename Dev::value_type;

  DynAsyncReadWriteDevice(auto &&...args) : dev(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> read(std::span<value_type> buf) override { return dev.read(buf); }

  Task<io::Result> write(std::span<const value_type> buf) override { return dev.write(buf); }
};
/// @brief Dynamic AsyncReadBuffer
template <class Buf>
class DynAsyncReadBuffer : public AsyncReadBuffer {
private:
  Buf buf;

public:
  using value_type = typename Buf::value_type;

  DynAsyncReadBuffer(auto &&...args) : buf(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> write(AsyncWriteDevice &awd) override { return buf.write(awd); }
};
/// @brief Dynamic AsyncWriteBuffer
template <class Buf>
class DynAsyncWriteBuffer : public AsyncWriteBuffer {
private:
  Buf buf;

public:
  using value_type = typename Buf::value_type;

  DynAsyncWriteBuffer(auto &&...args) : buf(std::forward<decltype(args)>(args)...) {}

  Task<io::Result> read(AsyncReadDevice &ard) override { return buf.read(ard); }
};

XSL_IO_NE
#endif
