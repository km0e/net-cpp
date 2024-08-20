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
