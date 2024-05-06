#include <spdlog/spdlog.h>
#include <xsl/config.h>
#include <xsl/http/config.h>
#include <xsl/http/http_msg.h>
#include <xsl/http/http_server.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/tcp_server.h>
#include <xsl/utils/wheel/wheel.h>

HTTP_NAMESPACE_BEGIN

HandlerGenerator::HandlerGenerator(wheel::shared_ptr<HttpRouter> router) : router(router) {}
transport::Handler HandlerGenerator::operator()() { return HttpHandler(this->router); }

wheel::vector<RequestResult> HttpParser::parse(const char* data, size_t len) {
  wheel::vector<RequestResult> reqs;
  wheel::string_view view(data, len);
  size_t pos = 0;
  while (pos < len) {
    HttpRequest req;
    size_t end = view.find("\r\n\r\n", pos);
    if (end == wheel::string_view::npos) {
      break;
    }
    req.raw = view.substr(pos, end - pos);
    wheel::string_view raw_view = req.raw;
    size_t header_start = raw_view.find("\r\n", pos);
    wheel::string_view line = raw_view.substr(0, header_start);
    header_start += 2;
    size_t space = line.find(' ');
    if (space == wheel::string_view::npos) {
      reqs.emplace_back(RequestError(RequestErrorKind::InvalidFormat));
      break;
    }
    req.method = line.substr(0, space);
    spdlog::debug("[HttpParser::parse] method: {}", req.method);
    size_t space2 = line.find(' ', space + 1);
    if (space2 == wheel::string_view::npos) {
      reqs.emplace_back(RequestError(RequestErrorKind::InvalidFormat));
      break;
    }
    req.path = line.substr(space + 1, space2 - space - 1);
    spdlog::debug("[HttpParser::parse] path: {}", req.path);
    req.version = line.substr(space2 + 1);
    spdlog::debug("[HttpParser::parse] version: {}", req.version);
    size_t body_len = 0;
    while (header_start < raw_view.size()) {
      size_t header_end = raw_view.find("\r\n", header_start);
      if (header_end == wheel::string_view::npos) {
        break;
      }
      wheel::string_view header = raw_view.substr(header_start, header_end - header_start);
      size_t colon = header.find(':');
      if (colon == wheel::string_view::npos) {
        reqs.emplace_back(RequestError(RequestErrorKind::InvalidFormat));
        pos = end + 4;
        continue;
      }
      auto key = header.substr(0, colon);
      auto value = header.substr(colon + 2);
      if (key == "Content-Length") {
        body_len = std::stoul(value.data());
      }
      req.headers[key] = value;
      header_start = header_end + 2;
    }
    req.body = view.substr(pos + header_start + 2, body_len);
    reqs.emplace_back(req);
    pos = end + 4;
  }
  return reqs;
}

HttpHandler::HttpHandler(wheel::shared_ptr<HttpRouter> router) : router(router) {}
HttpHandler::~HttpHandler() {}
transport::HandleState HttpHandler::operator()(int read_write, wheel::string& data) {
  if (read_write == 0) {
    auto reqs = this->parser.parse(data.c_str(), data.size());
    wheel::string res;
    res.reserve(1024);
    for (auto& req : reqs) {
      if (req.is_err()) {
        res += "HTTP/1.1 400 Bad Request\r\n\r\n";
        continue;
      }
      auto tmp = req.unwrap();
      res += this->router->route(tmp);
    }
    data = res;
    return transport::HandleState{sync::IOM_EVENTS::OUT, transport::HandleHint::WRITE};
  } else {
    return transport::HandleState{sync::IOM_EVENTS::NONE, transport::HandleHint::NONE};
  }
}

HTTP_NAMESPACE_END
