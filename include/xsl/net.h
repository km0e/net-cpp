/**
 * @file net.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Network utilities @version 0.11
 * @date 2024-08-27
 * @version 0.1.1
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_H
#  define XSL_NET_H
#  include <xsl/def.h>
#  include <xsl/net/dns/proto/def.h>
#  include <xsl/net/dns/proto/header.h>
#  include <xsl/net/dns/proto/question.h>
#  include <xsl/net/dns/proto/rr.h>
#  include <xsl/net/dns/utils.h>
#  include <xsl/net/http/msg.h>
#  include <xsl/net/http/proto.h>
#  include <xsl/net/http/proto/accept.h>
#  include <xsl/net/http/request/line.h>
#  include <xsl/net/http/request/target.h>
#  include <xsl/net/http/router.h>
#  include <xsl/net/uri.h>
#  include <xsl/sys.h>

XSL_NB
namespace net {
  using net::AbsoluteUri;
  using sys::net::gai_bind;
  using sys::net::gai_connect;
  using sys::net::SockAddr;
  using sys::net::Socket;
  using sys::net::SocketAttribute;
}  // namespace net

namespace udp {}  // namespace udp
namespace dns {
  using xsl::net::dns::Class;
  using xsl::net::dns::DnCompressor;
  using xsl::net::dns::DnDecompressor;
  using xsl::net::dns::Header;
  using xsl::net::dns::HeaderView;
  using xsl::net::dns::MAX_SIZE_DNS_UDP;
  using xsl::net::dns::Question;
  using xsl::net::dns::RCode;
  using xsl::net::dns::RR;
  using xsl::net::dns::RRSerializer;
  using xsl::net::dns::RRView;
  using xsl::net::dns::skip_question;
  using xsl::net::dns::Type;
  // using xsl::net::dns::Server;
}  // namespace dns
namespace http {
  using xsl::net::http::AbsoluteForm;
  using xsl::net::http::AsteriskForm;
  using xsl::net::http::AuthorityForm;
  using xsl::net::http::from_date_string;
  using xsl::net::http::HTTP_DEFAULT_PORT;
  using xsl::net::http::HTTP_METHOD_COUNT;
  using xsl::net::http::MediaTypeView;
  using xsl::net::http::MessageRestView;
  using xsl::net::http::Method;
  using xsl::net::http::OriginForm;
  using xsl::net::http::parse_accept;
  using xsl::net::http::parse_accept_encoding;
  using xsl::net::http::RequestLine;
  using xsl::net::http::RequestTarget;
  using xsl::net::http::RouteContext;
  using xsl::net::http::Router;
  using xsl::net::http::RouterLike;
  using xsl::net::http::Status;
  using xsl::net::http::to_date_string;
  using xsl::net::http::to_string_view;
  using xsl::net::http::Version;
}  // namespace http

XSL_NE
#endif
