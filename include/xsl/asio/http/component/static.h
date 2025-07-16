/**
 * @file static.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Static file server component
 * @version 0.1
 * @date 2024-08-25
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#ifndef XSL_ASIO_HTTP_COMPONENT_STATIC
#  define XSL_ASIO_HTTP_COMPONENT_STATIC
#  include "xsl/asio/http/component/compress.h"
#  include "xsl/asio/http/context.h"
#  include "xsl/asio/http/def.h"
#  include "xsl/io.h"
#  include "xsl/logctl.h"
#  include "xsl/net.h"
#  include "xsl/wheel.h"

#  include <algorithm>
#  include <charconv>
#  include <chrono>
#  include <filesystem>
#  include <optional>
#  include <system_error>

XSL_ASIO_HTTP_NB
using namespace xsl::io;
using namespace xsl::http;
/// @brief the static file configuration
struct StaticFileConfig {
  std::filesystem::path path;
  FixedVector<std::string_view> compress_encodings = {};
  bool compress = false;
};

template <AsyncRead R, AsyncWrite W>
class StaticFileServer {
public:
  using in_dev_type = R;
  using out_dev_type = W;

  constexpr StaticFileServer(StaticFileConfig&& cfg) : cfg(std::move(cfg)) {}
  constexpr std::optional<Status> sendfile(HandleContext<in_dev_type, out_dev_type>& ctx,
                                           std::filesystem::path& path,
                                           const MediaTypeView& content_type) {
    if (auto ok_media_type = ctx.request.get_header("Accept");
        ok_media_type) {  // check if the media type is acceptable
      auto media_types = parse_accept(*ok_media_type);
      if (!std::ranges::any_of(media_types, [content_type](const auto& media) {
            log_trace("accept media: {}", media.first.to_string_view());
            return media.first.type_includes(content_type);
          })) {
        log_warning("not acceptable: {}", content_type.to_string_view());
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
        log_debug("not modified: {}", path.native());
        return Status::NOT_MODIFIED;
      }
    }

    log_debug("check compress: {}", path.native());
    /// Check whether compressed file detection is enabled and find the corresponding files
    if (!this->cfg.compress_encodings.empty()) {
      if (auto accept_encoding = ctx.request.get_header("Accept-Encoding"); accept_encoding) {
        auto encodings = parse_accept_encoding(*accept_encoding);
        std::sort(encodings.begin(), encodings.end(), [](const auto& a, const auto& b) {
          return a.second > b.second;
        });  // sort by weight
        for (const auto& [encoding, weight] : encodings) {
          float w;
          if (auto res = std::from_chars(weight.data(), weight.data() + weight.size(), w);
              res.ec == errc{} || (w == 0)) {
            continue;
          }
          auto compress_encoding = std::ranges::find_if(this->cfg.compress_encodings,
                                                        [encoding](const auto& compress_encoding) {
                                                          return compress_encoding == encoding;
                                                        });  //
          if (compress_encoding == this->cfg.compress_encodings.end()) {
            continue;
          }
          auto ext = encoding_to_extension.find(encoding);
          if (ext == encoding_to_extension.end()) {
            continue;
          }
          path += ext->second;
          auto try_sendfile_res = this->try_sendfile(ctx, path, content_type);
          log_debug("try_sendfile: path: {} encoding: {}", path.native(), encoding);
          path = path.replace_extension();
          if (!try_sendfile_res) {
            ctx._response->_part.headers.emplace("Content-Encoding", encoding);
            return std::nullopt;
          }
          log_debug("try_sendfile failed: path: {} error: {}", path.native(),
                    try_sendfile_res->to_reason_phrase());
        }
      }
    }
    return this->try_sendfile(ctx, path, content_type);
  }

protected:
  StaticFileConfig cfg;

  constexpr std::optional<Status> try_sendfile(HandleContext<in_dev_type, out_dev_type>& ctx,
                                               const std::filesystem::path& path,
                                               const MediaTypeView& content_type) {
    std::error_code ec;
    auto status = std::filesystem::status(path, ec);  // check the status of the file
    if (ec || status.type() != std::filesystem::file_type::regular) {
      if (status.type() == std::filesystem::file_type::not_found) {
        return Status::NOT_FOUND;
      }
      return Status::INTERNAL_SERVER_ERROR;
    }

    auto file_size = std::filesystem::file_size(path, ec);  /// get the file size
    if (ec) {
      log_error("file_size failed: path: {} error: {}", path.native(), ec.message());
      return Status::INTERNAL_SERVER_ERROR;
    }
    auto last_modified = std::filesystem::last_write_time(path, ec);
    if (ec) {
      log_error("last_write_time failed: path: {} error: {}", path.native(), ec.message());
      return Status::INTERNAL_SERVER_ERROR;
    }
    ResponsePart part{Status::OK};
    part.headers.emplace("Content-Length", std::to_string(file_size));
    part.headers.emplace("Last-Modified", to_date_string(last_modified));
    part.headers.emplace("Content-Type", content_type.to_string_view());

    auto send_file = [hint = WriteFileHint{path.native(), 0, file_size}](
                         out_dev_type& awd) mutable { return awd.write_file(std::move(hint)); };
    ctx.resp(std::move(part), std::move(send_file));
    return std::nullopt;
  }
};

template <AsyncRead ABI, AsyncWrite ABO>
class FileRouteHandler : public StaticFileServer<ABI, ABO> {
  using Base = StaticFileServer<ABI, ABO>;

public:
  using in_dev_type = ABI;
  using out_dev_type = ABO;

  constexpr FileRouteHandler(StaticFileConfig&& cfg) : Base(std::move(cfg)), content_type{} {
    this->content_type = MediaTypeView::from_extension(this->cfg.path.extension().native());
  }
  constexpr FileRouteHandler(FileRouteHandler&&) = default;
  constexpr FileRouteHandler& operator=(FileRouteHandler&&) = default;
  constexpr FileRouteHandler(const FileRouteHandler&) = default;
  constexpr FileRouteHandler& operator=(const FileRouteHandler&) = default;
  constexpr ~FileRouteHandler() {}
  HandleResult operator()(HandleContext<in_dev_type, out_dev_type>& ctx) {
    co_return this->sendfile(ctx, this->cfg.path, this->content_type);
  }
  MediaTypeView content_type;
};

template <AsyncRead ABI, AsyncWrite ABO>
class FolderRouteHandler : public StaticFileServer<ABI, ABO> {
  using Base = StaticFileServer<ABI, ABO>;

public:
  using in_dev_type = ABI;
  using out_dev_type = ABO;

  constexpr FolderRouteHandler(StaticFileConfig&& cfg) : Base(std::move(cfg)) {}
  constexpr ~FolderRouteHandler() {}
  HandleResult operator()(HandleContext<in_dev_type, out_dev_type>& ctx) {
    log_debug("FolderRouteHandler: {}", ctx.current_path);
    if (ctx.current_path.empty()) {
      log_debug("FolderRouteHandler: empty path");
      co_return Status::NOT_FOUND;
    }
    auto full_path = this->cfg.path;
    full_path /= (ctx.current_path.substr(1));
    auto content_type = MediaTypeView::from_extension(full_path.extension().native());
    log_debug("FolderRouteHandler: full path: {}", full_path.native());
    co_return this->sendfile(ctx, full_path, content_type);
  }
};
/// @brief create a static handler from the config
template <AsyncRead R, AsyncWrite W>
constexpr Handler<R, W> create_static_handler(StaticFileConfig&& cfg) {
  using handler_type = Handler<R, W>;
  rt_assert(!cfg.path.empty(), "path is empty");
  std::error_code ec;
  auto status = std::filesystem::status(cfg.path, ec);
  rt_assert(!ec, std::format("stat failed: {}",
                             ec.message()));  // TODO:use closure to avoid unnecessary copy
  if (status.type() == std::filesystem::file_type::directory) {
    return handler_type{FolderRouteHandler<R, W>{std::move(cfg)}};
  } else if (status.type() == std::filesystem::file_type::regular) {
    auto frh = FileRouteHandler<R, W>(std::move(cfg));
    return handler_type{std::move(frh)};
  }
  rt_assert(false, "path is not a file or directory");
  return {};
}
XSL_ASIO_HTTP_NE
#endif  // XSL_ASIO_HTTP_HELPER_STATIC
