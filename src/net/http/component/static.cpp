#include "xsl/logctl.h"
#include "xsl/net/http/component/static.h"
#include "xsl/net/http/proto.h"
#include "xsl/net/http/router.h"

#include <sys/stat.h>

#include <filesystem>
#include <system_error>

HTTP_HELPER_NAMESPACE_BEGIN
using namespace http;
class FileRouteHandler {
public:
  FileRouteHandler(std::string&& path);
  ~FileRouteHandler();
  RouteHandleResult operator()(RouteContext& ctx);
  std::string path;
  ContentType content_type;
};
FileRouteHandler::FileRouteHandler(std::string&& path)
    : path(std::move(path)), content_type(content_type::MediaType{}, Charset::UTF_8) {
  if (auto point = this->path.rfind('.'); point != std::string::npos) {
    auto ext = std::string_view(this->path).substr(point + 1);
    this->content_type.media_type = content_type::MediaType::from_extension(ext);
  }
}

FileRouteHandler::~FileRouteHandler() {}

RouteHandleResult FileRouteHandler::operator()(RouteContext& ctx) {
  (void)ctx;
  struct stat buf;
  int res = stat(this->path.c_str(), &buf);
  if (res == -1) {
    ERROR( "stat failed: {}", strerror(errno));
    return RouteHandleResult(RouteHandleError("stat failed"));
  }
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), make_unique<TcpSendFile>(std::move(this->path)));
  auto resp = make_unique<HttpResponse<TcpSendTasks>>(
      ResponsePart{HttpVersion::HTTP_1_1, 200, "OK"}, std::move(tasks));
  resp->part.headers.emplace("Content-Type", to_string(this->content_type));
  return RouteHandleResult{std::move(resp)};
}

class FolderRouteHandler {
public:
  FolderRouteHandler(std::string&& path);
  ~FolderRouteHandler();
  RouteHandleResult operator()(RouteContext& ctx);
  std::string path;
};
FolderRouteHandler::FolderRouteHandler(std::string&& path) : path(std::move(path)) {}
FolderRouteHandler::~FolderRouteHandler() {}
RouteHandleResult FolderRouteHandler::operator()(RouteContext& ctx) {
  DEBUG( "FolderRouteHandler: {}", ctx.current_path);
  std::string full_path = this->path;
  full_path.append(ctx.current_path.substr(1));
  struct stat buf;
  int res = stat(full_path.c_str(), &buf);
  if (res == -1) {
    ERROR( "stat failed: path: {} error: {}", full_path, strerror(errno));
    return NOT_FOUND_HANDLER(ctx);
  }
  if (S_ISDIR(buf.st_mode)) {
    DEBUG( "FolderRouteHandler: is dir");
    return RouteHandleResult(NOT_FOUND_HANDLER(ctx));
  }
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(), make_unique<TcpSendFile>(std::move(full_path)));
  auto resp = make_unique<HttpResponse<TcpSendTasks>>(
      ResponsePart{HttpVersion::HTTP_1_1, 200, "OK"}, std::move(tasks));
  if (auto point = ctx.current_path.rfind('.'); point != std::string::npos) {
    auto ext = ctx.current_path.substr(point + 1);
    resp->part.headers.emplace(
        "Content-Type",
        to_string(ContentType{content_type::MediaType::from_extension(ext), Charset::UTF_8}));
    DEBUG( "FolderRouteHandler: Content-Type: {}", resp->part.headers["Content-Type"]);
  }
  return RouteHandleResult{std::move(resp)};
}
StaticCreateResult create_static_handler(std::string&& path) {
  if (path.empty()) {
    return AddRouteError{AddRouteErrorKind::InvalidPath};
  }
  std::error_code ec;
  auto status = std::filesystem::status(path, ec);
  if (ec) {
    ERROR("filesystem::status failed: {}", ec.message());
    return AddRouteError{AddRouteErrorKind::InvalidPath};
  }
  if (status.type() == std::filesystem::file_type::directory) {
    return RouteHandler{FolderRouteHandler{std::move(path)}};
  } else if (status.type() == std::filesystem::file_type::regular) {
    return RouteHandler{FileRouteHandler{std::move(path)}};
  }
  return AddRouteError{AddRouteErrorKind::InvalidPath};
}

HTTP_HELPER_NAMESPACE_END
