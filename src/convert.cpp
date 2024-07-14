#include "xsl/convert.h"
#include "xsl/def.h"

#include <system_error>
XSL_NB

std::string to_string(const std::error_code& ec) { return ec.message(); }

std::string to_string(const std::errc& ec) { return std::make_error_code(ec).message(); }

XSL_NE
