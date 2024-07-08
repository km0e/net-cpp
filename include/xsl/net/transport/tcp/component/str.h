#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_STR_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_STR_H_
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/component/def.h"
#  include "xsl/net/transport/tcp/utils.h"

#  include <sys/socket.h>

#  include <cstddef>
#  include <list>
#  include <ranges>

TCP_COMPONENTS_NAMESPACE_BEGIN
namespace impl {
  template <class... Flags>
  class TcpSendString;
  template <>
  class TcpSendString<feature::placeholder> {
  public:
    TcpSendString(TcpSendString&&) = default;
    TcpSendString(std::string&& data) : data_buffer() {
      this->data_buffer.emplace_back(std::move(data), 0);
    }
    TcpSendString(std::list<std::string>&& data) : data_buffer() {
      auto buf = data | std::views::transform([](std::string& s) {
                   return std::pair<std::string, size_t>{std::move(s), 0};
                 });
      this->data_buffer = std::list<std::pair<std::string, size_t>>(buf.begin(), buf.end());
    }
    ~TcpSendString() {}
    SendResult exec(int fd) {
      while (!this->data_buffer.empty()) {
        auto& data = this->data_buffer.front();
        auto res = send(fd, std::string_view(data.first).substr(data.second));
        if (!res.has_value()) {
          return std::unexpected{res.error()};
        }
        data.second += res.value();
        if (data.second != data.first.size()) {
          return {false};
        }
        this->data_buffer.pop_front();
      }
      return {true};
    }
    std::list<std::pair<std::string, size_t>> data_buffer;
  };
  template <>
  class TcpSendString<feature::node> : public SendTaskNode, TcpSendString<feature::placeholder> {
  public:
    using Base = TcpSendString<feature::placeholder>;
    using Base::Base;
    ~TcpSendString() {}
    SendResult exec(SendContext& ctx) override {
      DEBUG("compontent send");
      return Base::exec(ctx.sfd);
    }
  };
}  // namespace impl

template <class... Flags>
using TcpSendString
    = feature::origanize_feature_flags_t<impl::TcpSendString<feature::node>, Flags...>;

namespace impl {

  template <class... Flags>
  class TcpRecvString;
  template <>
  class TcpRecvString<feature::placeholder> {
  public:
    TcpRecvString(TcpRecvString&&) = default;
    TcpRecvString() : data_buffer() {}
    ~TcpRecvString() {}
    std::expected<bool, RecvError> exec(int fd) {
      auto res = recv(fd);
      if (!res.has_value()) {
        return std::unexpected{res.error()};
      }
      this->data_buffer += res.value();
      return {true};
    }
    std::string data_buffer;
  };
  template <>
  class TcpRecvString<feature::node> : public RecvTaskNode, TcpRecvString<feature::placeholder> {
  public:
    using Base = TcpRecvString<feature::placeholder>;
    using Base::Base;
    ~TcpRecvString() {}
    std::expected<bool, RecvError> exec(RecvContext& ctx) override { return Base::exec(ctx.sfd); }
  };

}  // namespace impl
template <class... Flags>
using TcpRecvString
    = feature::origanize_feature_flags_t<impl::TcpRecvString<feature::node>, Flags...>;

TCP_COMPONENTS_NAMESPACE_END
#endif
