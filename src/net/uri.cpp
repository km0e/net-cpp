/**
 * @file uri.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1.1
 * @date 2025-06-14
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <xsl/net/def.h>
#include <xsl/net/uri.h>
XSL_NET_NB
std::expected<std::string_view, std::errc> percent_decode(std::string& buffer,
                                                          std::string_view sv) noexcept {
  buffer.reserve(buffer.size() + sv.size());
  std::size_t initial_size = buffer.size();
  while (true) {
    auto percent_pos = sv.find('%');
    if (percent_pos == std::string_view::npos) {
      break;
    }
    buffer.append(sv.substr(0, percent_pos));
    if (sv.size() < percent_pos + 3) {
      return std::unexpected{std::errc::illegal_byte_sequence};
    }
    int value = 0;
    auto res = std::from_chars(sv.data() + percent_pos + 1, sv.data() + percent_pos + 3, value, 16);
    if (res.ec != std::errc()) {
      return std::unexpected{std::errc::illegal_byte_sequence};
    }
    if (value < 0 || value > 255) {
      return std::unexpected{std::errc::illegal_byte_sequence};
    }
    buffer.push_back(static_cast<char>(value));
    sv = sv.substr(percent_pos + 3);
  }
  if (buffer.size() == initial_size) {
    return {std::string_view{}};  // No percent-encoded characters found
  } else {
    buffer.append(sv);
    return {std::string_view{buffer.data() + initial_size, buffer.size() - initial_size}};
  }
}
const std::regex KVQuery::regex_re(KVQuery::regex_str.data(), KVQuery::regex_str.size());
const std::regex AbsoluteUri::regex_re(AbsoluteUri::regex_str.data(),
                                       AbsoluteUri::regex_str.size());
XSL_NET_NE
