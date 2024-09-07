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
#  include "xsl/dyn.h"
#  include "xsl/io.h"
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

namespace {
  template <class T, class U, class V>
  static constexpr auto io_dev_align(T&& ard, U&& awd, V&& service) {
    using r_dev_type = std::decay_t<V>;
    using l_in_dev_type = std::decay_t<T>;
    using l_out_dev_type = std::decay_t<U>;
    using r_in_dev_type = typename r_dev_type::in_dev_type;
    using r_out_dev_type = typename r_dev_type::out_dev_type;
    static_assert(dyn_cast_v<IODynGetChain, l_in_dev_type, r_in_dev_type>,
                  "Input device type mismatched with the service");
    static_assert(dyn_cast_v<IODynGetChain, l_out_dev_type, r_out_dev_type>,
                  "Output device type mismatched with the service");
    return std::make_tuple(to_dyn_unique<IODynGetChain, r_in_dev_type>(std::forward<T>(ard)),
                           to_dyn_unique<IODynGetChain, r_out_dev_type>(std::forward<U>(awd)),
                           std::addressof(service));
  }
}  // namespace
/**
 * @brief serve the connection
 *
 * @tparam ABr the async byte reader
 * @tparam ABw the async byte writer
 * @tparam Service the service type
 * @tparam ParserTraits the parser traits
 * @param ard
 * @param awd
 * @param service the service
 * @param parser the parser
 * @return Task<void>
 * @note this function would not save the state of the arguments, do not directly call it
 */
template <ABRL ABr, ABWL ABw, class Service, class ParserTraits = HttpParseTraits>
Task<void> imm_serve_connection(ABr& ard, ABw& awd, Service& service, Parser<ParserTraits> parser) {
  ParseData parse_data{};
  while (true) {
    {
      LOG5("Start to read request");
      auto res = co_await parser.read(ard, parse_data);
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
/**
 * @brief serve the connection
 *
 * @tparam ABrw the async byte reader and writer
 * @tparam Service the service type
 * @tparam ParserTraits the parser traits
 * @param ard
 * @param service
 * @param parser
 * @return Task<void>
 */
template <ABRWL ABrw, class Service, class ParserTraits = HttpParseTraits>
Task<void> serve_connection(std::unique_ptr<ABrw> ard, std::shared_ptr<Service> service,
                            Parser<ParserTraits> parser = Parser<ParserTraits>{}) {
  auto [r, w] = std::move(*ard).split();
  auto [pard, pawd, pservice] = io_dev_align(std::move(r), std::move(w), *service);
  co_return co_await imm_serve_connection(*pard, *pawd, *pservice, std::move(parser));
}
/// @brief the connection class
template <ABRL ABr, ABWL ABw>
class Connection {
  using abr_type = ABr;
  using abw_type = ABw;

public:
  constexpr Connection(abr_type&& in_dev, abw_type&& out_dev)
      : _ard(std::move(in_dev)), _awd(std::move(out_dev)), _parser{} {}

  constexpr Connection(Connection&&) = default;
  constexpr Connection& operator=(Connection&&) = default;
  constexpr ~Connection() {}

  Task<Result> read(std::span<Request<abr_type>> reqs) {
    std::size_t i = 0;
    ParseData parse_data;
    for (auto& req : reqs) {
      {
        auto res = co_await _parser.read(this->_ard, parse_data);
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

  Task<Result> write(std::span<const Response<abw_type>> resps) {
    for (auto& resp : resps) {
      auto [sz, err] = co_await resp.sendto(this->_awd);
      if (err) {
        LOG3("send error: {}", std::make_error_code(*err).message());
        co_return Result{0, *err};
      }
    }
    co_return Result{0, std::nullopt};
  }
  template <class Service>
  Task<void> serve_connection(this Connection self, std::shared_ptr<Service> service) {
    auto [pard, pawd, pservice]
        = io_dev_align(std::move(self._ard), std::move(self._awd), *service);
    co_return co_await http::imm_serve_connection(*pard, *pawd, *pservice, std::move(self._parser));
  }

private:
  abr_type _ard;
  abw_type _awd;

  Parser<HttpParseTraits> _parser;
};
/// @brief make a connection
template <ABRWL ABrw>
constexpr Connection<typename ABrw::template rebind<In>, typename ABrw::template rebind<Out>>
make_connection(ABrw&& dev) {
  auto [r, w] = std::move(dev).split();
  return {std::move(r), std::move(w)};
}
/// @brief make a connection
template <class ABrw>
void make_connection(ABrw& dev) = delete;
XSL_HTTP_NE
#endif
