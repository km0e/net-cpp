#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_STR_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_STR_H_
#  include "xsl/feature.h"
#  include "xsl/net/transport/tcp/component/def.h"
#  include "xsl/net/transport/tcp/utils.h"
#  include "xsl/wheel.h"
#  include "xsl/wheel/result.h"
#  include "xsl/wheel/type_traits.h"

#  include <spdlog/spdlog.h>
#  include <sys/socket.h>

#  include <ranges>

TCP_COMPONENTS_NAMESPACE_BEGIN
namespace impl {
  template <class... Flags>
  class TcpSendString;
  template <>
  class TcpSendString<feature::placeholder> {
  public:
    TcpSendString(TcpSendString&&) = default;
    TcpSendString(string&& data) : data_buffer() {
      this->data_buffer.emplace_back(xsl::move(data), 0);
    }
    TcpSendString(list<string>&& data) : data_buffer() {
      auto buf = data | std::views::transform([](string& s) {
                   return pair<string, size_t>{xsl::move(s), 0};
                 });
      this->data_buffer = list<pair<string, size_t>>(buf.begin(), buf.end());
    }
    ~TcpSendString() {}
    SendResult exec(int fd) {
      while (!this->data_buffer.empty()) {
        auto& data = this->data_buffer.front();
        auto res = send(fd, string_view(data.first).substr(data.second));
        if (res.is_err()) {
          return {res.unwrap_err()};
        }
        data.second += res.unwrap();
        if (data.second != data.first.size()) {
          return {false};
        }
        this->data_buffer.pop_front();
      }
      return {true};
    }
    list<pair<string, size_t>> data_buffer;
  };
  template <>
  class TcpSendString<feature::node> : public SendTaskNode, TcpSendString<feature::placeholder> {
  public:
    using Base = TcpSendString<feature::placeholder>;
    using Base::Base;
    ~TcpSendString() {}
    SendResult exec(SendContext& ctx) override {
      SPDLOG_DEBUG("compontent send");
      return Base::exec(ctx.sfd);
    }
  };
}  // namespace impl

template <class... Flags>
using TcpSendString = wheel::type_traits::swap_t<
    impl::TcpSendString<>,
    feature::origanize_feature_flags_t<wheel::type_traits::_n<Flags...>,
                                       wheel::type_traits::_n<feature::node>>>::type1;

namespace impl {

  template <class... Flags>
  class TcpRecvString;
  template <>
  class TcpRecvString<feature::placeholder> {
  public:
    TcpRecvString(TcpRecvString&&) = default;
    TcpRecvString() : data_buffer() {}
    ~TcpRecvString() {}
    Result<bool, RecvError> exec(int fd) {
      auto res = recv(fd);
      if (res.is_err()) {
        return {res.unwrap_err()};
      }
      this->data_buffer += res.unwrap();
      return {true};
    }
    string data_buffer;
  };
  template <>
  class TcpRecvString<feature::node> : public RecvTaskNode, TcpRecvString<feature::placeholder> {
  public:
    using Base = TcpRecvString<feature::placeholder>;
    using Base::Base;
    ~TcpRecvString() {}
    Result<bool, RecvError> exec(RecvContext& ctx) override { return Base::exec(ctx.sfd); }
  };

}  // namespace impl
template <class... Flags>
using TcpRecvString = wheel::type_traits::swap_t<
    impl::TcpRecvString<>,
    feature::origanize_feature_flags_t<wheel::type_traits::_n<Flags...>,
                                       wheel::type_traits::_n<feature::node>>>::type1;

TCP_COMPONENTS_NAMESPACE_END
#endif
