#include <xsl/http/helper.h>
#include <xsl/http/msg.h>

#include "xsl/http/context.h"
#include "xsl/http/http.h"
HTTP_NAMESPACE_BEGIN
StaticRouteHandler::StaticRouteHandler(wheel::string&& path) : path(path) {}
StaticRouteHandler::~StaticRouteHandler() {}
xsl::http::Response StaticRouteHandler::operator()(Context& request) {
    return xsl::http::Response();
}
HTTP_NAMESPACE_END
