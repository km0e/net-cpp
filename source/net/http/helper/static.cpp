#include "xsl/net/http/helper/static.h"
#include "xsl/net/http/router.h"
#include "xsl/wheel.h"

#include <sys/stat.h>

HELPER_NAMESPACE_BEGIN
using namespace http::detail;
class FileRouteHandler {
public:
  FileRouteHandler(string&& path);
  ~FileRouteHandler();
  RouteHandleResult operator()(Context& ctx);
  string path;
};
FileRouteHandler::FileRouteHandler(string&& path) : path(path) {}

FileRouteHandler::~FileRouteHandler() {}

RouteHandleResult FileRouteHandler::operator()(Context& ctx) {
  (void)ctx;
  struct stat buf;
  int res = stat(this->path.c_str(), &buf);
  if (res == -1) {
    SPDLOG_ERROR("stat failed: {}", strerror(errno));
    return RouteHandleResult(RouteHandleError("stat failed"));
  }
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(),
                      make_unique<TcpSendFile>(xsl::move(this->path)));
  return RouteHandleResult{make_unique<Response<TcpSendTasks>>(
      ResponsePart{200, "OK", HttpVersion::HTTP_1_1}, xsl::move(tasks))};
}

class FolderRouteHandler {
public:
  FolderRouteHandler(string&& path);
  ~FolderRouteHandler();
  RouteHandleResult operator()(Context& ctx);
  string path;
};
FolderRouteHandler::FolderRouteHandler(string&& path) : path(path) {}
FolderRouteHandler::~FolderRouteHandler() {}
RouteHandleResult FolderRouteHandler::operator()(Context& ctx) {
  string full_path = this->path;
  full_path.append(ctx.current_path.substr(1));
  struct stat buf;
  int res = stat(full_path.c_str(), &buf);
  if (res == -1) {
    SPDLOG_ERROR("stat failed: path: {} error: {}", full_path, strerror(errno));
    return NOT_FOUND_HANDLER(ctx);
  }
  if (S_ISDIR(buf.st_mode)) {
    return RouteHandleResult(NOT_FOUND_HANDLER(ctx));
  }
  TcpSendTasks tasks;
  tasks.emplace_after(tasks.before_begin(),
                      make_unique<TcpSendFile>(xsl::move(full_path)));
  return RouteHandleResult{make_unique<Response<TcpSendTasks>>(
      ResponsePart{200, "OK", HttpVersion::HTTP_1_1}, xsl::move(tasks))};
}
StaticCreateResult create_static_handler(string&& path) {
  if (path.empty()) {
    return AddRouteError{AddRouteErrorKind::InvalidPath};
  }
  struct stat buf;
  int res = stat(path.c_str(), &buf);
  if (res == -1) {
    SPDLOG_ERROR("stat failed: {}", strerror(errno));
    return AddRouteError{AddRouteErrorKind::InvalidPath};
  }
  if (path.back() == '/' && S_ISDIR(buf.st_mode)) {
    return RouteHandler{FolderRouteHandler{xsl::move(path)}};
  } else if (S_ISREG(buf.st_mode)) {
    return RouteHandler{FileRouteHandler{xsl::move(path)}};
  }
  return AddRouteError{AddRouteErrorKind::InvalidPath};
}

HELPER_NAMESPACE_END
