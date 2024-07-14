#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_STREAM
#  define XSL_NET_TRANSPORT_TCP_STREAM
#  include "xsl/coro/await.h"
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync/mutex.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys.h"

#  include <sys/socket.h>

#  include <atomic>
#  include <cstddef>
#  include <functional>
#  include <memory>
#  include <numeric>
#  include <optional>
#  include <string_view>
#  include <utility>
TCP_NB
enum class RecvErrorCategory {
  Unknown,
  Eof,
  NoData,
};
class RecvError {
public:
  RecvError(RecvErrorCategory category) noexcept : category(category) {}
  std::string_view message() const noexcept {
    switch (this->category) {
      case RecvErrorCategory::Unknown:
        return "Unknown error";
      case RecvErrorCategory::Eof:
        return "EOF";
      case RecvErrorCategory::NoData:
        return "No data";
    }
    return "Unknown error";
  }
  bool eof() const noexcept { return this->category == RecvErrorCategory::Eof; }
  RecvErrorCategory category;
};
using RecvResult = std::expected<void, RecvError>;

enum class SendErrorCategory {
  Unknown,
};

class SendError {
public:
  SendError(SendErrorCategory category) noexcept : category(category) {}
  std::string_view message() const noexcept {
    switch (this->category) {
      case SendErrorCategory::Unknown:
        return "Unknown error";
    }
    return "Unknown error";
  }
  SendErrorCategory category;
};

namespace impl_tcp_stream {
  struct InnerHandle {
    InnerHandle() noexcept : closed() , read_cb(std::nullopt), write_cb(std::nullopt) {}
    std::atomic_flag closed;
    sync::UnqRes<std::optional<std::function<void()>>> read_cb, write_cb;
  };
  class Callback {
  public:
    Callback(const std::shared_ptr<InnerHandle> &handle) noexcept : handle(handle) {}
    sync::PollHandleHint operator()(int, sync::IOM_EVENTS events) {
      DEBUG("TcpStream");
      if (this->handle->closed.test()) {
        return sync::PollHandleHintTag::DELETE;
      }
      if (!!(events & sync::IOM_EVENTS::IN)) {
        auto cb = std::exchange(*this->handle->read_cb.lock(), std::nullopt);
        if (cb) {
          (*cb)();
        }
      }
      if (!!(events & sync::IOM_EVENTS::OUT)) {
        auto cb = std::exchange(*this->handle->write_cb.lock(), std::nullopt);
        if (cb) {
          (*cb)();
        }
      }
      return sync::PollHandleHintTag::NONE;
    }

  private:
    std::shared_ptr<InnerHandle> handle;
  };
}  // namespace impl_tcp_stream

class TcpStream {
public:
  TcpStream(Socket &&sock, std::shared_ptr<sync::Poller> poller) noexcept
      : sock(std::move(sock)), handle(std::make_shared<impl_tcp_stream::InnerHandle>()) {
    poller->add(this->sock.raw_fd(),
                sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                impl_tcp_stream::Callback{this->handle});
  }
  TcpStream(TcpStream &&rhs) noexcept = default;
  TcpStream &operator=(TcpStream &&rhs) noexcept = default;
  ~TcpStream() {}
  coro::Task<RecvResult> read(std::string &rbuf) {
    TRACE("start recv string");
    std::vector<std::string> data;
    char buf[MAX_SINGLE_RECV_SIZE];
    ssize_t n;
    for (;;) {
      n = ::recv(this->sock.raw_fd(), buf, sizeof(buf), 0);
      DEBUG("recv n: {}", n);
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          if (!data.empty()) {
            DEBUG("recv over");
            break;
          }
          DEBUG("no data");
          co_await coro::CallbackAwaiter<void>{[handle = this->handle.get()](auto &&cb) {
            *handle->read_cb.lock() = std::move(cb);
            DEBUG("set read_cb");
          }};
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
      data.emplace_back(buf, n);
    };
    rbuf = std::accumulate(data.begin(), data.end(), rbuf);
    DEBUG("end recv string");
    co_return {};
  }
  coro::Task<RecvResult> read(std::string &rbuf, size_t size) {
    TRACE("start recv size");
    std::vector<std::string> data;
    char buf[MAX_SINGLE_RECV_SIZE];
    ssize_t n;
    size_t total = 0;
    do {
      n = ::recv(this->sock.raw_fd(), buf, std::min(sizeof(buf), size - total), 0);
      DEBUG("recv n: {}", n);
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          co_await coro::CallbackAwaiter<void>{[handle = this->handle.get()](auto &&cb) {
            *handle->read_cb.lock() = std::move(cb);
          }};
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
      data.emplace_back(buf, n);
      total += n;
    } while (total < size);
    rbuf = std::accumulate(data.begin(), data.end(), rbuf);
    co_return {};
  }
  coro::Task<std::expected<bool, SendError>> write(std::string_view data) {
    DEBUG("start write {} bytes", data.size());
    // ssize_t n = ::send(this->impl->sock.raw_fd(), data.data(), data.size(), 0);
    // DEBUG("write {} bytes", n);
    // if (n == -1) {
    //   if (errno == EAGAIN || errno == EWOULDBLOCK) {
    //     DEBUG("write over");
    //     return {true};
    //   } else {
    //     return std::unexpected{SendError{SendErrorCategory::Unknown}};
    //   }
    // } else if (n == 0) {
    //   return {0};
    // } else if (static_cast<size_t>(n) == data.size()) {
    //   DEBUG("write {} bytes", n);
    //   return {static_cast<size_t>(n)};
    // }
    // DEBUG("write {} bytes", n);
    // data = data.substr(n);
    // while (true) {
    //   ssize_t tmp = ::write(this->impl->sock.raw_fd(), data.data(), data.size());
    //   if (tmp == -1) {
    //     if (errno == EAGAIN || errno == EWOULDBLOCK) {
    //       return {static_cast<size_t>(n)};
    //     } else {
    //       return std::unexpected{SendError{SendErrorCategory::Unknown}};
    //     }
    //   } else if (tmp == 0) {
    //     return {static_cast<size_t>(n)};
    //   } else if (static_cast<size_t>(tmp) == data.size()) {
    //     DEBUG("write {} bytes", tmp);
    //     return {static_cast<size_t>(n + tmp)};
    //   }
    //   data = data.substr(tmp);
    //   n += tmp;
    // }
    // return std::unexpected{SendError{SendErrorCategory::Unknown}};

    while (true) {
      ssize_t n = ::send(this->sock.raw_fd(), data.data(), data.size(), 0);
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          DEBUG("need write again");
          co_await coro::CallbackAwaiter<void>{[handle = this->handle.get()](auto &&cb) {
            *handle->write_cb.lock() = std::move(cb);
          }};
          continue;
        } else {
          co_return std::unexpected{SendError{SendErrorCategory::Unknown}};
        }
      } else if (n == 0) {
        DEBUG("need write again");
        co_await coro::CallbackAwaiter<void>{
            [handle = this->handle.get()](auto &&cb) { *handle->write_cb.lock() = std::move(cb); }};
        continue;
      } else if (static_cast<size_t>(n) == data.size()) {
        co_return {true};
      }
      data = data.substr(n);
    }
  }

private:
  Socket sock;
  std::shared_ptr<impl_tcp_stream::InnerHandle> handle;
};
TCP_NE
#endif
