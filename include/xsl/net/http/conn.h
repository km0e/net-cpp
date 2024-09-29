/**
 * @file conn.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Connection class for HTTP server
 * @version 0.11
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_NET_HTTP_CONN
#  define XSL_NET_HTTP_CONN
#  include "xsl/coro.h"
#  include "xsl/coro/guard.h"
#  include "xsl/dyn.h"
#  include "xsl/io.h"
#  include "xsl/io/byte.h"
#  include "xsl/logctl.h"
#  include "xsl/net/http/def.h"
#  include "xsl/net/http/msg.h"
#  include "xsl/net/http/parse.h"

#  include <memory>
#  include <span>
#  include <type_traits>
#  include <utility>
XSL_HTTP_NB
using namespace xsl::io;
/**
 * @brief serve the connection
 *
 * @tparam ABr the async byte reader
 * @tparam ABO the async byte writer
 * @tparam Service the service type
 * @tparam ParserTraits the parser traits
 * @param ard
 * @param awd
 * @param service the service
 * @param parser the parser
 * @return Task<void>
 * @note this function would not save the state of the arguments, do not directly call it
 */
template <ABILike ABI, ABOLike ABO, class Service, class ParserTraits = HttpParseTraits>
Task<void> imm_serve_connection(ABI& ard, ABO& awd, Service& service,
                                Parser<ParserTraits>& parser) {
  ParseData parse_data{};
  while (true) {
    {
      LOG5("Start to read request");
      auto res = co_await parser.read(ard, parse_data);
      if (res != errc{}) {
        LOG3("recv error: {}", std::make_error_code(res).message());
        break;
      }
      LOG5("New request: {} {}", parse_data.request.method, parse_data.request.path);
    }

    Request request{std::move(parse_data.buffer), std::move(parse_data.request),
                    parse_data.content_part, ard};
    DEBUG("ready to serve request: {}", request.view.path);
    Response<ABO> resp = co_await (service)(
        std::move(request));  // TODO: may be will also need to be a coroutine in the future
    DEBUG("ready to send response: {}", static_cast<uint16_t>(resp._part.status_code));
    auto [sz, err] = co_await resp.sendto(awd);
    if (err) {
      LOG3("send error: {}", std::make_error_code(*err).message());
    }
  }
}

template <ABILike ABI, ABOLike ABO, class Service, class ParserTraits = HttpParseTraits>
decltype(auto) serve_connection(ABI&& _abr, ABO&& _abw, std::shared_ptr<Service>&& service,
                                Parser<ParserTraits> parser = Parser<ParserTraits>{}) {
  using l_in_dev_type = std::decay_t<ABI>;
  using l_out_dev_type = std::decay_t<ABO>;
  using r_in_dev_type = typename Service::in_dev_type;
  using r_out_dev_type = typename Service::out_dev_type;
  static_assert(dyn_cast_v<IODynGetChain, l_in_dev_type, r_in_dev_type>,
                "Input device type mismatched with the service");
  static_assert(dyn_cast_v<IODynGetChain, l_out_dev_type, r_out_dev_type>,
                "Output device type mismatched with the service");
  using final_in_dev_type
      = std::conditional_t<std::is_same_v<l_in_dev_type, r_in_dev_type>, l_in_dev_type,
                           dyn_cast_t<typename IODynGetChain<l_in_dev_type>::type, r_in_dev_type>>;
  using final_out_dev_type = std::conditional_t<
      std::is_same_v<l_out_dev_type, r_out_dev_type>, l_out_dev_type,
      dyn_cast_t<typename IODynGetChain<l_out_dev_type>::type, r_out_dev_type>>;
  auto fn = [](r_in_dev_type& _abr, r_out_dev_type& _abw, auto& _service, auto& _parser) {
    return imm_serve_connection(_abr, _abw, *_service, _parser);
  };
  return _coro::ArgGuard<decltype(fn), final_in_dev_type, final_out_dev_type,
                                std::shared_ptr<Service>, Parser<ParserTraits>>{
      fn, std::forward<ABI>(_abr), std::forward<ABO>(_abw), std::move(service), std::move(parser)};
}
/**
 * @brief serve the connection
 *
 * @tparam ABIO the async byte reader and writer
 * @tparam Service the service type
 * @tparam ParserTraits the parser traits
 * @param ard
 * @param service
 * @param parser
 * @return Task<void>
 */
template <ABIOLike ABIO, class Service, class ParserTraits = HttpParseTraits>
decltype(auto) serve_connection(ABIO&& _ab, std::shared_ptr<Service>&& service,
                                Parser<ParserTraits> parser = Parser<ParserTraits>{}) {
  using l_io_dev_type = std::decay_t<ABIO>;
  using l_in_dev_type = typename l_io_dev_type::in_dev_type;
  using l_out_dev_type = typename l_io_dev_type::out_dev_type;
  using r_in_dev_type = typename Service::in_dev_type;
  using r_out_dev_type = typename Service::out_dev_type;
  static_assert(dyn_cast_v<IODynGetChain, l_in_dev_type, r_in_dev_type>,
                "Input device type mismatched with the service");
  static_assert(dyn_cast_v<IODynGetChain, l_out_dev_type, r_out_dev_type>,
                "Output device type mismatched with the service");
  if constexpr (std::is_same_v<l_io_dev_type, r_in_dev_type>
                && std::is_same_v<l_io_dev_type, r_out_dev_type>) {
    return _coro::ArgGuard(
        [](auto& _ab, auto& _service, auto& _parser) {
          return imm_serve_connection(_ab, _ab, *_service, _parser);
        },
        std::move(_ab), std::move(service), std::move(parser));
  } else if constexpr (requires { _ab.split(); }) {
    auto [r, w] = std::move(_ab).split();
    return serve_connection(std::move(r), std::move(w), std::move(service), std::move(parser));
  }
}

