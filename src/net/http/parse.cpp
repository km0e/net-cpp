#include "xsl/net/http/parse.h"
#include "xsl/net/http/proto.h"

#include <spdlog/spdlog.h>

#include <regex>

HTTP_NAMESPACE_BEGIN
ParseError::ParseError(ParseErrorKind kind)
    : kind(kind), message(PARSE_ERROR_STRINGS[static_cast<int>(kind)]) {}
ParseError::ParseError(ParseErrorKind kind, std::string message) : kind(kind), message(message) {}
ParseError::~ParseError() {}
std::string ParseError::to_string() const {
  return std::string("ParseError: ")
         + std::string(PARSE_ERROR_STRINGS[static_cast<int>(this->kind)]) + " " + this->message;
}

ParseResult HttpParser::parse(const char* data, size_t& len) {
  ParseResult res{ParseError(ParseErrorKind::Unknown)};
  std::string_view view(data, len);
  size_t pos = 0, parse_end = 0;
  while (pos < len) {
    size_t end = view.find("\r\n", pos);
    if (end == std::string_view::npos) {
      res = ParseError(ParseErrorKind::Partial);
      break;
    } else if (end == pos) {
      parse_end = 2;
      res = std::move(this->view);
      this->view = RequestView();
      break;
    }
    if (this->view.version.empty() || this->view.method.empty() || this->view.url.empty()) {
      auto line = view.substr(pos, end - pos);
      size_t _1sp = line.find(' ');
      if (_1sp == std::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      this->view.method = line.substr(0, _1sp);
      size_t _2sp = line.find(' ', _1sp + 1);
      if (_2sp == std::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      this->view.url = line.substr(_1sp + 1, _2sp - _1sp - 1);
      // query map
      size_t query_pos = this->view.url.find('?');
      if (query_pos != std::string_view::npos) {
        auto query_start = query_pos + 1;
        while (true) {
          auto and_pos = this->view.url.find('&', query_start);
          if (and_pos == std::string_view::npos) {
            break;
          }
          auto eq_pos = this->view.url.find('=', query_start);
          if (eq_pos == std::string_view::npos) {
            this->view.query[this->view.url.substr(query_start)] = "";
            break;
          } else {
            this->view.query[this->view.url.substr(query_start, eq_pos - query_start)]
                = this->view.url.substr(eq_pos + 1, and_pos - eq_pos - 1);
          }
          query_start = and_pos + 1;
        }
        auto eq_pos = this->view.url.find('=', query_start);
        if (eq_pos != std::string_view::npos) {
          this->view.query[this->view.url.substr(query_start, eq_pos - query_start)]
              = this->view.url.substr(eq_pos + 1);
        } else {
          this->view.query[this->view.url.substr(query_start)] = "";
        }
        this->view.url = this->view.url.substr(0, query_pos);
      }
      auto tmpv = line.substr(_2sp + 1);
      if (std::regex_match(tmpv.begin(), tmpv.end(), HTTP_VERSION_REGEX)) {
        this->view.version = tmpv;
      } else {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      pos = end + 2;
    } else {
      size_t colon = view.find(':', pos);
      if (colon == std::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      auto key = view.substr(pos, colon - pos);
      size_t vstart = view.find_first_not_of(' ', colon + 1);
      size_t vend = view.find("\r\n", vstart);
      if (vend == std::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      auto value = view.substr(vstart, vend - vstart);
      this->view.headers[key] = value;
      pos = vend + 2;
    }
  }
  len = pos + parse_end;
  return res;
}

ParseResult HttpParser::parse(std::string_view& data) {
  size_t len = data.size();
  auto res = this->parse(data.data(), len);
  data = data.substr(len);
  return res;
}

void HttpParser::clear() { this->view.clear(); }

HTTP_NAMESPACE_END
