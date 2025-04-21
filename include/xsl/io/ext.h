/**
 * @file ext.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief io utilities
 * @version 0.1
 * @date 2025-04-17
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#ifndef XSL_IO_EXT
#  define XSL_IO_EXT
#  include "xsl/coro.h"
#  include "xsl/io/def.h"

#  include <concepts>

XSL_IO_NB

/// @brief Write file hint
struct WriteFileHint {
  std::string
      path;  ///< file path, must be string, not string_view. Because this function will be called
             ///< in coroutine, and the path may be destroyed before the function is called.
  std::size_t offset;
  std::size_t size;
};

template <class Device>
concept AsyncWriteExt = requires(Device t, WriteFileHint hint) {
  { t.write_file(hint) } -> std::same_as<Task<io::Result>>;
};
XSL_IO_NE

#endif  // !#ifndef XSL_IO_EXTe
