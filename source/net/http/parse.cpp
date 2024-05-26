#include "xsl/net/http/parse.h"
#include "xsl/wheel.h"

#include <spdlog/spdlog.h>

HTTP_NAMESPACE_BEGIN
ParseError::ParseError(ParseErrorKind kind)
    : kind(kind), message(PARSE_ERROR_STRINGS[static_cast<int>(kind)]) {}
ParseError::ParseError(ParseErrorKind kind, string message) : kind(kind), message(message) {}
ParseError::~ParseError() {}
std::string ParseError::to_string() const {
  return std::string("ParseError: ")
         + std::string(PARSE_ERROR_STRINGS[static_cast<int>(this->kind)]) + " " + this->message;
}

ParseResult HttpParser::parse(const char* data, size_t& len) {
  SPDLOG_TRACE("Parsing request");
  ParseResult res{ParseError(ParseErrorKind::Unknown)};
  string_view view(data, len);
  size_t pos = 0;
  while (pos < len) {
    size_t end = view.find("\r\n", pos);
    if (end == string_view::npos) {
      len = pos;
      res = ParseError(ParseErrorKind::Partial);
      break;
    } else if (end == pos) {
      len = pos + 2;
      res = xsl::move(this->view);
      this->view = RequestView();
      break;
    }
    if (this->view.version.empty() || this->view.method.empty() || this->view.uri.empty()) {
      auto line = view.substr(pos, end - pos);
      size_t _1sp = line.find(' ');
      if (_1sp == string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      this->view.method = line.substr(0, _1sp);
      size_t _2sp = line.find(' ', _1sp + 1);
      if (_2sp == string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      this->view.uri = line.substr(_1sp + 1, _2sp - _1sp - 1);
      this->view.version = line.substr(_2sp + 1);
      pos = end + 2;
    } else {
      size_t colon = view.find(':', pos);
      if (colon == string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      auto key = view.substr(pos, colon - pos);
      size_t vstart = view.find_first_not_of(' ', colon + 1);
      size_t vend = view.find("\r\n", vstart);
      if (vend == string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      auto value = view.substr(vstart, vend - vstart);
      this->view.headers[key] = value;
      pos = vend + 2;
    }
  }
  SPDLOG_TRACE("Parsed request over");
  return res;
}
HTTP_NAMESPACE_END
