#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_HELPER_STR
#  define XSL_NET_TRANSPORT_TCP_HELPER_STR
#  include "xsl/coro/lazy.h"
#  include "xsl/coro/semaphore.h"
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/component/def.h"
#  include "xsl/net/transport/tcp/stream.h"
#  include "xsl/sync.h"
#  include "xsl/sync/poller.h"

#  include <fcntl.h>
#  include <sys/socket.h>

#  include <cstddef>
#  include <expected>
#  include <list>
#  include <memory>
#  include <numeric>
#  include <ranges>
#  include <string>
#  include <string_view>
#  include <system_error>
#  include <tuple>

TCP_COMPONENTS_NB
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
    std::expected<bool, RecvError> exec([[maybe_unused]] int fd) {
      // while (!this->data_buffer.empty()) {
      //   auto& data = this->data_buffer.front();
      //   auto res = send(fd, std::string_view(data.first).substr(data.second));
      //   if (!res.has_value()) {
      //     return std::unexpected{res.error()};
      //   }
      //   data.second += res.value();
      //   if (data.second != data.first.size()) {
      //     return {false};
      //   }
      //   this->data_buffer.pop_front();
      // }
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
    std::expected<bool, RecvError> exec(SendContext& ctx) override {
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
    std::expected<bool, RecvErrorCategory> exec([[maybe_unused]] int fd) {
      // auto res = ::recv(fd);
      // if (!res.has_value()) {
      //   return std::unexpected{res.error()};
      // }
      // this->data_buffer += res.value();
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
    std::expected<bool, RecvErrorCategory> exec(RecvContext& ctx) override {
      return Base::exec(ctx.sfd);
    }
  };

}  // namespace impl
template <class... Flags>
using TcpRecvString
    = feature::origanize_feature_flags_t<impl::TcpRecvString<feature::node>, Flags...>;

namespace impl_string_writer {
  template <class... Flags>
  class StringWriter;
  template <>
  class StringWriter<feature::placeholder> {
  public:
    StringWriter(std::string& data) : data(data) {}
    coro::Task<std::expected<void, RecvError>> operator()(sys::Socket& sock,
                                                          coro::CountingSemaphore<1>& sem) {
      std::vector<std::string> datalist;
      char buf[MAX_SINGLE_RECV_SIZE];
      ssize_t n;
      while (true) {
        n = ::recv(sock.raw(), buf, sizeof(buf), 0);
        DEBUG("recv n: {}", n);
        if (n == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (!datalist.empty()) {
              DEBUG("recv over");
              break;
            }
            DEBUG("no data");
            co_await sem;
            continue;
          } else {
            ERROR("Failed to recv data, err : {}", strerror(errno));
            // TODO: handle recv error
            co_return std::unexpected{RecvError{RecvErrorCategory::Unknown}};
          }
        } else if (n == 0) {
          DEBUG("recv eof");
          co_return std::unexpected{RecvError{RecvErrorCategory::Eof}};
          break;
        }
        TRACE("recv {} bytes", n);
        datalist.emplace_back(buf, n);
      };
      this->data = std::accumulate(datalist.begin(), datalist.end(), this->data);
      DEBUG("end recv string");
      co_return {};
    }
    ~StringWriter() {}
    std::string& data;
  };
  template <>
  class StringWriter<feature::Exact> {
  public:
    StringWriter(std::string& data, size_t size) : data(data), size(size) {}
    coro::Task<std::expected<void, RecvError>> operator()(sys::Socket& sock,
                                                          coro::CountingSemaphore<1>& sem) {
      std::vector<std::string> datalist;
      char buf[MAX_SINGLE_RECV_SIZE];
      ssize_t n;
      size_t total = 0;
      do {
        n = ::recv(sock.raw(), buf, std::min(sizeof(buf), size - total), 0);
        DEBUG("recv n: {}", n);
        if (n == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            co_await sem;
            continue;
          } else {
            ERROR("Failed to recv data, err : {}", strerror(errno));
            co_return std::unexpected{RecvError{RecvErrorCategory::Unknown}};
          }
        } else if (n == 0) {
          DEBUG("recv eof");
          co_return std::unexpected{RecvError{RecvErrorCategory::Eof}};
          break;
        }
        datalist.emplace_back(buf, n);
        total += n;
      } while (total < size);
      this->data = std::accumulate(datalist.begin(), datalist.end(), this->data);
      co_return {};
    }
    ~StringWriter() {}
    std::string& data;
    size_t size;
  };
  StringWriter(std::string&) -> StringWriter<feature::placeholder>;
  StringWriter(std::string& data, size_t size) -> StringWriter<feature::Exact>;
}  // namespace impl_string_writer
using impl_string_writer::StringWriter;
namespace impl_string_reader {
  template <class... Flags>
  class StringReader;
  template <>
  class StringReader<feature::placeholder> {
  public:
    StringReader(std::string_view data) : data(data) {}
    coro::Task<std::expected<void, SendError>> operator()(sys::Socket& sock,
                                                          coro::CountingSemaphore<1>& sem) {
      while (true) {
        ssize_t n = ::send(sock.raw(), data.data(), data.size(), 0);
        if (n == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            DEBUG("need write again");
            co_await sem;
            continue;
          } else {
            co_return std::unexpected{SendError{SendErrorCategory::Unknown}};
          }
        } else if (n == 0) {
          DEBUG("need write again");
          co_await sem;
          continue;
        } else if (static_cast<size_t>(n) == data.size()) {
          co_return {};
        }
        data = data.substr(n);
      }
    }

