/**
 * @file uri.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "xsl/net/uri.h"

#include "xsl/net/def.h"
XSL_NET_NB
const std::regex KVQuery::regex_re(KVQuery::regex_str.data(), KVQuery::regex_str.size());
const std::regex AbsoluteUri::regex_re(AbsoluteUri::regex_str.data(),
                                       AbsoluteUri::regex_str.size());
XSL_NET_NE
