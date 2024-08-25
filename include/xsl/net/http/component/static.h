/**
 * @file static.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_NET_HTTP_COMPONENT_STATIC
#  define XSL_NET_HTTP_COMPONENT_STATIC
#  include "xsl/logctl.h"
#  include "xsl/net/http/component/compress.h"
#  include "xsl/net/http/context.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/proto.h"
#  include "xsl/net/http/proto/accept.h"
#  include "xsl/net/http/proto/media-type.h"
#  include "xsl/sys.h"
#  include "xsl/wheel.h"

#  include <algorithm>
#  include <charconv>
#  include <chrono>
#  include <cstdlib>
#  include <filesystem>
#  include <optional>
#  include <system_error>
XSL_HTTP_NB

struct StaticFileConfig {
  StaticFileConfig() = default;
  StaticFileConfig(StaticFileConfig&&) = default;
  StaticFileConfig& operator=(StaticFileConfig&&) = default;
  StaticFileConfig(const StaticFileConfig&) = default;
  StaticFileConfig& operator=(const StaticFileConfig&) = default;
  StaticFileConfig(std::filesystem::path path) : StaticFileConfig(std::move(path), {}) {}
  StaticFileConfig(std::filesystem::path path, FixedVector<std::string_view> compress_encodings)
      : StaticFileConfig(std::move(path), std::move(compress_encodings), true) {}
  StaticFileConfig(std::filesystem::path path, FixedVector<std::string_view> compress_encodings,
                   bool compress)
      : path(std::move(path)),
        compress_encodings(std::move(compress_encodings)),
        compress(compress) {}
  std::filesystem::path path;
  FixedVector<std::string_view> compress_encodings;
  bool compress;
};
template <ai::ABRL ByteReader, ai::ABWL ByteWriter>
class StaticFileServer {
public:
  StaticFileServer(StaticFileConfig&& cfg) : cfg(std::move(cfg)) {}
  std::optional<Status> sendfile(HandleContext<ByteReader, ByteWriter>& ctx,
                                 std::filesystem::path& path, const MediaTypeView& content_type) {
    if (auto ok_media_type = ctx.request.get_header("Accept"); ok_media_type) {
      auto media_types = parse_accept(*ok_media_type);
      if (!std::ranges::any_of(media_types, [content_type](const auto& media) {
            LOG6("accept media: {}", media.first.to_string_view());
            return media.first.type_includes(content_type);
          })) {
        WARN("not acceptable: {}", content_type.to_string_view());
        return Status::NOT_ACCEPTABLE;
      }
    }

    if (ctx.request.has_header("If-None-Match")) {
      // TODO: implement If-None-Match
    } else if (auto if_modified_since = ctx.request.get_header("If-Modified-Since");
               if_modified_since) {
      auto last_modified = std::filesystem::last_write_time(path);
      auto if_modified_since_time = from_date_string<std::chrono::file_clock>(*if_modified_since);
      if (if_modified_since_time && *if_modified_since_time >= last_modified) {
        DEBUG("not modified: {}", path.native());
        return Status::NOT_MODIFIED;
      }
    }

    DEBUG("check compress: {}", path.native());
    if (!this->cfg.compress_encodings.empty()) {
      if (auto accept_encoding = ctx.request.get_header("Accept-Encoding"); accept_encoding) {
        auto encodings = parse_accept_encoding(*accept_encoding);
        std::sort(encodings.begin(), encodings.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        for (const auto& [encoding, weight] : encodings) {
          float w;
          if (auto res = std::from_chars(weight.data(), weight.data() + weight.size(), w);
              res.ec == std::errc{} || (w == 0)) {
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
                                     const MediaTypeView& content_type) {
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
    part.headers.emplace("Content-Type", content_type.to_string_view());

    auto send_file = [hint = sys::SendfileHint{path.native(), 0, file_size}](ByteWriter& awd) {
      return sys::imm_sendfile(awd, std::move(hint));
    };
    ctx.resp(std::move(part), std::move(send_file));
    return std::nullopt;
  }
};

template <ai::ABRL ByteReader, ai::ABWL ByteWriter>
class FileRouteHandler : public StaticFileServer<ByteReader, ByteWriter> {
  using Base = StaticFileServer<ByteReader, ByteWriter>;

public:
  FileRouteHandler(StaticFileConfig&& cfg) : Base(std::move(cfg)), content_type{} {
    this->content_type = MediaTypeView::from_extension(this->cfg.path.extension().native());
  }
  FileRouteHandler(FileRouteHandler&&) = default;
  FileRouteHandler& operator=(FileRouteHandler&&) = default;
  FileRouteHandler(const FileRouteHandler&) = default;
  FileRouteHandler& operator=(const FileRouteHandler&) = default;
  ~FileRouteHandler() {}
  HandleResult operator()(HandleContext<ByteReader, ByteWriter>& ctx) {
    co_return this->sendfile(ctx, this->cfg.path, this->content_type);
  }
  MediaTypeView content_type;
};
template <ai::ABRL ByteReader, ai::ABWL ByteWriter>
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
    auto content_type = MediaTypeView::from_extension(full_path.extension().native());
    LOG5("FolderRouteHandler: full path: {}", full_path.native());
    co_return this->sendfile(ctx, full_path, content_type);
  }
};
template <ai::ABRL ByteReader, ai::ABWL ByteWriter>
Handler<ByteReader, ByteWriter> create_static_handler(StaticFileConfig&& cfg) {
  dynamic_assert(!cfg.path.empty(), "path is empty");
  std::error_code ec;
  auto status = std::filesystem::status(cfg.path, ec);
  dynamic_assert(!ec, std::format("stat failed: {}", ec.message()));
  if (status.type() == std::filesystem::file_type::directory) {
    return Handler<ByteReader, ByteWriter>{
        FolderRouteHandler<ByteReader, ByteWriter>{std::move(cfg)}};
  } else if (status.type() == std::filesystem::file_type::regular) {
    auto frh = FileRouteHandler<ByteReader, ByteWriter>(std::move(cfg));
    return Handler<ByteReader, ByteWriter>{std::move(frh)};
  }
  dynamic_assert(false, "path is not a file or directory");
  return {};
}
XSL_HTTP_NE
#endif  // XSL_NET_HTTP_HELPER_STATIC
