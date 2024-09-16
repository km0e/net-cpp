/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief IO utilities
 * @version 0.11
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_IO
#  define XSL_SYS_IO
#  include "xsl/io/def.h"
#  include "xsl/sys/def.h"

#  include <fcntl.h>
#  include <sys/sendfile.h>
#  include <unistd.h>
XSL_SYS_NB
/**
 * @brief read from device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param sig the signal receiver
 * @param buf the buffer to store the data
 * @return Task<io::Result>
 */
template <class Pointer>
Task<io::Result> read(RawHandle _raw, std::span<byte> buf, const Signal<1, Pointer> &sig) {
  do {
    ssize_t n = ::read(_raw, buf.data(), buf.size());
    if (n >= 0) {
      LOG6("{} recv {} bytes", _raw, n);
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, {errc::not_connected}};
      }
    } else {
      co_return {0, {errc(errno)}};
    }
  } while (true);
}

struct FileRxTraits {
  Task<io::Result> read(this auto &&self, std::span<byte> buf) {
    return _sys::read(self.raw(), buf, self.read_signal());
  }
};

/**
 * @brief write to device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param sig the signal receiver
 * @param data the data to write
 * @return Task<io::Result>
 */
template <class Pointer>
Task<io::Result> write(RawHandle _raw, std::span<const byte> data, const Signal<1, Pointer> &sig) {
  do {
    ssize_t n = ::write(_raw, data.data(), data.size());
    if (n >= 0) {
      co_return {n, std::nullopt};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, {errc::not_connected}};
      }
    } else {
      co_return {0, {errc(errno)}};
    }
  } while (true);
}
/**
 * @brief write file to device
 *
 * @tparam Dev the device
 * @param dev the device
 * @param hint the hint to write file
 * @return Task<io::Result>
 */
template <class Dev>
Task<io::Result> write_file(Dev &dev, io::WriteFileHint hint) {
  int ffd = open(hint.path.c_str(), O_RDONLY | O_CLOEXEC);
  if (ffd == -1) {
    LOG2("open file failed");
    co_return io::Result{0, {errc(errno)}};
  }
  Defer defer{[ffd] { close(ffd); }};
  off_t offset = hint.offset;
  std::size_t map_size = hint.size;
  auto pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
  auto pa_size = map_size + (offset - pa_offset);
  auto *src = mmap(nullptr, pa_size, PROT_READ, MAP_PRIVATE, ffd, pa_offset);
  if (src == MAP_FAILED) {
    LOG2("mmap failed");
    co_return io::Result{0, {errc(errno)}};
  }
  Defer defer2{[src, pa_size] { munmap(src, pa_size); }};
  std::span<byte> data{reinterpret_cast<byte *>(src) + (offset - pa_offset), map_size};
  DEBUG("ready to send {}", data.size());
  co_return co_await dev.write(data);
}
struct FileTxTraits {
  Task<io::Result> write(this auto &&self, std::span<const byte> data) {
    return _sys::write(self.raw(), data, self.write_signal());
  }

  template <class Pointer>
  Task<io::Result> write_file(this auto &&self, io::WriteFileHint &&hint) {
    return _sys::write_file(self, std::move(hint));
  }
};
XSL_SYS_NE
#endif
