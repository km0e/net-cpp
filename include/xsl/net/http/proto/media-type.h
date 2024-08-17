#pragma once
#ifndef XSL_NET_HTTP_PROTO_MEDIA_TYPE
#  define XSL_NET_HTTP_PROTO_MEDIA_TYPE
#  include "xsl/net/http/proto/base.h"
#  include "xsl/wheel.h"

#  include <array>
#  include <cstddef>
#  include <cstdint>
#  include <string_view>
XSL_HTTP_NB
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

//<
const std::size_t COMMON_MEDIA_TYPE_COUNT = 82;

namespace common_media_type {
  //< main-type
  const std::string_view any = "*";
  const std::string_view text = "text";
  const std::string_view image = "image";
  const std::string_view audio = "audio";
  const std::string_view video = "video";
  const std::string_view application = "application";
  const std::string_view multipart = "multipart";
  const std::string_view message = "message";
  const std::string_view model = "model";
  //< sub-type
  const std::string_view plain = "plain";
  const std::string_view css = "css";
  const std::string_view csv = "csv";
  const std::string_view html = "html";
  const std::string_view javascript = "javascript";
  const std::string_view json = "json";
  const std::string_view xml = "xml";
  const std::string_view zip = "zip";
  const std::string_view gzip = "gzip";
  const std::string_view jpeg = "jpeg";
  const std::string_view png = "png";
  const std::string_view gif = "gif";
  const std::string_view bmp = "bmp";
  const std::string_view svg = "svg";
  const std::string_view tiff = "tiff";
  const std::string_view webp = "webp";
  const std::string_view mp3 = "mp3";
  const std::string_view wav = "wav";
  const std::string_view ogg = "ogg";
  const std::string_view mpeg = "mpeg";
  const std::string_view mp4 = "mp4";
  const std::string_view webm = "webm";
  const std::string_view ogg_video = "ogg-video";
  const std::string_view ogg_audio = "ogg-audio";
  //< full-type
  const std::string_view any_any = "*/*";
  const std::string_view text_any = "text/*";
  const std::string_view image_any = "image/*";
  const std::string_view audio_any = "audio/*";
  const std::string_view video_any = "video/*";
  const std::string_view application_any = "application/*";
  const std::string_view multipart_any = "multipart/*";
  const std::string_view message_any = "message/*";
  const std::string_view model_any = "model/*";
  const std::string_view text_html = "text/html";
  const std::string_view text_css = "text/css";
  const std::string_view text_xml = "text/xml";
  const std::string_view image_gif = "image/gif";
  const std::string_view image_jpeg = "image/jpeg";
  const std::string_view application_javascript = "application/javascript";
  const std::string_view application_atom_xml = "application/atom+xml";
  const std::string_view application_rss_xml = "application/rss+xml";
  const std::string_view text_mathml = "text/mathml";
  const std::string_view text_plain = "text/plain";
  const std::string_view text_vnd_sun_j2me_app_descriptor = "text/vnd.sun.j2me.app-descriptor";
  const std::string_view text_vnd_wap_wml = "text/vnd.wap.wml";
  const std::string_view text_x_component = "text/x-component";
  const std::string_view image_avif = "image/avif";
  const std::string_view image_png = "image/png";
  const std::string_view image_svg_xml = "image/svg+xml";
  const std::string_view image_tiff = "image/tiff";
  const std::string_view image_vnd_wap_wbmp = "image/vnd.wap.wbmp";
  const std::string_view image_webp = "image/webp";
  const std::string_view image_x_icon = "image/x-icon";
  const std::string_view image_x_jng = "image/x-jng";
  const std::string_view image_x_ms_bmp = "image/x-ms-bmp";
  const std::string_view font_woff = "font/woff";
  const std::string_view font_woff2 = "font/woff2";
  const std::string_view application_java_archive = "application/java-archive";
  const std::string_view application_json = "application/json";
  const std::string_view application_mac_binhex40 = "application/mac-binhex40";
  const std::string_view application_msword = "application/msword";
  const std::string_view application_pdf = "application/pdf";
  const std::string_view application_postscript = "application/postscript";
  const std::string_view application_rtf = "application/rtf";
  const std::string_view application_vnd_apple_mpegurl = "application/vnd.apple.mpegurl";
  const std::string_view application_vnd_google_earth_kml_xml
      = "application/vnd.google-earth.kml+xml";
  const std::string_view application_vnd_google_earth_kmz = "application/vnd.google-earth.kmz";
  const std::string_view application_vnd_ms_excel = "application/vnd.ms-excel";
  const std::string_view application_vnd_ms_fontobject = "application/vnd.ms-fontobject";
  const std::string_view application_vnd_ms_powerpoint = "application/vnd.ms-powerpoint";
  const std::string_view application_vnd_oasis_opendocument_graphics
      = "application/vnd.oasis.opendocument.graphics";
  const std::string_view application_vnd_oasis_opendocument_presentation
      = "application/vnd.oasis.opendocument.presentation";
  const std::string_view application_vnd_oasis_opendocument_spreadsheet
      = "application/vnd.oasis.opendocument.spreadsheet";
  const std::string_view application_vnd_oasis_opendocument_text
      = "application/vnd.oasis.opendocument.text";
  const std::string_view application_vnd_openxmlformats_officedocument_presentationml_presentation
      = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
  const std::string_view application_vnd_openxmlformats_officedocument_spreadsheetml_sheet
      = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
  const std::string_view application_vnd_openxmlformats_officedocument_wordprocessingml_document
      = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
  const std::string_view application_vnd_wap_wmlc = "application/vnd.wap.wmlc";
  const std::string_view application_wasm = "application/wasm";
  const std::string_view application_x_7z_compressed = "application/x-7z-compressed";
  const std::string_view application_x_cocoa = "application/x-cocoa";
  const std::string_view application_x_java_archive_diff = "application/x-java-archive-diff";
  const std::string_view application_x_java_jnlp_file = "application/x-java-jnlp-file";
  const std::string_view application_x_makeself = "application/x-makeself";
  const std::string_view application_x_perl = "application/x-perl";
  const std::string_view application_x_pilot = "application/x-pilot";
  const std::string_view application_x_rar_compressed = "application/x-rar-compressed";
  const std::string_view application_x_redhat_package_manager
      = "application/x-redhat-package-manager";
  const std::string_view application_x_sea = "application/x-sea";
  const std::string_view application_x_shockwave_flash = "application/x-shockwave-flash";
  const std::string_view application_x_stuffit = "application/x-stuffit";
  const std::string_view application_x_tcl = "application/x-tcl";
  const std::string_view application_x_x509_ca_cert = "application/x-x509-ca-cert";
  const std::string_view application_x_xpinstall = "application/x-xpinstall";
  const std::string_view application_xhtml_xml = "application/xhtml+xml";
  const std::string_view application_xspf_xml = "application/xspf+xml";
  const std::string_view application_zip = "application/zip";
  const std::string_view application_octet_stream = "application/octet-stream";
  const std::string_view audio_midi = "audio/midi";
  const std::string_view audio_mpeg = "audio/mpeg";
  const std::string_view audio_ogg = "audio/ogg";
  const std::string_view audio_x_m4a = "audio/x-m4a";
  const std::string_view audio_x_realaudio = "audio/x-realaudio";
  const std::string_view video_3gpp = "video/3gpp";
  const std::string_view video_mp2t = "video/mp2t";
  const std::string_view video_mp4 = "video/mp4";
  const std::string_view video_mpeg = "video/mpeg";
  const std::string_view video_quicktime = "video/quicktime";
  const std::string_view video_webm = "video/webm";
  const std::string_view video_x_flv = "video/x-flv";
  const std::string_view video_x_m4v = "video/x-m4v";
  const std::string_view video_x_mng = "video/x-mng";
  const std::string_view video_x_ms_asf = "video/x-ms-asf";
  const std::string_view video_x_ms_wmv = "video/x-ms-wmv";
  const std::string_view video_x_msvideo = "video/x-msvideo";

