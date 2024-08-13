#pragma once

#ifndef XSL_NET_HTTP_COMPONENT_STATIC
#  define XSL_NET_HTTP_COMPONENT_STATIC
#  include "xsl/ai/dev.h"
#  include "xsl/net/http/component/compress.h"
#  include "xsl/net/http/component/def.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/proto/accept.h"
#  include "xsl/net/http/proto/media-type.h"
#  include "xsl/sys/net/io.h"
#  include "xsl/wheel/vec.h"

#  include <algorithm>
#  include <charconv>
#  include <cstdlib>
#  include <filesystem>
#  include <optional>
#  include <system_error>
XSL_NET_HTTP_COMPONENT_NB

struct StaticFileConfig {
  StaticFileConfig() = default;
  StaticFileConfig(StaticFileConfig&&) = default;
  StaticFileConfig& operator=(StaticFileConfig&&) = default;
  StaticFileConfig(const StaticFileConfig&) = default;
  StaticFileConfig& operator=(const StaticFileConfig&) = default;
  StaticFileConfig(std::filesystem::path path) : StaticFileConfig(std::move(path), {}) {}
  StaticFileConfig(std::filesystem::path path,
                   wheel::FixedVector<std::string_view> compress_encodings)
      : StaticFileConfig(std::move(path), std::move(compress_encodings), true) {}
  StaticFileConfig(std::filesystem::path path,
                   wheel::FixedVector<std::string_view> compress_encodings, bool compress)
      : path(std::move(path)),
        compress_encodings(std::move(compress_encodings)),
        compress(compress) {}
  std::filesystem::path path;
  wheel::FixedVector<std::string_view> compress_encodings;
  bool compress;
};
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class StaticFileServer {
public:
  StaticFileServer(StaticFileConfig&& cfg) : cfg(std::move(cfg)) {}
  std::optional<Status> sendfile(HandleContext<ByteReader, ByteWriter>& ctx,
                                 std::filesystem::path& path,
                                 const proto::MediaType& content_type) {
    if (auto ok_media_type = ctx.request.get_header("Accept"); ok_media_type) {
      auto media_types = proto::parse_accept(*ok_media_type);
      if (!std::any_of(media_types.begin(), media_types.end(), [content_type](const auto& media) {
            bool any_ok = media.first.main_type() == "*" && media.first.sub_type() == "*";
            return any_ok
                   || (content_type.main_type == media.first.main_type()
                       && content_type.sub_type == media.first.sub_type());
          })) {
        WARN("not acceptable: {}", content_type.to_string());
        return Status::NOT_ACCEPTABLE;
      }
    }

    if (!this->cfg.compress_encodings.empty()) {
      if (auto accept_encoding = ctx.request.get_header("Accept-Encoding"); accept_encoding) {
        auto encodings = proto::parse_accept_encoding(*accept_encoding);
        std::sort(encodings.begin(), encodings.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        for (const auto& [encoding, weight] : encodings) {
          float w;
          if (auto res = std::from_chars(weight.data(), weight.data() + weight.size(), w);
              !res || (w == 0)) {
            continue;
          }
          auto compress_encoding = std::ranges::find_if(
              this->cfg.compress_encodings,
              [encoding](const auto& compress_encoding) { return compress_encoding == encoding; });
          if (compress_encoding == this->cfg.compress_encodings.end()) {
            continue;
          }
          auto ext = encoding_to_extension.find(encoding);
          if (ext == encoding_to_extension.end()) {
            continue;
          }
          path += ext->second;
          auto try_sendfile_res = this->try_sendfile(ctx, path, content_type);
          DEBUG("try_sendfile: path: {} encoding: {}", path.native(), encoding);
          path = path.replace_extension();
          if (!try_sendfile_res) {
            ctx._response->_part.headers.emplace("Content-Encoding", encoding);
            return std::nullopt;
          }
          DEBUG("try_sendfile failed: path: {} error: {}", path.native(),
                to_string_view(*try_sendfile_res));
        }
      }
    }
    return this->try_sendfile(ctx, path, content_type);
  }

protected:
  StaticFileConfig cfg;

  std::optional<Status> try_sendfile(HandleContext<ByteReader, ByteWriter>& ctx,
                                     const std::filesystem::path& path,
                                     const proto::MediaType& content_type) {
    std::error_code ec;
    auto status = std::filesystem::status(path, ec);
    if (ec || status.type() != std::filesystem::file_type::regular) {
      if (status.type() == std::filesystem::file_type::not_found) {
        return Status::NOT_FOUND;
      }
      return Status::INTERNAL_SERVER_ERROR;
    }

    auto file_size = std::filesystem::file_size(path, ec);
    if (ec) {
      LOG2("file_size failed: path: {} error: {}", path.native(), ec.message());
      return Status::INTERNAL_SERVER_ERROR;
    }
    auto last_modified = std::filesystem::last_write_time(path, ec);
    if (ec) {
      LOG2("last_write_time failed: path: {} error: {}", path.native(), ec.message());
      return Status::INTERNAL_SERVER_ERROR;
    }
    ResponsePart part{Status::OK};
    part.headers.emplace("Content-Length", std::to_string(file_size));
    part.headers.emplace("Last-Modified", to_date_string(last_modified));
    part.headers.emplace("Content-Type", content_type.to_string());

    auto send_file = [hint = sys::net::SendfileHint{path.native(), 0, file_size}](ByteWriter& awd) {
      return sys::net::immediate_sendfile(awd, std::move(hint));
    };
    ctx.resp(std::move(part), std::move(send_file));
    return std::nullopt;
  }
};

template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class FileRouteHandler : public StaticFileServer<ByteReader, ByteWriter> {
  using Base = StaticFileServer<ByteReader, ByteWriter>;

public:
  FileRouteHandler(StaticFileConfig&& cfg) : Base(std::move(cfg)), content_type() {
    this->content_type = proto::MediaType::from_extension(this->cfg.path.extension().native());
  }
  FileRouteHandler(FileRouteHandler&&) = default;
  FileRouteHandler& operator=(FileRouteHandler&&) = default;
  FileRouteHandler(const FileRouteHandler&) = default;
  FileRouteHandler& operator=(const FileRouteHandler&) = default;
  ~FileRouteHandler() {}
  HandleResult operator()(HandleContext<ByteReader, ByteWriter>& ctx) {
    co_return this->sendfile(ctx, this->cfg.path, this->content_type);
  }
  proto::MediaType content_type;
};
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
class FolderRouteHandler : public StaticFileServer<ByteReader, ByteWriter> {
  using Base = StaticFileServer<ByteReader, ByteWriter>;

public:
  FolderRouteHandler(StaticFileConfig&& cfg) : Base(std::move(cfg)) {}
  ~FolderRouteHandler() {}
  HandleResult operator()(HandleContext<ByteReader, ByteWriter>& ctx) {
    LOG5("FolderRouteHandler: {}", ctx.current_path);
    if (ctx.current_path.empty()) {
      LOG5("FolderRouteHandler: empty path");
      co_return Status::NOT_FOUND;
    }
    auto full_path = this->cfg.path;
    full_path /= (ctx.current_path.substr(1));
    auto content_type = proto::MediaType::from_extension(full_path.extension().native());
    LOG5("FolderRouteHandler: full path: {}", full_path.native());
    co_return this->sendfile(ctx, full_path, content_type);
  }
};
template <ai::AsyncReadDeviceLike<std::byte> ByteReader,
          ai::AsyncWriteDeviceLike<std::byte> ByteWriter>
Handler<ByteReader, ByteWriter> create_static_handler(StaticFileConfig&& cfg) {
  wheel::dynamic_assert(!cfg.path.empty(), "path is empty");
  std::error_code ec;
  auto status = std::filesystem::status(cfg.path, ec);
  wheel::dynamic_assert(!ec, std::format("stat failed: {}", ec.message()));
  if (status.type() == std::filesystem::file_type::directory) {
    return Handler<ByteReader, ByteWriter>{
        FolderRouteHandler<ByteReader, ByteWriter>{std::move(cfg)}};
  } else if (status.type() == std::filesystem::file_type::regular) {
    auto frh = FileRouteHandler<ByteReader, ByteWriter>(std::move(cfg));
    return Handler<ByteReader, ByteWriter>{std::move(frh)};
  }
  wheel::dynamic_assert(false, "path is not a file or directory");
  return {};
}
XSL_NET_HTTP_COMPONENT_NE
#endif  // XSL_NET_HTTP_HELPER_STATIC
