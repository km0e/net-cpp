#pragma once
#ifndef XSL_NET_HTTP_BODY
#  define XSL_NET_HTTP_BODY
#  include "xsl/feature.h"
#  include "xsl/net/http/def.h"
#  include "xsl/sys/io/dev.h"

#  include <memory>
HTTP_NB
class BodyStream {
public:
  BodyStream() = default;
  BodyStream(std::shared_ptr<sys::io::AsyncDevice<feature::In<std::byte>>> ard,
             std::string_view content)
      : _ard(std::move(ard)), content_part(content) {}
  BodyStream(std::shared_ptr<sys::io::AsyncDevice<feature::In<std::byte>>> ard)
      : _ard(std::move(ard)), content_part() {}
  BodyStream(BodyStream &&) = default;
  BodyStream &operator=(BodyStream &&) = default;
  ~BodyStream() {}

private:
  std::shared_ptr<sys::io::AsyncDevice<feature::In<std::byte>>> _ard;
  std::string_view content_part;
};
HTTP_NE
#endif
