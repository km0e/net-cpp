/**
 * @file conn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_CONN
#  define XSL_NET_HTTP_CONN
#  include "xsl/ai.h"
#  include "xsl/coro.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"

#  include <cstdint>
#  include <memory>
#  include <span>
#  include <type_traits>
#  include <utility>
XSL_HTTP_NB
namespace impl_conn {
  template <class ABD, class ServiceABD>
  using DeviceCastParams
      = std::integral_constant<std::uint8_t, std::is_same_v<ABD, ServiceABD>
                                                 ? 0
                                                 : (std::is_base_of_v<ABD, ServiceABD> ? 1 : 2)>;
  template <std::uint8_t ABr, std::uint8_t ABw>
  struct DeviceCast;
  template <>
  struct DeviceCast<0, 0> {
    template <class ABr, class ABw, class Service>
    static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
      return std::make_tuple(std::addressof(ard), std::addressof(awd), std::addressof(service));
    }
  };
  // template <>
  // struct DeviceCast<0, 1> {
  //   template <class ABr, class ABw, class Service>
  //   static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
  //   }
  // };
  template <>
  struct DeviceCast<0, 2> {
    template <class ABr, class ABw, class Service>
    static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
      return std::make_tuple(std::addressof(ard), std::move(awd).to_unique_dyn(),
                             std::addressof(service));
    }
  };
  // template <>
  // struct DeviceCast<1, 0> {
  //   template <class ABr, class ABw, class Service>
  //   static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
  //     static_assert(false, "unimplemented");
  //   }
  // };
  // template <>
  // struct DeviceCast<1, 1> {
  //   template <class ABr, class ABw, class Service>
  //   static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
  //     static_assert(false, "unimplemented");
  //   }
  // };
  // template <>
  // struct DeviceCast<1, 2> {
  //   template <class ABr, class ABw, class Service>
  //   static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
  //     static_assert(false, "unimplemented");
  //   }
  // };
  template <>
  struct DeviceCast<2, 0> {
    template <class ABr, class ABw, class Service>
    static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
      return std::make_tuple(std::move(ard).to_unique_dyn(), std::addressof(awd),
                             std::addressof(service));
    }
  };
  // template <>
  // struct DeviceCast<2, 1> {
  //   template <class ABr, class ABw, class Service>
  //   static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
  //     static_assert(false, "unimplemented");
  //   }
  // };
  template <>
  struct DeviceCast<2, 2> {
    template <class ABr, class ABw, class Service>
    static auto cast(ABr&& ard, ABw&& awd, Service&& service) {
      return std::make_tuple(std::move(ard).to_unique_dyn(), std::move(awd).to_unique_dyn(),
                             std::addressof(service));
    }
  };
  template <class ABr, class ABw, class Service>
  using DeviceCastCompose = DeviceCast<DeviceCastParams<ABr, typename Service::abr_type>::value,
                                       DeviceCastParams<ABw, typename Service::abw_type>::value>;
}  // namespace impl_conn

template <class Executor, ai::ABRL ABr, ai::ABWL ABw, class Service,
          class ParserTraits = HttpParseTraits>
Lazy<void, Executor> imm_serve_connection(ABr& ard, ABw& awd, Service& service,
                                          Parser<ParserTraits> parser) {
  ParseData parse_data;
  while (true) {
    {
      LOG5("Start to read request");
      auto res = co_await [&]() {
        if constexpr (std::is_same_v<ABr, ABR>) {
          return parser.read(ard, parse_data);
        } else {
          return parser.template read<Executor>(ard, parse_data);
        }
      }();
      if (!res) {
        LOG3("recv error: {}", std::make_error_code(res.error()).message());
        break;
      }
      LOG5("New request: {} {}", parse_data.request.method, parse_data.request.path);
    }

    Request request{std::move(parse_data.buffer), std::move(parse_data.request),
                    parse_data.content_part, ard};
    auto resp = co_await (service)(std::move(request));
    auto [sz, err] = co_await resp.sendto(awd);
    if (err) {
      LOG3("send error: {}", std::make_error_code(*err).message());
    }
  }
}

template <class Executor, ai::ABRWL ABrw, class Service, class ParserTraits = HttpParseTraits>
Lazy<void, Executor> serve_connection(std::unique_ptr<ABrw> ard, std::shared_ptr<Service> service,
                                      Parser<ParserTraits> parser = Parser<ParserTraits>{}) {
  auto [r, w] = std::move(*ard).split();
  auto [pard, pawd, pservice]
      = impl_conn::DeviceCastCompose<typename ABrw::in_dev_type, typename ABrw::out_dev_type,
                                     Service>::cast(std::move(r), std::move(w), *service);
  co_return co_await imm_serve_connection<Executor>(*pard, *pawd, *pservice, std::move(parser));
}

template <ai::ABRL ABr, ai::ABWL ABw>
class Connection {
  using abr_type = ABr;
  using abw_type = ABw;

public:
  Connection(abr_type&& in_dev, abw_type&& out_dev)
      : _ard(std::move(in_dev)), _awd(std::move(out_dev)), _parser{} {}

  Connection(Connection&&) = default;
  Connection& operator=(Connection&&) = default;
  ~Connection() {}

  template <class Executor = coro::ExecutorBase>
  Task<Result, Executor> read(std::span<Request<abr_type>> reqs) {
    std::size_t i = 0;
    ParseData parse_data;
    for (auto& req : reqs) {
      {
        auto res = co_await _parser.template read<Executor>(this->_ard, parse_data);
        if (!res) {
          if (res.error() != std::errc::no_message) {
            LOG3("recv error: {}", std::make_error_code(res.error()).message());
          }
          co_return Result{i, res.error()};
        }
        LOG4("New request: {} {}", parse_data.request.method, parse_data.request.path);
      }
      req = Request<abr_type>{std::move(parse_data.buffer), std::move(parse_data.request),
                              parse_data.content_part, this->_ard};
      ++i;
    }
    co_return Result{i, std::nullopt};
  }

  Task<Result> read(std::span<Request<abr_type>> reqs) {
    return this->read<coro::ExecutorBase>(reqs);
  }

  template <class Executor = coro::ExecutorBase>
  Task<Result, Executor> write(std::span<const Response<abw_type>> resps) {
    for (auto& resp : resps) {
      auto [sz, err] = co_await resp.sendto(this->_awd);
      if (err) {
        LOG3("send error: {}", std::make_error_code(*err).message());
        co_return Result{0, *err};
      }
    }
    co_return Result{0, std::nullopt};
  }
  template <class Executor = coro::ExecutorBase, class Service>
  Lazy<void, Executor> serve_connection(this Connection self, std::shared_ptr<Service> service) {
    auto [pard, pawd, pservice] = impl_conn::DeviceCastCompose<abr_type, abw_type, Service>::cast(
        std::move(self._ard), std::move(self._awd), *service);
    co_return co_await http::imm_serve_connection<Executor>(*pard, *pawd, *pservice,
                                                            std::move(self._parser));
  }

private:
  abr_type _ard;
  abw_type _awd;

  Parser<HttpParseTraits> _parser;
};
template <ai::ABRWL ABrw>
Connection<typename ABrw::in_dev_type, typename ABrw::out_dev_type> make_connection(ABrw&& dev) {
  auto [r, w] = std::move(dev).split();
  return {std::move(r), std::move(w)};
}
template <class ABrw>
void make_connection(ABrw& dev) = delete;
XSL_HTTP_NE
#endif
