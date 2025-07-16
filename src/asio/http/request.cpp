/**
 * @file request.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-10
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "xsl/asio/http/request.h"

#include "xsl/net.h"

XSL_ASIO_HTTP_NB

RequestPartBuilder& RequestPartBuilder::set_method(http::Method method) {
  auto sv = method.to_string_view();
  this->append(sv);
  return *this;
}

RequestPartBuilder& RequestPartBuilder::set_target(std::string_view target) {
  this->append(' ');
  this->append(target);
  return *this;
}

RequestPartBuilder& RequestPartBuilder::set_version(http::Version version) {
  auto sv = version.to_string_view();
  this->append(' ');
  this->append(sv);
  this->append(CRLF);
  return *this;
}

RequestPartBuilder& RequestPartBuilder::add_header(std::string_view key, std::string_view value) {
  this->append(key);
  this->append(": ");
  this->append(value);
  this->append(CRLF);
  return *this;
}

XSL_ASIO_HTTP_NE
