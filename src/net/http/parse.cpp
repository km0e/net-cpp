#include "xsl/net/http/parse.h"
#include "xsl/net/http/proto.h"
#include "xsl/regex.h"

#include <regex>
#include <system_error>

XSL_HTTP_NB
ParseUnit::ParseUnit() : view() {}

ParseResult ParseUnit::parse(const char* data, size_t len) {  // TODO: request target
  std::expected<RequestView, std::errc> res = std::unexpected{std::errc()};
  std::string_view view(data, len);
  size_t pos = 0, parse_end = 0;
  while (pos < len) {
    size_t end = view.find("\r\n", pos);
    if (end == std::string_view::npos) {
      res = std::unexpected{std::errc::resource_unavailable_try_again};
      break;
    } else if (end == pos) {
      parse_end = 2;
      res = std::move(this->view);
      this->view = RequestView();
      break;
    }
    if (this->view.version.empty() || this->view.method.empty()
        || (this->view.path.empty() && this->view.authority.empty())) {
      auto line = view.substr(pos, end - pos);
      size_t _1sp = line.find(' ');
      if (_1sp == std::string_view::npos) {
        res = std::unexpected{std::errc::illegal_byte_sequence};
        break;
      }
      this->view.method = line.substr(0, _1sp);
      size_t _2sp = line.find(' ', _1sp + 1);
      if (_2sp == std::string_view::npos) {
        res = std::unexpected{std::errc::illegal_byte_sequence};
        break;
      }
      this->parse_request_target(line.substr(_1sp + 1, _2sp - _1sp - 1));
      auto tmp_version = line.substr(_2sp + 1);
      if (std::regex_match(tmp_version.begin(), tmp_version.end(), HTTP_VERSION_REGEX)) {
        this->view.version = tmp_version;
      } else {
        res = std::unexpected{std::errc::illegal_byte_sequence};
        break;
      }
      pos = end + 2;
    } else {
      size_t colon = view.find(':', pos);
      if (colon == std::string_view::npos) {
        res = std::unexpected{std::errc::illegal_byte_sequence};
        break;
      }
      auto key = view.substr(pos, colon - pos);
      size_t vstart = view.find_first_not_of(' ', colon + 1);
      size_t vend = view.find("\r\n", vstart);
      if (vend == std::string_view::npos) {
        res = std::unexpected{std::errc::illegal_byte_sequence};
        break;
      }
      auto value = view.substr(vstart, vend - vstart);
      this->view.headers[key] = value;
      pos = vend + 2;
    }
  }
  return {pos + parse_end, std::move(res)};
}

ParseResult ParseUnit::parse(std::string_view data) {
  return this->parse(data.data(), data.length());
}

void ParseUnit::clear() { this->view.clear(); }

void ParseUnit::parse_request_target(std::string_view target) {
  static const std::regex REQUEST_TARGET_REGEX(std::format(
      R"({}|{}|([^/?#]*)|{})", regex::origin_form, regex::absolute_form, regex::asterisk_form));
  static const std::regex QUERY_REGEX(R"(([^&=]+)=([^&=]+))");
  std::cmatch match;
  if (std::regex_match(target.begin(), target.end(), match, REQUEST_TARGET_REGEX)) {
    this->view.query.clear();
    if (match[1].matched) {
      this->view.scheme = "";
      this->view.authority = "";
      this->view.path = std::string_view(match[1].first, match[1].length());
      std::cregex_iterator query_start(match[2].first, match[2].second, QUERY_REGEX);
      std::cregex_iterator query_end;
      while (query_start != query_end) {
        this->view.query[std::string_view((*query_start)[1].first, (*query_start)[1].length())]
            = std::string_view((*query_start)[2].first, (*query_start)[2].length());
        ++query_start;
      }
    } else if (match[3].matched) {
      this->view.scheme = std::string_view(match[3].first, match[3].length());
      this->view.authority = std::string_view(match[4].first, match[4].length());
      this->view.path = std::string_view(match[5].first, match[5].length());
      std::cregex_iterator query_start(match[6].first, match[6].second, QUERY_REGEX);
      std::cregex_iterator query_end;
      while (query_start != query_end) {
        this->view.query[std::string_view((*query_start)[1].first, (*query_start)[1].length())]
            = std::string_view((*query_start)[2].first, (*query_start)[2].length());
        ++query_start;
      }
    } else if (match[7].matched) {
      this->view.scheme = "";
      this->view.authority = std::string_view(match[7].first, match[7].length());
      this->view.path = "";
    } else if (match[8].matched) {
      this->view.scheme = "";
      this->view.authority = "";
      this->view.path = "*";
    }
  }
}

XSL_HTTP_NE
