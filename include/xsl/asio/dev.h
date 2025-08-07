/**
 * @file dev.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Async device
 * @version 0.2.0
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_DEV
#  define XSL_ASIO_DEV
#  include <xsl/asio/def.h>
#  include <xsl/asio/raw.h>
#  include <xsl/io/context.h>

XSL_ASIO_NB

using io::IOM_EVENTS;

/// @brief RawAsyncReadWriteDevice is a wrapper for read-write file descriptor with async support
template <IOM_EVENTS... Es>
struct AsyncDevice : public AsyncOwner<Es...> {
  using Base = AsyncOwner<Es...>;
  using Base::Base;
};

XSL_ASIO_NE
#endif
