#pragma once
#ifndef XSL_NET_HTTP_PROTO_MEDIA_TYPE
#  define XSL_NET_HTTP_PROTO_MEDIA_TYPE
#  include "xsl/net/http/proto/base.h"
#  include "xsl/net/http/proto/def.h"

#  include <array>
#  include <cstdint>
XSL_NET_HTTP_PROTO_NB
enum class MediaMainType : uint8_t {
  ANY,
  TEXT,
  IMAGE,
  AUDIO,
  VIDEO,
  APPLICATION,
  MULTIPART,
  MESSAGE,
  MODEL,
  UNKNOWN = 0xff,
};

const std::size_t MEDIA_MAIN_TYPE_COUNT = 9;

const std::array<std::string_view, MEDIA_MAIN_TYPE_COUNT> MEDIA_MAIN_TYPE_STRINGS = {
    "*", "text", "image", "audio", "video", "application", "multipart", "message", "model",
};

std::string_view to_string_view(const MediaMainType &type);

bool operator==(MediaMainType lhs, std::string_view rhs);

enum class MediaSubType : uint8_t {
  ANY,
  PLAIN,
  CSS,
  CSV,
  HTML,
  JAVASCRIPT,
  JSON,
  XML,
  ZIP,
  GZIP,
  JPEG,
  PNG,
  GIF,
  BMP,
  SVG,
  TIFF,
  WEBP,
  MP3,
  WAV,
  OGG,
  MPEG,
  MP4,
  WEBM,
  OGG_VIDEO,
  OGG_AUDIO,
  PDF,
  ZIP_APPLICATION,
  GZIP_APPLICATION,
  OCTET_STREAM,
  FORM_DATA,
  SIGNED,
  ENCRYPTED,
  BYTERANGES,
  UNKNOWN = 0xff,
};

const std::size_t MEDIA_SUB_TYPE_COUNT = 33;

const std::array<std::string_view, MEDIA_SUB_TYPE_COUNT> MEDIA_SUB_TYPE_STRINGS = {
    "*",          "plain", "css",  "csv",  "html",         "javascript", "json",   "xml",
    "zip",        "gzip",  "jpeg", "png",  "gif",          "bmp",        "svg",    "tiff",
    "webp",       "mp3",   "wav",  "ogg",  "mpeg",         "mp4",        "webm",   "ogg-video",
    "ogg-audio",  "pdf",   "zip",  "gzip", "octet-stream", "form-data",  "signed", "encrypted",
    "byteranges",
};

std::string_view to_string_view(const MediaSubType &subtype);

bool operator==(MediaSubType lhs, std::string_view rhs);

struct MediaType {
  static MediaType from_extension(std::string_view extension);
  MediaType();
  MediaType(MediaMainType main_type, MediaSubType sub_type);
  MediaType(MediaType &&) = default;
  MediaType &operator=(MediaType &&) = default;
  MediaType(const MediaType &) = default;
  MediaType &operator=(const MediaType &) = default;
  MediaMainType main_type;
  MediaSubType sub_type;
  Parameters parameters;
  std::string to_string() const;
};

struct MediaTypeView {
  const char *start;
  const char *slash;
  const char *end;
  ParametersView parameters;
  std::string_view main_type() const;
  std::string_view sub_type() const;
};

XSL_NET_HTTP_PROTO_NE
#endif
