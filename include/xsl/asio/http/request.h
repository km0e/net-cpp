/**
 * @file request.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP request definitions
 * @version 0.1
 * @date 2025-07-10
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP_REQUEST
#  define XSL_ASIO_HTTP_REQUEST
#  include "xsl/asio/http/common.h"
#  include "xsl/asio/http/def.h"
#  include "xsl/net.h"

XSL_ASIO_HTTP_NB
using namespace xsl::http;

/// @brief the request
class Request : public Message {  // TODO: abstract the ard
public:
  constexpr Request() : Message(), line() {}

  RequestLineView line;
};

/**
 * @class RequestBuilder
 * @brief A builder for constructing HTTP requests.
 * @note Function call sequence:
 *       - set_method
 *       - set_target
 *       - set_version
 *       - add_header (optional, can be called multiple times)
 */
class RequestPartBuilder {
  std::vector<std::unique_ptr<byte[]>> _raw;  ///< The raw data buffer for the request part
  std::size_t offset = 0;                     ///< Offset for writing to the buffer

public:
  constexpr RequestPartBuilder() : _raw() {
    this->_raw.emplace_back(std::make_unique<byte[]>(HTTP_BUFFER_BLOCK_SIZE));
  }
  RequestPartBuilder(RequestPartBuilder&&) = default;
  RequestPartBuilder& operator=(RequestPartBuilder&&) = default;
  ~RequestPartBuilder() = default;

  bool is_valid() const { return !this->_raw.empty(); }
  RequestPartBuilder& append(char c) {
    this->check_append();
    this->_raw.back().get()[this->offset++] = byte(c);
    return *this;
  }
  RequestPartBuilder& append(std::string_view str) {
    if (str.empty()) {
      return *this;
    }
    this->check_append();
    auto data = std::as_bytes(std::span(str));
    if (this->offset + data.size() <= HTTP_BUFFER_BLOCK_SIZE) {
      std::ranges::move(data, this->_raw.back().get() + this->offset);
      this->offset += data.size();
      return *this;
    }
    std::size_t remaining = HTTP_BUFFER_BLOCK_SIZE - this->offset;
    std::ranges::copy_n(data.begin(), remaining, this->_raw.back().get() + this->offset);
    this->_raw.emplace_back(std::make_unique<byte[]>(HTTP_BUFFER_BLOCK_SIZE));
    data = data.subspan(remaining);
    while (data.size() > HTTP_BUFFER_BLOCK_SIZE) {
      std::ranges::copy_n(data.begin(), HTTP_BUFFER_BLOCK_SIZE, this->_raw.back().get());
      this->_raw.emplace_back(std::make_unique<byte[]>(HTTP_BUFFER_BLOCK_SIZE));
    }
    if (!data.empty()) {
      std::ranges::move(data, this->_raw.back().get());
      this->offset = data.size();
    }
    return *this;
  }

  RequestPartBuilder& set_method(Method method);
  RequestPartBuilder& set_target(std::string_view target);
  RequestPartBuilder& set_version(Version version);
  RequestPartBuilder& add_header(std::string_view key, std::string_view value);

  template <AsyncWrite W>
  Task<errc> write(W& awd) {
    log_debug("Whole message: {}", this->to_string());
    if (!this->is_valid()) {
      co_return errc::invalid_argument;  // Return an error if the request part is not valid
    }
    this->append(CRLF);  // Append CRLF to indicate the end of the request part
    for (const auto& block : this->_raw | std::views::take(this->_raw.size() - 1)) {
      auto res = co_await awd.write(block.get(), HTTP_BUFFER_BLOCK_SIZE);
      if (!res) {
        log_error("Failed to write request part: {}", res.message());
        co_return std::move(res.ec);
      }
    }
    if (this->offset > 0) {
      auto res = co_await awd.write(this->_raw.back().get(), this->offset);
      if (!res) {
        log_error("Failed to write request part: {}", res.message());
        co_return std::move(res.ec);
      }
    }
    co_return errc{};  // Return success if all blocks are written successfully
  }

  std::string to_string() const {
    std::string result;
    for (const auto& block : this->_raw | std::views::take(this->_raw.size() - 1)) {
      result.append(reinterpret_cast<const char*>(block.get()), HTTP_BUFFER_BLOCK_SIZE);
    }
    if (this->offset > 0) {
      result.append(reinterpret_cast<const char*>(this->_raw.back().get()), this->offset);
    }
    result.append(CRLF);  // Append CRLF to indicate the end of the request part
    return result;
  }

private:
  void check_append() {
    if (this->offset == HTTP_BUFFER_BLOCK_SIZE) {
      this->_raw.emplace_back(std::make_unique<byte[]>(HTTP_BUFFER_BLOCK_SIZE));
      this->offset = 0;
    }
  }
};

XSL_ASIO_HTTP_NE
#endif
