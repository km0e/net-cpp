/**
 * @file common.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Shared constants for the HTTP-vs-asio comparison servers
 * @version 0.1.0
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef HTTP_BENCH_COMMON
#  define HTTP_BENCH_COMMON
#  include <string_view>

namespace http_bench {

/// @brief the response payload served by both servers on GET /hello
inline constexpr std::string_view HELLO_BODY = "Hello, World!";
/// @brief the route served by both servers
inline constexpr std::string_view HELLO_PATH = "/hello";

}  // namespace http_bench
#endif