  static const us_map<std::string_view> extension_to_media_type = {
      {".html", text_html},
      {".shtml", text_html},
      {".htm", text_html},
      {".css", text_css},
      {".xml", text_xml},
      {".gif", image_gif},
      {".jpeg", image_jpeg},
      {".jpg", image_jpeg},
      {".js", application_javascript},
      {".atom", application_atom_xml},
      {".rss", application_rss_xml},
      {".mml", text_mathml},
      {".txt", text_plain},
      {".jad", text_vnd_sun_j2me_app_descriptor},
      {".wml", text_vnd_wap_wml},
      {".htc", text_x_component},
      {".avif", image_avif},
      {".png", image_png},
      {".svg", image_svg_xml},
      {".svgz", image_svg_xml},
      {".tif", image_tiff},
      {".tiff", image_tiff},
      {".wbmp", image_vnd_wap_wbmp},
      {".webp", image_webp},
      {".ico", image_x_icon},
      {".jng", image_x_jng},
      {".bmp", image_x_ms_bmp},
      {".woff", font_woff},
      {".woff2", font_woff2},
      {".jar", application_java_archive},
      {".war", application_java_archive},
      {".ear", application_java_archive},
      {".json", application_json},
      {".hqx", application_mac_binhex40},
      {".doc", application_msword},
      {".pdf", application_pdf},
      {".ps", application_postscript},
      {".eps", application_postscript},
      {".ai", application_postscript},
      {".rtf", application_rtf},
      {".m3u8", application_vnd_apple_mpegurl},
      {".kml", application_vnd_google_earth_kml_xml},
      {".kmz", application_vnd_google_earth_kmz},
      {".xls", application_vnd_ms_excel},
      {".eot", application_vnd_ms_fontobject},
      {".ppt", application_vnd_ms_powerpoint},
      {".odg", application_vnd_oasis_opendocument_graphics},
      {".odp", application_vnd_oasis_opendocument_presentation},
      {".ods", application_vnd_oasis_opendocument_spreadsheet},
      {".odt", application_vnd_oasis_opendocument_text},
      {".pptx", application_vnd_openxmlformats_officedocument_presentationml_presentation},
      {".xlsx", application_vnd_openxmlformats_officedocument_spreadsheetml_sheet},
      {".docx", application_vnd_openxmlformats_officedocument_wordprocessingml_document},
      {".wmlc", application_vnd_wap_wmlc},
      {".wasm", application_wasm},
      {".7z", application_x_7z_compressed},
      {".cco", application_x_cocoa},
      {".jardiff", application_x_java_archive_diff},
      {".jnlp", application_x_java_jnlp_file},
      {".run", application_x_makeself},
      {".pl", application_x_perl},
      {".pm", application_x_perl},
      {".prc", application_x_pilot},
      {".pdb", application_x_pilot},
      {".rar", application_x_rar_compressed},
      {".rpm", application_x_redhat_package_manager},
      {".sea", application_x_sea},
      {".swf", application_x_shockwave_flash},
      {".sit", application_x_stuffit},
      {".tcl", application_x_tcl},
      {".tk", application_x_tcl},
      {".der", application_x_x509_ca_cert},
      {".pem", application_x_x509_ca_cert},
      {".crt", application_x_x509_ca_cert},
      {".xpi", application_x_xpinstall},
      {".xhtml", application_xhtml_xml},
      {".xspf", application_xspf_xml},
      {".zip", application_zip},
      {".bin", application_octet_stream},
      {".exe", application_octet_stream},
      {".dll", application_octet_stream},
      {".deb", application_octet_stream},
      {".dmg", application_octet_stream},
      {".iso", application_octet_stream},
      {".img", application_octet_stream},
      {".msi", application_octet_stream},
      {".msp", application_octet_stream},
      {".msm", application_octet_stream},

      {".mid", audio_midi},
      {".midi", audio_midi},
      {".kar", audio_midi},
      {".mp3", audio_mpeg},
      {".ogg", audio_ogg},
      {".m4a", audio_x_m4a},
      {".ra", audio_x_realaudio},
      {".3gpp", video_3gpp},
      {".3gp", video_3gpp},
      {".ts", video_mp2t},
      {".mp4", video_mp4},
      {".mpeg", video_mpeg},
      {".mpg", video_mpeg},
      {".mov", video_quicktime},
      {".webm", video_webm},
      {".flv", video_x_flv},
      {".m4v", video_x_m4v},
      {".mng", video_x_mng},
      {".asx", video_x_ms_asf},
      {".asf", video_x_ms_asf},
      {".wmv", video_x_ms_wmv},
      {".avi", video_x_msvideo},
  };

}  // namespace common_media_type
const std::array<std::string_view, COMMON_MEDIA_TYPE_COUNT> COMMON_MEDIA_TYPES
    = {"text/html",
       "text/css",
       "text/xml",
       "image/gif",
       "image/jpeg",
       "application/javascript",
       "application/atom+xml",
       "application/rss+xml",
       "text/mathml",
       "text/plain",
       "text/vnd.sun.j2me.app-descriptor",
       "text/vnd.wap.wml",
       "text/x-component",
       "image/avif",
       "image/png",
       "image/svg+xml",
       "image/tiff",
       "image/vnd.wap.wbmp",
       "image/webp",
       "image/x-icon",
       "image/x-jng",
       "image/x-ms-bmp",
       "font/woff",
       "font/woff2",
       "application/java-archive",
       "application/json",
       "application/mac-binhex40",
       "application/msword",
       "application/pdf",
       "application/postscript",
       "application/rtf",
       "application/vnd.apple.mpegurl",
       "application/vnd.google-earth.kml+xml",
       "application/vnd.google-earth.kmz",
       "application/vnd.ms-excel",
       "application/vnd.ms-fontobject",
       "application/vnd.ms-powerpoint",
       "application/vnd.oasis.opendocument.graphics",
       "application/vnd.oasis.opendocument.presentation",
       "application/vnd.oasis.opendocument.spreadsheet",
       "application/vnd.oasis.opendocument.text",
       "application/vnd.openxmlformats-officedocument.presentationml.presentation",
       "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
       "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
       "application/vnd.wap.wmlc",
       "application/wasm",
       "application/x-7z-compressed",
       "application/x-cocoa",
       "application/x-java-archive-diff",
       "application/x-java-jnlp-file",
       "application/x-makeself",
       "application/x-perl",
       "application/x-pilot",
       "application/x-rar-compressed",
       "application/x-redhat-package-manager",
       "application/x-sea",
       "application/x-shockwave-flash",
       "application/x-stuffit",
       "application/x-tcl",
       "application/x-x509-ca-cert",
       "application/x-xpinstall",
       "application/xhtml+xml",
       "application/xspf+xml",
       "application/zip",
       "application/octet-stream",
       "audio/midi",
       "audio/mpeg",
       "audio/ogg",
       "audio/x-m4a",
       "audio/x-realaudio",
       "video/3gpp",
       "video/mp2t",
       "video/mp4",
       "video/mpeg",
       "video/quicktime",
       "video/webm",
       "video/x-flv",
       "video/x-m4v",
       "video/x-mng",
       "video/x-ms-asf",
       "video/x-ms-wmv",
       "video/x-msvideo"};

//< mime-type

struct MediaTypeBase {
  static MediaTypeBase from_extension(std::string_view extension);
  const char *start;
  const char *slash;
  const char *end;
  std::string_view main_type() const;
  std::string_view sub_type() const;
  bool type_includes(const MediaTypeBase &other) const;
  std::string_view to_string_view() const;
};

struct MediaType : MediaTypeBase {
  static MediaType from_extension(std::string_view extension);
  Parameters parameters;
  std::string to_string() const;
};

struct MediaTypeView : MediaTypeBase {
  static MediaTypeView from_extension(std::string_view extension);
  ParametersView parameters;
  std::string_view to_string_view() const;
};

XSL_HTTP_NE
#endif
