/**
 * @file question.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Dns question structure
 * @version 0.1.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_PROTO_QUESTION
#  define XSL_NET_DNS_PROTO_QUESTION
#  include <xsl/net/dns/proto/def.h>
#  include <xsl/net/dns/utils.h>

XSL_NET_DNS_NB
/**
 * @brief DNS question structure
 * @see https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.2
 */
struct Question {
  static constexpr std::size_t SIZE = 4;  ///< size of the question (type + class)

  Type type = Type::ANY;      ///< type of the question
  Class class_ = Class::ANY;  ///< class of the question

  constexpr Question() = default;
  constexpr Question(Type type, Class class_) : type(type), class_(class_) {}
  constexpr Question(const Question &) = default;
  constexpr Question &operator=(const Question &) = default;

  constexpr void serialize(std::span<byte> &buf) const {
    type.serialized(buf);
    class_.serialized(buf);
  }
  constexpr std::size_t serialize(byte *buf) const {
    return type.serialize(buf) + class_.serialize(buf + type.SIZE);
  }
  constexpr std::size_t deserialize(const byte *buf) {
    std::size_t offset = 0;
    offset += type.deserialize(buf + offset);
    offset += class_.deserialize(buf + offset);
    return offset;
  }
};

/// @brief skip the question part
constexpr errc skip_question(std::span<const byte> &src) {
  auto ec = skip_dn(src);
  if (ec != errc{}) {
    return ec;
  }
  src = src.subspan(4);
  return {};
}

XSL_NET_DNS_NE
#endif
