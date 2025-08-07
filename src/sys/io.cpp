/**
 * @file io.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <xsl/sys/io.h>

XSL_SYS_NB
Task<io::Result> read(RawHandle _raw, std::span<byte> buf, Signal<1> &sig) {
  do {
    ssize_t n = ::read(_raw, buf.data(), buf.size());
    if (n >= 0) {
      log_trace("{} recv {} bytes", _raw, n);
      co_return {static_cast<std::size_t>(n)};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, errc::not_connected};
      }
    } else {
      co_return {0, errc(errno)};
    }
  } while (true);
}

Task<io::Result> write(RawHandle _raw, std::span<const byte> data, Signal<1> &sig) {
  do {
    ssize_t n = ::write(_raw, data.data(), data.size());
    if (n >= 0) {
      co_return {static_cast<std::size_t>(n)};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {0, errc::not_connected};
      }
    } else {
      co_return {0, errc(errno)};
    }
  } while (true);
}
XSL_SYS_NE
