#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_STATIC
#  define XSL_NET_HTTP_COMPONENT_STATIC
#  include "xsl/net/http/component/def.h"
#  include "xsl/net/http/router.h"
HTTP_HELPER_NB

// @brief Create a static handler for a file or folder.
// @param path The path of the file or folder.
// @return A static handler for the file or folder.

RouteHandler create_static_handler(std::string&& path);

HTTP_HELPER_NE
#endif  // XSL_NET_HTTP_HELPER_STATIC
