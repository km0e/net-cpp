#include "xsl/http/context.h"
#include "xsl/http/http.h"
#include "xsl/http/msg.h"
#include "xsl/utils/wheel/wheel.h"
HTTP_NAMESPACE_BEGIN
Context::Context(Request&& request) {
    this->request = request;
    this->current_path = request.path;
}
Context::~Context() {}

HTTP_NAMESPACE_END
