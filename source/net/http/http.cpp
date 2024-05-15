#include "xsl/net/http/def.h"
HTTP_NAMESPACE_BEGIN
string method_cast(HttpMethod method) {
  switch (method) {
    case HttpMethod::EXT:
      return "EXT";
    case HttpMethod::GET:
      return "GET";
    case HttpMethod::POST:
      return "POST";
    case HttpMethod::PUT:
      return "PUT";
    case HttpMethod::DELETE:
      return "DELETE";
    case HttpMethod::HEAD:
      return "HEAD";
    case HttpMethod::OPTIONS:
      return "OPTIONS";
    case HttpMethod::TRACE:
      return "TRACE";
    case HttpMethod::CONNECT:
      return "CONNECT";
    default:
      return "Unknown";
  }
}
HttpMethod method_cast(string_view method) {
  if (method == "EXT") {
    return HttpMethod::EXT;
  } else if (method == "GET") {
    return HttpMethod::GET;
  } else if (method == "POST") {
    return HttpMethod::POST;
  } else if (method == "PUT") {
    return HttpMethod::PUT;
  } else if (method == "DELETE") {
    return HttpMethod::DELETE;
  } else if (method == "HEAD") {
    return HttpMethod::HEAD;
  } else if (method == "OPTIONS") {
    return HttpMethod::OPTIONS;
  } else if (method == "TRACE") {
    return HttpMethod::TRACE;
  } else if (method == "CONNECT") {
    return HttpMethod::CONNECT;
  } else {
    return HttpMethod::UNKNOWN;
  }
}
string version_cast(HttpVersion version) {
  switch (version) {
    case HttpVersion::EXT:
      return "EXT";
    case HttpVersion::HTTP_1_0:
      return "HTTP/1.0";
    case HttpVersion::HTTP_1_1:
      return "HTTP/1.1";
    case HttpVersion::HTTP_2_0:
      return "HTTP/2.0";
    default:
      return "Unknown";
  }
}
HttpVersion version_cast(string_view version) {
  if (version == "EXT") {
    return HttpVersion::EXT;
  } else if (version == "HTTP/1.0") {
    return HttpVersion::HTTP_1_0;
  } else if (version == "HTTP/1.1") {
    return HttpVersion::HTTP_1_1;
  } else if (version == "HTTP/2.0") {
    return HttpVersion::HTTP_2_0;
  } else {
    return HttpVersion::UNKNOWN;
  }
}
HTTP_NAMESPACE_END
