/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief just define the namespace for tcp
 * @version 0.1.0
 * @date 2024-08-18
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_TCP
#  define XSL_ASIO_TCP
#  define XSL_ASIO_TCP_NB \
    XSL_ASIO_NB           \
    namespace tcp {
#  define XSL_ASIO_TCP_NE \
    }                     \
    XSL_ASIO_NE
#endif
