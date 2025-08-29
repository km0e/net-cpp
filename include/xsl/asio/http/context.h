/**
 * @file context.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief HTTP context
 * @version 0.1.2
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_HTTP_CONTEXT
#  define XSL_ASIO_HTTP_CONTEXT
#  include <xsl/asio/http/def.h>
#  include <xsl/asio/http/request.h>
#  include <xsl/asio/http/response.h>
#  include <xsl/io/def.h>
#  include <xsl/net.h>

#  include <chrono>
#  include <optional>
XSL_ASIO_HTTP_NB
using namespace xsl::io;
template <AsyncRead R, AsyncWrite W>
class HandleContext {
public:
  using in_dev_type = R;
  using out_dev_type = W;
  using request_type = Request;
  using response_type = ResponseBuilder<out_dev_type>;

  using response_body_type = Task<Result>(out_dev_type&);
  constexpr HandleContext(std::string_view current_path, request_type& request, in_dev_type& in_dev)
      : current_path(current_path), request(request), in_dev(in_dev), _response(std::nullopt) {}
  constexpr HandleContext(HandleContext&&) = default;
  constexpr HandleContext& operator=(HandleContext&&) = default;
  constexpr ~HandleContext() {}

  /// @brief easy response with status code
  constexpr void easy_resp(Status status_code) {
    this->_response
        = response_type{{Version::HTTP_1_1, status_code, status_code.to_reason_phrase()}};
  }
  /// @brief easy response with status code and body
  constexpr void easy_resp(Status status_code, std::invocable<out_dev_type&> auto&& body) {
    this->_response
        = response_type{{Version::HTTP_1_1, status_code, status_code.to_reason_phrase()},
                        std::forward<decltype(body)>(body)};
  }
  /// @brief easy response with status code and some arguments to construct the body
  template <class... Args>
    requires std::constructible_from<std::string, Args...>
  constexpr void easy_resp(Status status_code, Args&&... args) {
    this->resp({Version::HTTP_1_1, status_code, status_code.to_reason_phrase()},
               std::forward<Args>(args)...);
  }
  /// @brief response with ResponsePart
  constexpr void resp(ResponsePart&& part) { this->_response = response_type{std::move(part)}; }
  /// @brief response with ResponsePart and body
  constexpr void resp(ResponsePart&& part, std::invocable<out_dev_type&> auto&& body) {
    this->_response = response_type{{std::move(part)}, std::forward<decltype(body)>(body)};
  }
  /// @brief response with ResponsePart and some arguments to construct the body
  template <class... Args>
    requires std::constructible_from<std::string, Args...>
  constexpr void resp(ResponsePart&& part, Args&&... args) {
    auto body = std::string(std::forward<Args>(args)...);
    part.headers.emplace("Content-Length", std::to_string(body.size()));
    this->_response = response_type{{std::move(part)},
                                    [body = std::move(body)](out_dev_type& awd) -> Task<Result> {
                                      return awd.write(body.data(), body.size());
                                    }};
  }
  /// @brief checkout the response
  constexpr response_type checkout(this HandleContext&& self) {
    if (!self._response) {
      self.easy_resp(Status::INTERNAL_SERVER_ERROR);
    }
    self.check_and_add_date();
    return std::move(*self._response);
  }

  std::string_view current_path;

  request_type& request;
  in_dev_type& in_dev;

  std::optional<response_type> _response;

private:
  constexpr void check_and_add_date() {
    if (!_response->_part.headers.contains("Date")) {
      _response->_part.headers.emplace("Date", to_date_string(std::chrono::system_clock::now()));
    }
  }
};

using HandleResult = Task<std::optional<Status>>;

template <AsyncRead ABI, AsyncWrite ABO>
using Handler = std::function<HandleResult(HandleContext<ABI, ABO>& ctx)>;

XSL_ASIO_HTTP_NE
#endif
