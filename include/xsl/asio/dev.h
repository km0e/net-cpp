/**
 * @file dev.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Async device
 * @version 0.3.0
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_DEV
#  define XSL_ASIO_DEV
#  include <xsl/asio/def.h>
#  include <xsl/asio/io.h>
#  include <xsl/compose.h>
#  include <xsl/io/context.h>
#  include <xsl/type_traits.h>

XSL_ASIO_NB

struct AsyncDeviceUtil {
  /**
   * @brief Get the read signal
   *
   * @return SPSCSignal2<1>&
   */
  constexpr auto read_signal(this auto &&self) noexcept -> like_t<decltype(self), SPSCSignal2<1>> {
    return *self.template signal<IOM_EVENTS::IN>();
  }

  /**
   * @brief Get the write signal
   *
   * @return SPSCSignal2<1>&
   */
  constexpr auto write_signal(this auto &&self) noexcept -> like_t<decltype(self), SPSCSignal2<1>> {
    return *self.template signal<IOM_EVENTS::OUT>();
  }
};

XSL_ASIO_NE
#endif
