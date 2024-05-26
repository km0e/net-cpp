#include "xsl/net/http/context.h"
#include "xsl/net/http/msg.h"
#include "xsl/wheel.h"
HTTP_NAMESPACE_BEGIN
Context::Context(Request&& request)
    : current_path(request.view.uri), request(std::move(request)), is_ok(false) {}
Context::~Context() {}

HTTP_NAMESPACE_END
