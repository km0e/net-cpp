/**
 * @file http_client.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief A simple HTTP client example using xsl::asio
 * @version 0.1.1
 * @date 2025-06-13
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <CLI/CLI.hpp>
#include <xsl/asio.h>
#include <xsl/coro.h>
#include <xsl/io.h>
#include <xsl/log.h>

using namespace xsl::asio;
using namespace xsl;

std::string url = "http://www.baidu.com";
std::string output_file = "";

Task<void> run(std::string_view url) {
  auto tls_builder = TLSContextBuilder::client();
  MUST(tls_builder.set_verify_mode()
           .default_verify_paths()
           .set_min_version()
           .add_mode(TLSMode::ENABLE_PARTIAL_WRITE)
           .build(),
       tls_ctx);
  auto& ctx = co_await CurrentIOContext;
  HttpClient client{};
  client.set_tls_context(std::move(tls_ctx));

  MUST(co_await client.get(url), res);
  auto [response, socket] = std::move(res);
  log_info("Response status: {}", response->line.status_code.to_string_view());
  log_info("Response headers:");
  for (const auto& header : response->rest.headers) {
    log_info("  {}: {}", header.first, header.second);
  }
  std::size_t content_length
      = response->get_header("Content-Length")
            .transform([](const std::string_view& value) { return std::atoi(value.data()); })
            .value_or(0);
  std::size_t current_length = response->rest.content_part.size();
  if (!output_file.empty()) {
    log_info("Saving response to file: {}", output_file);
    std::ofstream ofs(output_file, std::ios::binary);
    if (!ofs) {
      log_error("Failed to open output file: {}", output_file);
      co_return;
    }
    ofs.write(response->rest.content_part.data(), response->rest.content_part.size());
    log_debug("Reading response body from socket...");
    byte buffer[4096];
    while (current_length < content_length) {
      auto res = co_await socket->read(buffer, 4096);
      if (!res) {
        log_error("Failed to read from socket: {}", res.message());
        co_return;
      }
      current_length += res.size;
      ofs.write(reinterpret_cast<const char*>(buffer), res.size);
    }
    log_info("Response saved successfully.");
  } else {
    std::cout.write(response->rest.content_part.data(), response->rest.content_part.size());
    log_debug("Reading response body from socket...");
    byte buffer[4096];
    while (current_length < content_length) {
      auto res = co_await socket->read(buffer, 4096);
      if (!res) {
        log_error("Failed to read from socket: {}", res.message());
        co_return;
      }
      current_length += res.size;
      std::cout.write(reinterpret_cast<const char*>(buffer), res.size);
    }
  }
  ctx.shutdown();
}

int main(int argc, char* argv[]) {
  CLI::App app{"Http client example"};
  app.add_option("--url", url, "The URL to request");
  app.add_option("-o,--output", output_file, "Output file to save the response")
      ->check(CLI::ExistingFile | CLI::NonexistentPath);
  CLI11_PARSE(app, argc, argv);

  MUST(asio_ctx(NewThreadExecutor{}), ctx);
  run(url).detach(*ctx);
  static_cast<sys::IOContext*>(ctx->get_reserved())->run();
  return 0;
}
