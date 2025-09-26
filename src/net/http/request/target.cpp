/**
 * @file target.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <xsl/net/http/request/target.h>

#include <format>
XSL_NET_HTTP_NB

const std::regex AuthorityForm::regex_re(AuthorityForm::regex_str.data(),
                                         AuthorityForm::regex_str.size());
const std::regex RequestTarget::regex_re(std::format(R"({}|{}|{}|{})", OriginForm::regex_str,
                                                     AbsoluteForm::regex_str,
                                                     AuthorityForm::regex_str,
                                                     AsteriskForm::regex_str));
XSL_NET_HTTP_NE
