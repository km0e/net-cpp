/**
 * @file normal.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief UDP DNS resolver
 * @version 0.12
 * @date 2025-07-30
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_APP_DNS_RESOLVER_UDP
#  define XSL_APP_DNS_RESOLVER_UDP
#  include <resolver/def.h>
#  include <xsl/asio.h>
#  include <xsl/feature.h>
#  include <xsl/log.h>
#  include <xsl/net.h>
#  include <xsl/sync.h>
#  include <xsl/sys.h>

#  include <atomic>
#  include <cstdint>
#  include <cstring>
#  include <expected>
#  include <iterator>
#  include <string_view>
#  include <unordered_map>
#  include <utility>
#  include <vector>

using namespace xsl::io;
using namespace xsl::dns;

struct QueryBuf {
  DnsBuf buf = {};
  DnsSig over = {};

  QueryBuf() = default;

  constexpr void header(Header &header) { buf.fill(header.serialize(buf.unfilled())); }
  constexpr void question(Question &q) { buf.fill(q.serialize(buf.unfilled())); }
  std::uint16_t id() const {
    uint16_t u16;
    xsl::deserialize(buf.data(), u16);
    return u16;
  }
};

class UdpResolver : public Resolver {
  using SktTraitsType = sys::net::SocketTraits<Udp<>>;
  using sockaddr_t = sys::net::SockAddr<SktTraitsType>;

public:
  static Expected<std::unique_ptr<UdpResolver>> create(Context &ctx, std::string_view ip,
                                                       uint16_t port) {
    TRV(sa, sys::net::make_sockaddr<SktTraitsType>(ip, port));
    return {std::make_unique<UdpResolver>(ctx, std::move(sa))};
  }
  UdpResolver(Context &ctx, sockaddr_t &&addr) : addr{std::move(addr)}, socket{ctx, addr} {}
  ~UdpResolver() override = default;

  Task<std::expected<std::pair<std::vector<RRView>, DnsBuf>, errc>> query(std::string_view dn,
                                                                          Question &q) override {
    log_info("Querying DNS for domain: {}, type: {}, class: {}", dn, q.type, q.class_);
    if (*dn.rbegin() == '.') {
      dn = dn.substr(0, dn.size() - 1);
    }

    QueryBuf query{};

    Header header{};
    header.id = this->id.fetch_add(1);
    header.flags = 0b0000000100000000;
    header.qdcount = 1;
    query.header(header);

    {
      DnCompressor compressor{query.buf.data()};
      CO_TRV(size, compressor.prepare(dn));
      compressor.compress(query.buf.unfilled());
      query.buf.fill(size);
    }
    query.question(q);
    DnsBuf buf{};

    if (header.id == 0) {
      co_yield this->run();
    }

    recv_wait_list.lock()->emplace(query.id(), std::make_pair(&buf, &query.over));
    co_yield this->socket.sendto(addr, query.buf.data(), query.buf.size());

    if (!co_await query.over) {
      co_return std::unexpected{errc::operation_canceled};
    }

    auto des_span = std::as_bytes(std::span{buf.data(), buf.size()});
    header.deserialize(des_span);  // this will consume the header part
    if (header.rcode() != RCode::NO_ERROR) {
      co_return std::unexpected{header.rcode().to_errc()};
    }

    if (header.qdcount != 1) {  /// FIX:
      co_return std::unexpected{errc::illegal_byte_sequence};
    }

    errc ec = skip_question(des_span);  // this will consume the question part
    if (ec != errc{}) {
      co_return std::unexpected{ec};
    }

    std::vector<RRView> rrs;

    {
      DnDecompressor decompressor{query.buf.data()};
      for (std::size_t i = 0; i < header.ancount; i++) {
        ec = decompressor.decompress(des_span);
        if (ec != errc{}) co_return std::unexpected{ec};

        auto dn_sv = decompressor.dn();
        if (*dn_sv.rbegin() == '.') {
          dn_sv = dn_sv.substr(0, dn_sv.size() - 1);
        }
        log_debug("Decompressed domain name: {}", dn_sv);
        if (dn_sv != dn) {
          continue;
        }

        auto v = RRView(des_span);
        log_info("Received DNS record for domain: {}, type: {}, class: {}, ttl: {}", dn_sv,
                 v.type(), v.class_(), v.ttl());
        rrs.emplace_back(v);
      }
    }
    co_return std::pair{std::move(rrs), std::move(buf)};
  }

  Task<void> run() {
    auto blk = std::make_unique<byte[]>(MAX_SIZE_DNS_UDP);
    while (true) {
      auto res = co_await this->socket.recvfrom(addr, blk.get(), MAX_SIZE_DNS_UDP);
      if (!res) {
        continue;
      }
      uint16_t id;
      xsl::deserialize(blk.get(), id);
      log_debug("Received DNS response with id: {}", id);
      DnsBuf *buf = nullptr;
      DnsSig *sig = nullptr;

      {
        auto lock = this->recv_wait_list.lock();
        auto it = lock->find(id);
        if (it != lock->end()) {
          std::tie(buf, sig) = it->second;
          lock->erase(it);
        }
      }
      if (buf && sig) {
        buf->resize(res.size);
        std::swap(buf->underlying(), blk);
        sig->release();
      }
    }
  }

private:
  sockaddr_t addr;

  ShardRes<std::unordered_map<std::uint16_t, std::pair<DnsBuf *, DnsSig *>>> recv_wait_list = {};
  std::atomic_uint16_t id = 0;

  asio::AsyncSocket<SktTraitsType> socket;
};

#endif
