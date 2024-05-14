#include "xsl/http/helper/static.h"
#include "xsl/http/router.h"
#include "xsl/transport/tcp/tcp.h"
#include "xsl/wheel/wheel.h"

#include <sys/stat.h>

HELPER_NAMESPACE_BEGIN
using namespace http::detail;
class FileRouteHandler {
public:
  FileRouteHandler(wheel::string&& path);
  ~FileRouteHandler();
  RouteHandleResult operator()(Context& ctx);
  wheel::string path;
};
FileRouteHandler::FileRouteHandler(wheel::string&& path) : path(path) {}

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
                      wheel::make_unique<TcpSendFile>(wheel::move(this->path)));
  return RouteHandleResult{wheel::make_unique<Response<TcpSendTasks>>(
      ResponsePart{200, "OK", HttpVersion::HTTP_1_1}, wheel::move(tasks))};
}

class FolderRouteHandler {
public:
  FolderRouteHandler(wheel::string&& path);
  ~FolderRouteHandler();
  RouteHandleResult operator()(Context& ctx);
  wheel::string path;
};
FolderRouteHandler::FolderRouteHandler(wheel::string&& path) : path(path) {}
FolderRouteHandler::~FolderRouteHandler() {}
RouteHandleResult FolderRouteHandler::operator()(Context& ctx) {
  wheel::string full_path = this->path;
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
                      wheel::make_unique<TcpSendFile>(wheel::move(full_path)));
  return RouteHandleResult{wheel::make_unique<Response<TcpSendTasks>>(
      ResponsePart{200, "OK", HttpVersion::HTTP_1_1}, wheel::move(tasks))};
}
StaticCreateResult create_static_handler(wheel::string&& path) {
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
    return RouteHandler{FolderRouteHandler{wheel::move(path)}};
  } else if (S_ISREG(buf.st_mode)) {
    return RouteHandler{FileRouteHandler{wheel::move(path)}};
  }
  return AddRouteError{AddRouteErrorKind::InvalidPath};
}

HELPER_NAMESPACE_END
