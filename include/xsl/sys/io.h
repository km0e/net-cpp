/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief IO utilities
 * @version 0.1.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_SYS_IO
#  define XSL_SYS_IO
#  include <fcntl.h>
#  include <sys/sendfile.h>
#  include <unistd.h>
#  include <xsl/byte.h>
#  include <xsl/io/def.h>
#  include <xsl/io/ext.h>
#  include <xsl/log.h>
#  include <xsl/sys/def.h>
XSL_SYS_NB

// /**
//  * @brief write file to device
//  *
//  * @tparam Dev the device
//  * @param dev the device
//  * @param hint the hint to write file
//  * @return Task<io::Result>
//  */
// template <class Dev>
// Task<io::Result> write_file(Dev &dev, io::WriteFileHint hint) {
//   int ffd = open(hint.path.c_str(), O_RDONLY | O_CLOEXEC);
//   if (ffd == -1) {
//     log_error("open file failed");
//     co_return io::Result{0, errc(errno)};
//   }
//   Defer defer{[ffd] { close(ffd); }};
//   off_t offset = hint.offset;
//   std::size_t map_size = hint.size;
//   auto pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
//   auto pa_size = map_size + (offset - pa_offset);
//   auto *src = mmap(nullptr, pa_size, PROT_READ, MAP_PRIVATE, ffd, pa_offset);
//   if (src == MAP_FAILED) {
//     log_error("mmap failed");
//     co_return io::Result{0, errc(errno)};
//   }
//   Defer defer2{[src, pa_size] { munmap(src, pa_size); }};
//   std::span<byte> data{reinterpret_cast<byte *>(src) + (offset - pa_offset), map_size};
//   log_debug("ready to send {}", data.size());
//   co_return co_await dev.write(data);
// }

XSL_SYS_NE
#endif
