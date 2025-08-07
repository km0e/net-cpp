#pragma once
#ifndef XSL_APP_DNS_LISTENER_UDP_H
#  define XSL_APP_DNS_LISTENER_UDP_H

#  include <listener/def.h>
#  include <xsl/asio.h>
#  include <xsl/def.h>
#  include <xsl/io.h>
#  include <xsl/macro.h>
#  include <xsl/wheel.h>

#  include <cstring>

//clang-format off
#  include <dns.h>
//clang-format on

using namespace xsl;
using namespace xsl::dns;
using namespace xsl::coro;
using namespace xsl::net;

struct DnsQuery {
  FixedBuffer<MAX_SIZE_DNS_UDP> buf = {};
  sys::net::SockAddrCompose<Udp<>> addr = {};

  // constexpr void header(Header &header) { buf.fill(header.serialize(buf.unfilled())); }
  // constexpr void question(Question &q) { buf.fill(q.serialize(buf.unfilled())); }
  // std::uint16_t id() const {
  //   uint16_t u16;
  //   xsl::deserialize(buf.data(), u16);
  //   return u16;
  // }
};

class UdpListener : public Listener {
  using SktTraitsType = sys::net::SocketTraits<Udp<>>;
  using SktAddrType = sys::net::SockAddr<SktTraitsType>;

public:
  static expected<UdpListener, error_condition> create(Context &poller, char *addr) {
    auto pos = static_cast<char *>(std::memchr(addr, ':', strlen(addr)));
    ENSURE(pos != nullptr, make_error_condition(errc::invalid_argument,
                                                "Invalid address format, expected <ip>:<port>"));
    *pos = '\0';                           // Split the address into IP and port
    Defer defer([pos]() { *pos = ':'; });  // Restore the original address format

    TRV(make_sockaddr<SktTraitsType>(addr, pos + 1), sa,
        make_ec_gen(
            std::format("Failed to create DNS server socket address from {}:{}", addr, pos + 1)));
    asio::AsyncSocket<SktTraitsType> socket(poller, sa);
    HNSURE(socket.bind(sa), ([addr, pos](errc e) {
             return make_error_condition(
                 e, std::format("Failed to bind DNS server to {}:{}", addr, pos + 1));
           }));
    return expected<UdpListener, error_condition>{std::in_place_t{}, std::move(socket),
                                                  std::string(addr), std::string(pos + 1)};
  }
  UdpListener(asio::AsyncSocket<SktTraitsType> &&socket, std::string &&ip, std::string &&port)
      : ip(std::move(ip)), port(std::move(port)), socket(std::move(socket)) {}
  ~UdpListener() override = default;
  Task<void> run(DnsCore &core) override {
    std::string addr;
    while (true) {
      DnsQuery q;
      log_debug("Waiting for DNS query...");
      auto res = co_await this->socket.recvfrom(q.addr, q.buf.data(), dns::MAX_SIZE_DNS_UDP);
      if (!res) {
        log_error("Failed to receive DNS query: {}", res.message());
        continue;
      }
      q.buf.resize(res.size);
      q.addr.to_string(addr);
      log_info("Received DNS query from {}", addr);
      co_yield this->handle(std::move(q), core);
    }
  }
  Task<void> handle(DnsQuery query, DnsCore &core) {
    HeaderView header(query.buf.data());
    if (header.qdcount() == 0) {
      log_error("No question in DNS query");
      co_return;
    }
    const byte *ptr = query.buf.data() + Header::SIZE;

    std::size_t ancount = 0;
    DnDecompressor decompressor{ptr};
    DnCompressor compressor{query.buf.data()};
    std::vector<std::pair<std::string, Question>> questions;
    for (std::size_t i = 0; i < header.qdcount(); i++) {
      auto res = decompressor.decompress(ptr);
      if (!res) {
        log_error("Failed to decompress DNS query: {}", to_string(res.error()));
        co_return;
      }
      auto dn = decompressor.dn();
      log_debug("Decompressed domain name: {}", dn);
      if (*res > 2) {  // if the domain name is not completely compressed
        compressor.add_dnptr(ptr);
      }
      ptr += *res;
      Question q;
      ptr += q.deserialize(ptr);
      questions.emplace_back(dn, q);
    }
    query.buf.resize(ptr - query.buf.data());  // Resize the buffer to the end of questions
    for (auto [dn, q] : questions) {
      auto rr = co_await core.get(dn, q);  // check cache first

      if (!rr) {
        log_debug("Failed to resolve domain: {}, type: {}, class: {}, error: {}", dn, q.type,
                  q.class_, rr.error().message());
        continue;
      }
      auto [rrs, buf] = std::move(*rr);
      for (const auto &rr : rrs) {
        log_debug("Resolved domain: {}, type: {}, class: {}, count: {}", dn, q.type, q.class_,
                  rrs.size());
        flush_log();
        auto sz = *compressor.prepare(dn);  // Prepare the domain name for compression
        compressor.compress(query.buf.unfilled());
        query.buf.fill(sz);
        memcpy(query.buf.unfilled(), rr.data(), rr.size());
        query.buf.fill(rr.size());
      }
      ancount += rrs.size();
    }
    header.ancount(ancount);
    header.arcount(0);  // No additional records in response
    header.basic_response();
    co_await this->socket.sendto(query.addr, query.buf.data(), query.buf.size());
    co_return;
  }

private:
  std::string ip, port;  ///< IP and port of the DNS server
  asio::AsyncSocket<SktTraitsType> socket;
};

#endif
