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
  using _net::AbsoluteUri;
  using sys::net::gai_bind;
  using sys::net::gai_connect;
  using sys::net::SockAddr;
  using sys::net::Socket;
  using sys::net::SocketAttribute;
  // using xsl::_net::io::splice;
}  // namespace net

namespace udp {}  // namespace udp
namespace dns {
  using xsl::_net::dns::Class;
  using xsl::_net::dns::DnCompressor;
  using xsl::_net::dns::DnDecompressor;
  using xsl::_net::dns::Header;
  using xsl::_net::dns::HeaderView;
  using xsl::_net::dns::MAX_SIZE_DNS_UDP;
  using xsl::_net::dns::Question;
  using xsl::_net::dns::RCode;
  using xsl::_net::dns::RR;
  using xsl::_net::dns::RRSerializer;
  using xsl::_net::dns::RRView;
  using xsl::_net::dns::skip_question;
  using xsl::_net::dns::Type;
  // using xsl::_net::dns::Server;
}  // namespace dns

namespace http {
  using xsl::_net::http::AbsoluteForm;
  using xsl::_net::http::AsteriskForm;
  using xsl::_net::http::AuthorityForm;
  using xsl::_net::http::from_date_string;
  using xsl::_net::http::HTTP_DEFAULT_PORT;
  using xsl::_net::http::HTTP_METHOD_COUNT;
  using xsl::_net::http::MediaTypeView;
  using xsl::_net::http::MessageRestView;
  using xsl::_net::http::Method;
  using xsl::_net::http::OriginForm;
  using xsl::_net::http::parse_accept;
  using xsl::_net::http::parse_accept_encoding;
  using xsl::_net::http::RequestLine;
  using xsl::_net::http::RequestTarget;
  using xsl::_net::http::RouteContext;
  using xsl::_net::http::Router;
  using xsl::_net::http::RouterLike;
  using xsl::_net::http::Status;
  using xsl::_net::http::to_date_string;
  using xsl::_net::http::to_string_view;
  using xsl::_net::http::Version;
}  // namespace http

XSL_NE
#endif