template <ABIOLike ABIO, class Service, class ParserTraits = HttpParseTraits>
decltype(auto) serve_connection(std::unique_ptr<ABIO>&& _ab, std::shared_ptr<Service> service,
                                Parser<ParserTraits> parser = Parser<ParserTraits>{}) {
  using l_io_dev_type = std::decay_t<ABIO>;
  using l_in_dev_type = AIOTraits<l_io_dev_type>::in_dev_type;
  using l_out_dev_type = AIOTraits<l_io_dev_type>::out_dev_type;
  using r_in_dev_type = typename Service::in_dev_type;
  using r_out_dev_type = typename Service::out_dev_type;
  static_assert(dyn_cast_v<IODynGetChain, l_in_dev_type, r_in_dev_type>,
                "Input device type mismatched with the service");
  static_assert(dyn_cast_v<IODynGetChain, l_out_dev_type, r_out_dev_type>,
                "Output device type mismatched with the service");
  if constexpr (std::is_same_v<l_io_dev_type, r_in_dev_type>
                && std::is_same_v<l_io_dev_type, r_out_dev_type>) {
    return _coro::ArgGuard(
        [](auto& _ab, auto& _service, auto& _parser) {
          return imm_serve_connection(*_ab, *_ab, *_service, _parser);
        },
        std::move(_ab), std::move(service), std::move(parser));
  } else if constexpr (requires { std::move(*_ab).split(); }) {
    auto [r, w] = std::move(*_ab).split();
    return serve_connection(std::move(r), std::move(w), std::move(service), std::move(parser));
  }
}

class ConnectionBase {
public:
  ConnectionBase() : _parser{} {}
  template <class _Conn>
  Task<Result> read(this _Conn& self, std::span<Request<typename _Conn::in_dev_type>> reqs) {
    std::size_t i = 0;
    ParseData parse_data;
    for (auto& req : reqs) {
      {
        auto res = co_await self._parser.read(self.read_dev(), parse_data);
        if (!res) {
          if (res.error() != errc::no_message) {
            LOG3("recv error: {}", std::make_error_code(res.error()).message());
          }
          co_return Result{i, res.error()};
        }
        LOG4("New request: {} {}", parse_data.request.method, parse_data.request.path);
      }
      req = Request<typename _Conn::in_dev_type>{std::move(parse_data.buffer),
                                                 std::move(parse_data.request),
                                                 parse_data.content_part, self.read_dev()};
      ++i;
    }
    co_return Result{i, std::nullopt};
  }

  template <class _Conn>
  Task<Result> write(this _Conn& self,
                     std::span<const Response<typename _Conn::out_dev_type>> resps) {
    for (auto& resp : resps) {
      auto [sz, err] = co_await resp.sendto(self.write_dev());
      if (err) {
        LOG3("send error: {}", std::make_error_code(*err).message());
        co_return Result{0, *err};
      }
    }
    co_return Result{0, std::nullopt};
  }

protected:
  Parser<HttpParseTraits> _parser;
};

template <class... IO>
class Connection;

template <ABIOLike ABIO>
class Connection<ABIO> : public ConnectionBase {
  using Base = ConnectionBase;
  using io_dev_type = ABIO;
  using abio_traits_type = AIOTraits<io_dev_type>;
  using in_dev_type = typename abio_traits_type::in_dev_type;
  using out_dev_type = typename abio_traits_type::out_dev_type;

public:
  constexpr Connection(io_dev_type&& ab) : Base{}, _ab(std::move(ab)) {}

  constexpr Connection(Connection&&) = default;
  constexpr Connection& operator=(Connection&&) = default;
  ~Connection() {}

  io_dev_type& read_dev() { return this->_ab; }
  io_dev_type& write_dev() { return this->_ab; }

  template <class Service>
  decltype(auto) serve_connection(this Connection&& self, std::shared_ptr<Service> service) {
    return http::serve_connection(std::move(self._ab), std::move(service), std::move(self._parser));
  }

private:
  io_dev_type _ab;
};

/// @brief the connection class
template <ABILike ABI, ABOLike ABO>
class Connection<ABI, ABO> : public ConnectionBase {
  using Base = ConnectionBase;
  using in_dev_type = ABI;
  using out_dev_type = ABO;
  using abi_traits_type = AIOTraits<in_dev_type>;
  using abo_traits_type = AIOTraits<out_dev_type>;

public:
  constexpr Connection(in_dev_type&& in_dev, out_dev_type&& out_dev)
      : Base{}, _ard(std::move(in_dev)), _awd(std::move(out_dev)) {}

  constexpr Connection(Connection&&) = default;
  constexpr Connection& operator=(Connection&&) = default;
  ~Connection() {}

  in_dev_type& read_dev() { return this->_ard; }
  out_dev_type& write_dev() { return this->_awd; }

  template <class Service>
  decltype(auto) serve_connection(this Connection&& self, std::shared_ptr<Service> service) {
    return http::serve_connection(std::move(self._ard), std::move(self._awd), std::move(service),
                                  std::move(self._parser));
  }

private:
  in_dev_type _ard;
  out_dev_type _awd;
};

template <ABILike ABI, ABOLike ABO>
Connection(ABI&&, ABO&&) -> Connection<ABI, ABO>;

XSL_HTTP_NE
#endif
