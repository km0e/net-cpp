#pragma once
#ifndef _XSL_UTILS_FD_H_
#  define _XSL_UTILS_FD_H_
#  include "xsl/utils/def.h"
UTILS_NAMESPACE_BEGIN
bool set_non_blocking(int fd, bool non_blocking = true);
UTILS_NAMESPACE_END
#endif
