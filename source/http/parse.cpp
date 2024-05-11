#include <spdlog/spdlog.h>
#include <xsl/http/parse.h>

#include <cstddef>

#include "xsl/http/http.h"
HTTP_NAMESPACE_BEGIN
ParseError::ParseError(ParseErrorKind kind) : kind(kind) {}
ParseError::ParseError(ParseErrorKind kind, wheel::string message) : kind(kind), message(message) {}
ParseError::~ParseError() {}
// wheel::vector<RequestResult> HttpParser::parse(const char* data, size_t len) {
//   wheel::vector<RequestResult> reqs;
//   wheel::string_view view(data, len);
//   size_t pos = 0;
//   while (pos < len) {
//     Request req;
//     size_t end = view.find("\r\n\r\n", pos);
//     if (end == wheel::string_view::npos) {
//       break;
//     }
//     req.raw = view.substr(pos, end - pos);
//     wheel::string_view raw_view = req.raw;
//     size_t header_start = raw_view.find("\r\n", pos);
//     wheel::string_view line = raw_view.substr(0, header_start);
//     header_start += 2;
//     size_t space = line.find(' ');
//     if (space == wheel::string_view::npos) {
//       reqs.emplace_back(RequestError(RequestErrorKind::InvalidFormat));
//       break;
//     }
//     req.method_view = line.substr(0, space);
//     req.method = method_cast(req.method_view);
//     spdlog::debug("[HttpParser::parse] method: {}", req.method_view);
//     size_t space2 = line.find(' ', space + 1);
//     if (space2 == wheel::string_view::npos) {
//       reqs.emplace_back(RequestError(RequestErrorKind::InvalidFormat));
//       break;
//     }
//     req.path = line.substr(space + 1, space2 - space - 1);
//     spdlog::debug("[HttpParser::parse] path: {}", req.path);
//     req.version = line.substr(space2 + 1);
//     spdlog::debug("[HttpParser::parse] version: {}", req.version);
//     size_t body_len = 0;
//     while (header_start < raw_view.size()) {
//       size_t header_end = raw_view.find("\r\n", header_start);
//       if (header_end == wheel::string_view::npos) {
//         break;
//       }
//       wheel::string_view header = raw_view.substr(header_start, header_end - header_start);
//       size_t colon = header.find(':');
//       if (colon == wheel::string_view::npos) {
//         reqs.emplace_back(RequestError(RequestErrorKind::InvalidFormat));
//         pos = end + 4;
//         continue;
//       }
//       auto key = header.substr(0, colon);
//       auto value = header.substr(colon + 2);
//       if (key == "Content-Length") {
//         body_len = std::stoul(value.data());
//       }
//       req.headers[key] = value;
//       header_start = header_end + 2;
//     }
//     req.body = view.substr(pos + header_start + 2, body_len);
//     reqs.emplace_back(req);
//     pos = end + 4;
//   }
//   return reqs;
// }

// brief: parse the http request
// you should keep the memory of the view until the next parse
// @param data: the data to be parsed
// @param len: the length of the data, if there is a complete request, the len will be updated to
// the end of the request
// @return: the parsed request or the error
ParseResult HttpParser::parse(const char* data, size_t& len) {
  spdlog::trace("[HttpParser::parse] Parsing request");
  ParseResult res{ParseError(ParseErrorKind::Unknown)};
  wheel::string_view view(data, len);
  size_t pos = 0;
  while (pos < len) {
    size_t end = view.find("\r\n", pos);
    if (end == wheel::string_view::npos) {
      res = ParseError(ParseErrorKind::Partial);
      break;
    } else if (end == pos) {
      len = pos + 2;
      res = this->view;
      this->view = RequestView();
      break;
    }
    if (this->view.version.empty() || this->view.method.empty() || this->view.path.empty()) {
      auto line = view.substr(pos, end - pos);
      size_t _1sp = line.find(' ');
      if (_1sp == wheel::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      this->view.method = line.substr(0, _1sp);
      size_t _2sp = line.find(' ', _1sp + 1);
      if (_2sp == wheel::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      this->view.path = line.substr(_1sp + 1, _2sp - _1sp - 1);
      this->view.version = line.substr(_2sp + 1);
      pos = end + 2;
    } else {
      size_t colon = view.find(':', pos);
      if (colon == wheel::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      auto key = view.substr(pos, colon - pos);
      size_t vstart = view.find_first_not_of(' ', colon + 1);
      size_t vend = view.find("\r\n", vstart);
      if (vend == wheel::string_view::npos) {
        res = ParseError(ParseErrorKind::InvalidFormat);
        break;
      }
      auto value = view.substr(vstart, vend - vstart);
      this->view.headers[key] = value;
      pos = vend + 2;
    }
  }
  spdlog::trace("[HttpParser::parse] Parsed request over");
  return res;
}
HTTP_NAMESPACE_END
