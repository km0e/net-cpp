/**
 * @file resolver.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief DNS resolver
 * @version 0.11
 * @date 2024-09-09
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_DNS_CLIENT
#  define XSL_NET_DNS_CLIENT
#  include "xsl/coro.h"
#  include "xsl/feature.h"
#  include "xsl/net/dns/cache.h"
#  include "xsl/net/dns/def.h"
#  include "xsl/net/dns/proto/def.h"
#  include "xsl/net/dns/proto/header.h"
#  include "xsl/net/dns/proto/question.h"
#  include "xsl/net/dns/proto/rr.h"
#  include "xsl/net/dns/utils.h"
#  include "xsl/sync.h"
#  include "xsl/sys.h"

#  include <atomic>
#  include <concepts>
#  include <cstdint>
#  include <cstring>
#  include <expected>
#  include <forward_list>
#  include <iterator>
#  include <memory>
#  include <string_view>
#  include <unordered_map>
#  include <utility>
#  include <vector>
XSL_NET_DNS_NB
using namespace xsl::io;

struct Query {
  Block datagram;
  Signal<1> over;

  std::uint16_t id() const {
    uint16_t u16;
    xsl::deserialize(datagram.data.get(), u16);
    return u16;
  }
};

template <class LowerLayer>
class ResolverImpl {
  using sockaddr_t = sys::net::SockAddrCompose<Udp<LowerLayer>>;
public:
  ResolverImpl(Poller &poller,
               std::vector<sys::net::SockAddrCompose<Udp<LowerLayer>>> &&dns_servers)
      : init_dns_servers{std::move(dns_servers)},
        send_wait_list{},
        send_signal{},
        recv_wait_list{},
        id{0},
        socket{poller},
        cache{} {}
  ~ResolverImpl() = default;
  Task<std::expected<const std::forward_list<RR> *, errc>> query(std::string_view dn, Type type,
                                                                 Class class_) {
    if (*dn.rbegin() == '.') {
      dn = dn.substr(0, dn.size() - 1);
    }

    if (auto rrs = cache.get(dn); rrs) {
      DEBUG("cache hit");
      co_return rrs;
    }
    for (auto &sa : init_dns_servers) {
      auto rrs = co_await query(sa, dn, type, class_);
      if (rrs) {
        co_return rrs;
      }
    }
  }

  Task<std::expected<const std::forward_list<RR> *, errc>> query(sys::net::SockAddrCompose<Udp<LowerLayer>> &sa,
                                                                 std::string_view dn, Type type, Class class_) {
    Query query{{512}, {}};

    auto ser_span = query.datagram.span();

    Header header{};
    header.id = this->id.fetch_add(1);
    header.flags = 0b0000000100000000;
    header.qdcount = 1;
    header.serialize(ser_span);
    // offset += 12;/
    {
      DnCompressor compressor{query.datagram.data.get()};

      auto ec = serialized(ser_span, dn, type, class_, compressor);
      if (ec != errc{}) {
        co_return std::unexpected{ec};
      }
    }

    query.datagram.valid_size = 512 - ser_span.size_bytes();

    this->send_wait_list.lock()->emplace_front(&query, &sa);
    this->send_signal.release();

    if (!co_await query.over) {
      co_return std::unexpected{errc::operation_canceled};
    }

    auto des_span = xsl::as_bytes(query.datagram.span());
    header.deserialize(des_span);  // this will consume the header part
    if (header.rcode() != RCode::NO_ERROR) {
      co_return std::unexpected{header.rcode().to_errc()};
    }

    if (header.qdcount != 1) {
      co_return std::unexpected{errc::illegal_byte_sequence};
    }

    errc ec = skip_question(des_span);  // this will consume the question part
    if (ec != errc{}) {
      co_return std::unexpected{ec};
    }

    std::forward_list<RR> rrs;

    {
      DnDecompressor decompressor{query.datagram.data.get()};
      for (std::size_t i = 0; i < header.ancount; i++) {
        ec = decompressor.decompress(des_span);
        // auto res_rr = deserialized(des_span, decompressor);
        if (ec != errc{}) {
          co_return std::unexpected{ec};
        }
        auto dn_sv = decompressor.dn();
        if (*dn_sv.rbegin() == '.') {
          dn_sv = dn_sv.substr(0, dn_sv.size() - 1);
        }
        if (dn_sv != dn) {
          continue;
        }

        RR rr = RR::from_bytes(des_span);
        if (!rr.is_valid()) {
          co_return std::unexpected{errc::result_out_of_range};
        }
        rrs.emplace_front(std::move(rr));
      }
      if (!rrs.empty()) {
        co_return cache.insert(dn, std::move(rrs));
      }
      // std::forward_list<std::string> ns{};
      // for (std::size_t i = 0; i < header.nscount; i++) {
      //   auto res_rr = deserialized(des_span, decompressor);
      //   if (!res_rr) {
      //     co_return std::unexpected{res_rr.error()};
      //   }
      //   auto [rr_name, rr] = std::move(*res_rr);
      //   auto ans_sv = std::string_view{rr_name};
      //   if (*ans_sv.rbegin() == '.') {
      //     ans_sv = ans_sv.substr(0, ans_sv.size() - 1);
      //   }
      //   if (ans_sv != dn) {
      //     continue;
      //   }
      //   rrs.emplace_front(std::move(rr));
      // }
    }
    co_return cache.insert(dn, std::move(rrs));
  }

  Task<void> run() {
    co_yield [&] -> Task<void> {
      do {
        auto dgs = std::exchange(*this->send_wait_list.lock(), {});
        for (auto [query,sa] : dgs) {
          this->recv_wait_list.lock()->emplace(query->id(), query);
          // co_await this->socket.send(query->datagram.span(0));
          co_await this->socket.sendto(query->datagram.span(0), *sa);
        }
      } while (co_await this->send_signal);
    }();
    co_yield [&] -> Task<void> {
      Block block{512};
      while (true) {
        auto [r_sz, r_err] = co_await this->socket.recv(block.span(0));
        if (r_err) {
          continue;
        }
        uint16_t id;
        xsl::deserialize(block.data.get(), id);
        Query *query = nullptr;
        {
          auto lock = this->recv_wait_list.lock();
          auto it = lock->find(id);
          if (it != lock->end()) {
            query = it->second;
            lock->erase(it);
          }
        }
        if (query) {
          block.valid_size = r_sz;
          std::swap(query->datagram, block);
          query->over.release();
          block.valid_size = 512;  // reset the block
        }
      }
    }();
  }

private:
  std::vector<sockaddr_t> init_dns_servers;

  ShardRes<std::forward_list<std::pair<Query *,sockaddr_t*>>> send_wait_list;
  Signal<> send_signal;

  ShardRes<std::unordered_map<std::uint16_t, Query *>> recv_wait_list;
  std::atomic_uint16_t id;

  sys::udp::AsyncSocket<LowerLayer> socket;

  MemoryCache cache;
};

template <class LowerLayer = Ip<4>>
class Resolver {
public:
  template <std::input_iterator _InIt>
    requires std::same_as<std::iter_value_t<_InIt>, const char *>
  Resolver(Poller &poller, _InIt first, _InIt last) : impl{nullptr} {
    std::vector<sys::net::SockAddrCompose<Udp<LowerLayer>>> init_dns_servers;
    for (auto it = first; it != last; it++) {
      const char *pos = std::strchr(*it, ':');
      std::uint16_t port = 53;
      if (pos) {
        port = std::stoul(pos + 1);
      }
      init_dns_servers.emplace_back(*it, port);
    };
    impl = std::make_unique<ResolverImpl<LowerLayer>>(poller, std::move(init_dns_servers));
  }
  Resolver(Poller &poller, sys::net::SockAddrCompose<Udp<LowerLayer>> &&sa) : impl{nullptr} {
    std::vector<sys::net::SockAddrCompose<Udp<LowerLayer>>> init_dns_servers;
    init_dns_servers.emplace_back(std::move(sa));
    impl = std::make_unique<ResolverImpl<LowerLayer>>(poller, std::move(init_dns_servers));
  }
  Resolver(Resolver &&) = default;
  Resolver &operator=(Resolver &&) = default;
  ~Resolver() = default;
  /**
   * @brief query the domain name
   *
   * @param dn domain name
   * @return Task<std::expected<const std::forward_list<RR> *, errc>>
   */
  decltype(auto) query(const std::string_view &dn, Type type = Type::A, Class class_ = Class::IN) {
    return this->impl->query(dn, type, class_);
  }
  /// @brief run the resolver
  Task<void> run() { return this->impl->run(); }

protected:
  std::unique_ptr<ResolverImpl<LowerLayer>> impl;
};

using Ipv4Resolver = Resolver<Ip<4>>;
using Ipv6Resolver = Resolver<Ip<6>>;

XSL_NET_DNS_NE
#endif
