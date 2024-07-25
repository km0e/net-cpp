#pragma once
#ifndef XSL_NET_HTTP_BODY
#  define XSL_NET_HTTP_BODY
#  include "xsl/net/http/def.h"
#  include "xsl/sys/io.h"

#  include <memory>
HTTP_NB
class BodyStream {
public:
  BodyStream(std::shared_ptr<sys::io::AsyncReadDevice> ard) : _ard(std::move(ard)) {}
  BodyStream(BodyStream &&) = default;
  BodyStream &operator=(BodyStream &&) = default;
  ~BodyStream() {}

private:
  std::shared_ptr<sys::io::AsyncReadDevice> _ard;
};
HTTP_NE
#endif