    ~StringReader() {}

    std::string_view data;
  };
  StringReader(std::string_view) -> StringReader<feature::placeholder>;
}  // namespace impl_string_reader

using impl_string_reader::StringReader;

namespace impl_string_forwarder {
  const size_t MAX_SINGLE_FWD_SIZE = 4096;
  inline coro::Lazy<std::expected<void, SendError>> write(
      int pipe_fd, std::shared_ptr<coro::CountingSemaphore<1>> read_sem,
      std::pair<std::shared_ptr<sys::Socket>, std::shared_ptr<coro::CountingSemaphore<1>>>
          write_meta) {
    auto [sock, sem] = write_meta;
    while (true) {
      ssize_t n = ::splice(pipe_fd, nullptr, sock->raw(), nullptr, MAX_SINGLE_FWD_SIZE,
                           SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);
      DEBUG("send n: {}", n);
      if (n == 0) {
        co_await *read_sem;
      } else if (n == -1) {
        if (errno == EAGAIN) {
          co_await *read_sem;
          continue;
        } else {
          ERROR("Failed to send data, err : {}", strerror(errno));
          co_return std::unexpected{SendError{SendErrorCategory::Unknown}};
        }
      }
      DEBUG("fwd: send {} bytes", n);
    }
  }
  inline coro::Lazy<std::expected<void, RecvError>> read(
      int pipe_fd, std::shared_ptr<coro::CountingSemaphore<1>> write_sem [[maybe_unused]],
      std::pair<std::shared_ptr<sys::Socket>, std::shared_ptr<coro::CountingSemaphore<1>>>
          read_meta) {
    auto [sock, sem] = read_meta;
    while (true) {
      ssize_t n = ::splice(sock->raw(), nullptr, pipe_fd, nullptr, MAX_SINGLE_FWD_SIZE,
                           SPLICE_F_MOVE | SPLICE_F_MORE | SPLICE_F_NONBLOCK);
      DEBUG("recv n: {}", n);
      if (n == 0) {
        co_await *sem;
      } else if (n == -1) {
        if (errno == EAGAIN) {
          DEBUG("no data");
          co_await *sem;
          continue;
        } else {
          ERROR("Failed to recv data, err : {}", strerror(errno));
          co_return std::unexpected{RecvError{RecvErrorCategory::Unknown}};
        }
      }
      DEBUG("fwd: recv {} bytes", n);
    }
  }
  template <class... Flags>
  class StringForwarder;
  template <>
  class StringForwarder<TcpStream, TcpStream> {
  public:
    StringForwarder(std::shared_ptr<coro::CountingSemaphore<1>> read_sem,
                    std::shared_ptr<coro::CountingSemaphore<1>> write_sem) noexcept
        : read_sem(read_sem), write_sem(write_sem) {}

  private:
    std::shared_ptr<coro::CountingSemaphore<1>> read_sem, write_sem;
  };
}  // namespace impl_string_forwarder
using impl_string_forwarder::StringForwarder;
template <class From, class To>
std::expected<std::tuple<StringForwarder<From, To>, coro::Lazy<std::expected<void, RecvError>>,
                         coro::Lazy<std::expected<void, SendError>>>,
              std::errc>
forward(std::shared_ptr<Poller> poller, From& from, To& to) {
  int pipe_fd[2];
  int res = ::pipe2(pipe_fd, O_NONBLOCK);
  if (res == -1) {
    return std::unexpected{std::errc{errno}};
  }
  auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
  auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
  using InCallback = sync::PollCallback<sync::IOM_EVENTS::IN>;
  poller->add(pipe_fd[0], sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET, InCallback{read_sem});
  using OutCallback = sync::PollCallback<sync::IOM_EVENTS::OUT>;
  poller->add(pipe_fd[1], sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET, OutCallback{write_sem});
  auto lazy_read = impl_string_forwarder::read(pipe_fd[1], write_sem, from.read_meta());
  auto lazy_write = impl_string_forwarder::write(pipe_fd[0], read_sem, to.write_meta());
  return {std::make_tuple(StringForwarder<From, To>{read_sem, write_sem}, std::move(lazy_read),
                          std::move(lazy_write))};
}  // namespace impl_string_forwarder

TCP_COMPONENTS_NE
#endif
