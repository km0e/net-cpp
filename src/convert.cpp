/**
 * @file convert.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/convert.h"
#include "xsl/def.h"

#include <system_error>
XSL_NB

std::string to_string(const std::error_code& ec) { return ec.message(); }

std::string to_string(const std::errc& ec) { return std::make_error_code(ec).message(); }

XSL_NE
