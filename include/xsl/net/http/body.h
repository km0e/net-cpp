#pragma once
#ifndef XSL_NET_HTTP_BODY
#  define XSL_NET_HTTP_BODY
#  include "xsl/feature.h"
#  include "xsl/net/http/def.h"
#  include "xsl/sys/net/dev.h"

#  include <memory>
HTTP_NB
template <class ByteReader>
class BodyStream {
public:
  BodyStream() : _ard(nullptr), content_part() {}
  BodyStream(std::shared_ptr<ByteReader> ard, std::string_view content)
      : _ard(std::move(ard)), content_part(content) {}
  BodyStream(std::shared_ptr<ByteReader> ard) : _ard(std::move(ard)), content_part() {}
  BodyStream(BodyStream &&) = default;
  BodyStream &operator=(BodyStream &&) = default;
  ~BodyStream() {}

  std::shared_ptr<ByteReader> _ard;
  std::string_view content_part;
};
HTTP_NE
#endif
