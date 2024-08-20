/**
 * @file server.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_LOCAL
#  define XSL_NET_DNS_LOCAL
#  include "xsl/coro.h"
#  include "xsl/net/dns/def.h"
#  include "xsl/net/dns/proto/header.h"
#  include "xsl/net/dns/utils.h"
#  include "xsl/net/udp/utils.h"
#  include "xsl/sys.h"

#  include <netinet/in.h>
#  include <resolv.h>

#  include <bitset>
#  include <cstddef>
#  include <cstdint>
#  include <expected>
#  include <string>
#  include <string_view>
XSL_NET_DNS_NB

class QueryFlags {
public:
  QueryFlags() : flags(0b0000000100000000) {}
  std::uint16_t get() const { return flags.to_ulong(); }

private:
  std::bitset<16> flags;
};

template <class Ip>
class Server {
public:
  Server(sys::udp::AsyncSocket<Ip> &&socket)
      : socket(std::move(socket)), id(0), buffer(std::make_unique<byte[]>(512)) {}
  Server(Server &&) = default;
  Server &operator=(Server &&) = default;
  ~Server() = default;
  template <class Executor = coro::ExecutorBase>
  Task<std::string, Executor> query(const std::string_view &name, QueryFlags flags = QueryFlags()) {
    std::size_t offset = 0;

    Header header{};
    header.id = this->id++;
    header.flags = flags.get();
    header.qdcount = 1;
    header.serialize(this->buffer.get());
    offset += 12;

    DnCompressor compressor{this->buffer.get()};
    auto status = compressor.prepare(name);
    if (status.invalid_domain_name()) {
      co_return std::string{};
    }
    compressor.compress(std::span{this->buffer.get() + offset, status.size()});
    offset += status.size();
    serialize(ExtQType::ANY, this->buffer.get() + offset);
    offset += 2;
    serialize(ExtQClass::ANY, this->buffer.get() + offset);
    offset += 2;

    auto [w_sz, w_err]
        = co_await this->socket.template write<Executor>(std::span{this->buffer.get(), offset});
    if (w_err) {
      co_return std::string{};
    }

    auto [r_sz, r_err]
        = co_await this->socket.template read<Executor>(std::span{this->buffer.get(), 512});
    if (r_err) {
      co_return std::string{};
    }

    co_return std::string{reinterpret_cast<const char *>(this->buffer.get()), r_sz};
  }

private:
  sys::udp::AsyncSocket<Ip> socket;
  std::uint16_t id;
  std::unique_ptr<byte[]> buffer;
};

template <class Ip>
std::expected<Server<Ip>, std::error_condition> serv(const std::string_view &ip,
                                                     const std::string_view &port, Poller &poller) {
  auto socket = udp::dial<Ip>(ip.data(), port.data());
  if (!socket) {
    return std::unexpected{socket.error()};
  }
  return Server<Ip>{std::move(*socket).async(poller)};
}

XSL_NET_DNS_NE

#endif
