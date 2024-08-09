#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_STATIC
#  define XSL_NET_HTTP_COMPONENT_STATIC
#  include "xsl/ai/dev.h"
#  include "xsl/net/http/component/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/sys/net/io.h"

#  include <filesystem>
HTTP_HELPER_NB

template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class FileRouteHandler {
public:
  FileRouteHandler(std::string&& path)
      : path(std::move(path)), content_type(content_type::MediaType{}, Charset::UTF_8) {
    if (auto point = this->path.rfind('.'); point != std::string::npos) {
      auto ext = std::string_view(this->path).substr(point + 1);
      this->content_type.media_type = content_type::MediaType::from_extension(ext);
    }
  }
  ~FileRouteHandler() {}
  HandleResult operator()(HandleContext<ByteReader, ByteWriter>& ctx) {
    (void)ctx;
    struct stat buf;
    int res = stat(this->path.c_str(), &buf);
    if (res == -1) {
      LOG2("stat failed: {}", strerror(errno));
      co_return std::unexpected{RouteError::NotFound};
    }
    auto send_file = std::bind(sys::net::immediate_sendfile<coro::ExecutorBase>,
                               std::placeholders::_1, this->path);
    ResponsePart part{Status::OK};
    part.headers.emplace("Content-Type", to_string(this->content_type));
    ctx.resp(std::move(part), std::move(send_file));
    co_return;
  }
  std::string path;
  ContentType content_type;
};
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class FolderRouteHandler {
public:
  FolderRouteHandler(std::string&& path) : path(std::move(path)) {}
  ~FolderRouteHandler() {}
  HandleResult operator()(HandleContext<ByteReader, ByteWriter>& ctx) {
    LOG5("FolderRouteHandler: {}", ctx.current_path);
    std::string full_path = this->path;
    full_path.append(ctx.current_path.substr(1));
    struct stat buf;
    int res = stat(full_path.c_str(), &buf);
    if (res == -1) {
      LOG2("stat failed: path: {} error: {}", full_path, strerror(errno));
      return NOT_FOUND_HANDLER<ByteReader, ByteWriter>(ctx);
    }
    if (S_ISDIR(buf.st_mode)) {
      LOG5("FolderRouteHandler: is dir");
      return {NOT_FOUND_HANDLER<ByteReader, ByteWriter>(ctx)};
    }
    auto send_file = std::bind(sys::net::immediate_sendfile<coro::ExecutorBase>,
                               std::placeholders::_1, full_path);
    ResponsePart part{Status::OK};
    if (auto point = ctx.current_path.rfind('.'); point != std::string::npos) {
      auto ext = ctx.current_path.substr(point + 1);
      part.headers.emplace(
          "Content-Type",
          to_string(ContentType{content_type::MediaType::from_extension(ext), Charset::UTF_8}));
      LOG5("FolderRouteHandler: Content-Type: {}", part.headers["Content-Type"]);
    }
    ctx.resp(std::move(part), std::move(send_file));
  }
  std::string path;
};
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
RouteHandler<ByteReader, ByteWriter> create_static_handler(std::string&& path) {
  if (path.empty()) {
    throw std::invalid_argument("path is empty");
  }
  std::error_code ec;
  auto status = std::filesystem::status(path, ec);
  if (ec) {
    throw std::system_error(ec);
  }
  if (status.type() == std::filesystem::file_type::directory) {
    return RouteHandler<ByteReader, ByteWriter>{
        FolderRouteHandler<ByteReader, ByteWriter>{std::move(path)}};
  } else if (status.type() == std::filesystem::file_type::regular) {
    return RouteHandler<ByteReader, ByteWriter>{
        FileRouteHandler<ByteReader, ByteWriter>{std::move(path)}};
  }
  throw std::invalid_argument("path is not a file or directory");
}
HTTP_HELPER_NE
#endif  // XSL_NET_HTTP_HELPER_STATIC
