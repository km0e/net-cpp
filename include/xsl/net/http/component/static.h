#pragma once
#ifndef XSL_NET_HTTP_COMPONENT_STATIC
#  define XSL_NET_HTTP_COMPONENT_STATIC
#  include "xsl/ai/dev.h"
#  include "xsl/coro.h"
#  include "xsl/net/http/component/def.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/sys/net/io.h"

#  include <filesystem>
#  include <string_view>
#  include <system_error>
HTTP_HELPER_NB

template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class FileRouteHandler {
public:
  FileRouteHandler(std::string_view path)
      : path(path), content_type(content_type::MediaType{}, Charset::UTF_8) {
    this->content_type.media_type
        = content_type::MediaType::from_extension(this->path.extension().native());
  }
  ~FileRouteHandler() {}
  HandleResult operator()(HandleContext<ByteReader, ByteWriter>& ctx) {
    (void)ctx;
    std::error_code ec;
    auto status = std::filesystem::status(this->path, ec);
    if (ec || status.type() != std::filesystem::file_type::regular) {
      LOG2("stat failed: path: {} error: {}", this->path.native(), ec.message());
      if (status.type() == std::filesystem::file_type::not_found) {
        return NOT_FOUND_HANDLER<ByteReader, ByteWriter>(ctx);
      }
      return INTERNAL_SERVER_ERROR_HANDLER<ByteReader, ByteWriter>(ctx);
    }

    ResponsePart part{Status::OK};
    auto file_size = std::filesystem::file_size(this->path, ec);
    if (ec) {
      LOG2("file_size failed: path: {} error: {}", this->path.native(), ec.message());
      return INTERNAL_SERVER_ERROR_HANDLER<ByteReader, ByteWriter>(ctx);
    }
    auto last_modified = std::filesystem::last_write_time(this->path, ec);
    if (ec) {
      LOG2("last_write_time failed: path: {} error: {}", this->path.native(), ec.message());
      return INTERNAL_SERVER_ERROR_HANDLER<ByteReader, ByteWriter>(ctx);
    }
    part.headers.emplace("Content-Length", std::to_string(file_size));
    part.headers.emplace("Last-Modified", to_date_string(last_modified));
    part.headers.emplace("Content-Type", this->content_type.to_string());

    auto send_file
        = [hint = sys::net::SendfileHint{this->path.native(), 0, file_size}](ByteWriter& awd) {
            return sys::net::immediate_sendfile(awd, std::move(hint));
          };
    ctx.resp(std::move(part), std::move(send_file));
    return []() -> HandleResult { co_return; }();
  }
  std::filesystem::path path;
  ContentType content_type;
};
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class FolderRouteHandler {
public:
  FolderRouteHandler(std::string_view path) : path(path) {}
  ~FolderRouteHandler() {}
  HandleResult operator()(HandleContext<ByteReader, ByteWriter>& ctx) {
    LOG5("FolderRouteHandler: {}", ctx.current_path);
    if (ctx.current_path.empty()) {
      LOG5("FolderRouteHandler: empty path");
      return {NOT_FOUND_HANDLER<ByteReader, ByteWriter>(ctx)};
    }
    auto full_path = this->path;
    full_path.append(ctx.current_path.substr(1));

    std::error_code ec;
    auto status = std::filesystem::status(full_path, ec);
    if (ec || status.type() != std::filesystem::file_type::regular) {
      LOG2("stat failed: path: {} error: {}", full_path.native(), ec.message());
      if (status.type() == std::filesystem::file_type::not_found) {
        return NOT_FOUND_HANDLER<ByteReader, ByteWriter>(ctx);
      }
      return INTERNAL_SERVER_ERROR_HANDLER<ByteReader, ByteWriter>(ctx);
    }

    ResponsePart part{Status::OK};
    auto file_size = std::filesystem::file_size(full_path, ec);
    if (ec) {
      LOG2("file_size failed: path: {} error: {}", full_path.native(), ec.message());
      return INTERNAL_SERVER_ERROR_HANDLER<ByteReader, ByteWriter>(ctx);
    }
    auto last_modified = std::filesystem::last_write_time(full_path, ec);
    if (ec) {
      LOG2("last_write_time failed: path: {} error: {}", full_path.native(), ec.message());
      return INTERNAL_SERVER_ERROR_HANDLER<ByteReader, ByteWriter>(ctx);
    }
    part.headers.emplace("Content-Length", std::to_string(file_size));
    part.headers.emplace("Last-Modified", to_date_string(last_modified));
    part.headers.emplace(
        "Content-Type",
        ContentType{content_type::MediaType::from_extension(full_path.extension().native()),
                    Charset::UTF_8}
            .to_string());
    auto send_file = [hint
                      = sys::net::SendfileHint{full_path.native(), 0, file_size}](ByteWriter& awd) {
      return sys::net::immediate_sendfile<coro::ExecutorBase, ByteWriter>(awd, std::move(hint));
    };
    ctx.resp(std::move(part), std::move(send_file));
    return []() -> HandleResult { co_return; }();
  }
  std::filesystem::path path;
};
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
RouteHandler<ByteReader, ByteWriter> create_static_handler(std::string_view path) {
  wheel::dynamic_assert(!path.empty(), "path is empty");
  std::error_code ec;
  auto status = std::filesystem::status(path, ec);
  wheel::dynamic_assert(!ec, std::format("stat failed: {}", ec.message()));
  if (status.type() == std::filesystem::file_type::directory) {
    return RouteHandler<ByteReader, ByteWriter>{
        FolderRouteHandler<ByteReader, ByteWriter>{std::move(path)}};
  } else if (status.type() == std::filesystem::file_type::regular) {
    return RouteHandler<ByteReader, ByteWriter>{
        FileRouteHandler<ByteReader, ByteWriter>{std::move(path)}};
  }
  wheel::dynamic_assert(false, "path is not a file or directory");
  return {};
}
HTTP_HELPER_NE
#endif  // XSL_NET_HTTP_HELPER_STATIC
