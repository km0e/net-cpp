#include <xsl/http/config.h>
#include <xsl/http/http_msg.h>
HTTP_NAMESPACE_BEGIN
RequestError::RequestError(RequestErrorKind kind) : kind(kind) {}
RequestError::RequestError(RequestErrorKind kind, wheel::string message)
    : kind(kind), message(message) {}
RequestError::~RequestError() {}
HttpRequest::HttpRequest() {}
HttpRequest::~HttpRequest() {}

HTTP_NAMESPACE_END
