/**
 * @file conn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Connection class for HTTP server
 * @version 0.11s
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

#  include <memory>
#  include <span>
#  include <type_traits>
#  include <utility>
XSL_HTTP_NB
namespace impl_conn {

  template <bool RR2L, bool RL2R, bool WR2L, bool WL2R>
  struct BaseCast {
    static_assert(false, "unimplemented");
  };

  template <>
  struct BaseCast<false, true, false, true> {
    template <class LABr, class RABr, class LABw, class RABw, class T, class U, class V>
    static auto cast(T&& ard, U&& awd, V&& service) {
      return std::make_tuple(std::move(ard).template to_unique_dyn<RABr>(),
                             std::move(awd).template to_unique_dyn<RABw>(),
                             std::addressof(service));
    }
  };
  template <>
  struct BaseCast<false, true, true, true> {
    template <class LABr, class RABr, class LABw, class RABw, class T, class U, class V>
    static auto cast(T&& ard, U&& awd, V&& service) {
      return std::make_tuple(std::move(ard).template to_unique_dyn<RABr>(), std::addressof(awd),
                             std::addressof(service));
    }
  };
  template <>
  struct BaseCast<true, true, false, true> {
    template <class LABr, class RABr, class LABw, class RABw, class T, class U, class V>
    static auto cast(T&& ard, U&& awd, V&& service) {
      return std::make_tuple(std::addressof(ard), std::move(awd).template to_unique_dyn<RABw>(),
                             std::addressof(service));
    }
  };
  template <>
  struct BaseCast<true, true, true, true> {
    template <class LABr, class RABr, class LABw, class RABw, class T, class U, class V>
    static auto cast(T&& ard, U&& awd, V&& service) {
      return std::make_tuple(std::addressof(ard), std::addressof(awd), std::addressof(service));
    }
  };
  template <class...>
  struct BaseCastCompose;

  template <class Abr, class Abw, class Service>
  struct BaseCastCompose<Abr, Abw, Service> {
    template <class T, class U, class V>
    static auto cast(T&& ard, U&& awd, V&& service) {
      using labr_type = typename Abr::dynamic_type;
      using labw_type = typename Abw::dynamic_type;
      using rabr_type = typename Service::abr_type::dynamic_type;
      using rabw_type = typename Service::abw_type::dynamic_type;
      return BaseCast<std::is_base_of_v<labr_type, rabr_type>,
                      std::is_base_of_v<rabr_type, labr_type>,
                      std::is_base_of_v<labw_type, rabw_type>,
                      std::is_base_of_v<rabw_type, labw_type>>::template cast<labr_type, rabr_type,
                                                                              labw_type, rabw_type>(
          std::forward<T>(ard), std::forward<U>(awd), std::forward<V>(service));
    }
  };
  template <class ABrw, class Service>
  struct BaseCastCompose<ABrw, Service> {
    template <class T, class U, class V>
    static auto cast(T&& ard, U&& awd, V&& service) {
      using labr_type = typename ABrw::template rebind<In>::dynamic_type;
      using labw_type = typename ABrw::template rebind<Out>::dynamic_type;
      using rabr_type = typename Service::abr_type::dynamic_type;
      using rabw_type = typename Service::abw_type::dynamic_type;
      return BaseCast<std::is_base_of_v<labr_type, rabr_type>,
                      std::is_base_of_v<rabr_type, labr_type>,
                      std::is_base_of_v<labw_type, rabw_type>,
                      std::is_base_of_v<rabw_type, labw_type>>::template cast<labr_type, rabr_type,
                                                                              labw_type, rabw_type>(
          std::forward<T>(ard), std::forward<U>(awd), std::forward<V>(service));
    }
  };
}  // namespace impl_conn

template <class Executor = coro::ExecutorBase, ai::ABRL ABr, ai::ABWL ABw, class Service,
          class ParserTraits = HttpParseTraits>
Task<void, Executor> imm_serve_connection(ABr& ard, ABw& awd, Service& service,
                                          Parser<ParserTraits> parser) {
  ParseData parse_data;
  while (true) {
    {
      LOG5("Start to read request");
      auto res = co_await ai::read_poly_resolve<Executor>(parser, ard, parse_data);
      if (res != std::errc{}) {
        LOG3("recv error: {}", std::make_error_code(res).message());
        break;
      }
      LOG5("New request: {} {}", parse_data.request.method, parse_data.request.path);
    }

    Request request{std::move(parse_data.buffer), std::move(parse_data.request),
                    parse_data.content_part, ard};
    DEBUG("ready to serve request: {}", request.view.path);
    Response<ABw> resp = co_await (service)(
        std::move(request));  // TODO: may be will also need to be a coroutine in the future
    DEBUG("ready to send response: {}", static_cast<uint16_t>(resp._part.status_code));
    auto [sz, err] = co_await resp.sendto(awd);
    if (err) {
      LOG3("send error: {}", std::make_error_code(*err).message());
    }
  }
}

template <class Executor, ai::ABRWL ABrw, class Service, class ParserTraits = HttpParseTraits>
Task<void, Executor> serve_connection(std::unique_ptr<ABrw> ard, std::shared_ptr<Service> service,
                                      Parser<ParserTraits> parser = Parser<ParserTraits>{}) {
  auto [r, w] = std::move(*ard).split();
  auto [pard, pawd, pservice]
      = impl_conn::BaseCastCompose<ABrw, Service>::cast(std::move(r), std::move(w), *service);
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
  Task<void, Executor> serve_connection(this Connection self, std::shared_ptr<Service> service) {
    auto [pard, pawd, pservice] = impl_conn::BaseCastCompose<abr_type, abw_type, Service>::cast(
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
Connection<typename ABrw::template rebind<In>, typename ABrw::template rebind<Out>> make_connection(
    ABrw&& dev) {
  auto [r, w] = std::move(dev).split();
  return {std::move(r), std::move(w)};
}
template <class ABrw>
void make_connection(ABrw& dev) = delete;
XSL_HTTP_NE
#endif
