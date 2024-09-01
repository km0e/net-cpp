/**
 * @file question.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_QUESTION_H
#  define XSL_NET_DNS_PROTO_QUESTION_H
#  include "xsl/net/dns/def.h"
XSL_NET_DNS_NB
struct Question {
  std::string name;
  std::uint16_t type;
  std::uint16_t qclass;
};
XSL_NET_DNS_NE
#endif
