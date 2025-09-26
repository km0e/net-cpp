/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_DEF
#  define XSL_NET_DNS_DEF
#  include <xsl/net/def.h>

#  define XSL_NET_DNS_NB \
    XSL_NET_NB           \
    namespace dns {
#  define XSL_NET_DNS_NE \
    }                    \
    XSL_NET_NE
XSL_NET_DNS_NB
XSL_NET_DNS_NE
#endif
