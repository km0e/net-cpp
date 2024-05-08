#include <xsl/http/helper.h>
#include <xsl/http/msg.h>

#include "xsl/http/context.h"
#include "xsl/http/http.h"
#include <filesystem>
HTTP_NAMESPACE_BEGIN
StaticRouteHandler::StaticRouteHandler(wheel::string&& path) : path(path) {}
StaticRouteHandler::~StaticRouteHandler() {}
xsl::http::Response StaticRouteHandler::operator()(Context& ctx) {
  if (ctx.current_path == "") {
    if(std::filesystem::is_regular_file(path)) {
        
      return xsl::http::Response();
    }
    return xsl::http::Response();
  }
  return xsl::http::Response();
}
HTTP_NAMESPACE_END
