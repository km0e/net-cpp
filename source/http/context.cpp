#include "xsl/http/context.h"
#include "xsl/http/msg.h"
#include "xsl/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
Context::Context(Request&& request)
    : current_path(request.view.path), request(std::move(request)), is_ok(false) {}
Context::~Context() {}

HTTP_NAMESPACE_END
