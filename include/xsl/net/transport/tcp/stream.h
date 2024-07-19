#pragma once
#ifndef XSL_NET_TRANSPORT_TCP_STREAM
#  define XSL_NET_TRANSPORT_TCP_STREAM
#  include "xsl/coro/semaphore.h"
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/sync.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys.h"

#  include <sys/socket.h>

#  include <memory>
#  include <string_view>
#  include <type_traits>
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

using SendResult = std::expected<void, SendError>;

class TcpStream {
public:
  TcpStream(Socket &&sock, std::shared_ptr<sync::Poller> poller) noexcept
      : sock(std::make_shared<Socket>(std::move(sock))),
        read_sem(std::make_shared<coro::CountingSemaphore<1>>()),
        write_sem(std::make_shared<coro::CountingSemaphore<1>>()) {
    using IOCallback = sync::PollCallback<sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>;
    poller->add(this->sock->raw_fd(),
                sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                IOCallback{this->read_sem, this->write_sem});
  }
  TcpStream(TcpStream &&rhs) noexcept
      : sock(std::move(rhs.sock)),
        read_sem(std::move(rhs.read_sem)),
        write_sem(std::move(rhs.write_sem)) {
    DEBUG("TcpStream move");
  }
  TcpStream &operator=(TcpStream &&rhs) noexcept = default;

  ~TcpStream() {}

  std::pair<std::shared_ptr<Socket>, std::shared_ptr<coro::CountingSemaphore<1>>> read_meta() {
    return {this->sock, this->read_sem};
  }
  std::pair<std::shared_ptr<Socket>, std::shared_ptr<coro::CountingSemaphore<1>>> write_meta() {
    return {this->sock, this->write_sem};
  }

  template <class Func>
    requires std::is_invocable_r_v<coro::Task<RecvResult>, Func, std::shared_ptr<Socket>,
                                   std::shared_ptr<coro::CountingSemaphore<1>>>
  decltype(auto) read(Func &&func) {
    return std::forward<Func>(func)(this->sock, this->read_sem);
  }

  template <class Func>
    requires std::is_invocable_r_v<coro::Task<SendResult>, Func, std::shared_ptr<Socket>,
                                   std::shared_ptr<coro::CountingSemaphore<1>>>
  decltype(auto) write(Func &&func) {
    return std::forward<Func>(func)(this->sock, this->write_sem);
  }

  template <class Func>
    requires std::is_invocable_r_v<coro::Task<RecvResult>, Func, Socket &,
                                   coro::CountingSemaphore<1> &>
  decltype(auto) unsafe_read(Func &&func) {
    return std::forward<Func>(func)(*this->sock, *this->read_sem);
  }

  template <class Func>
    requires std::is_invocable_r_v<coro::Task<SendResult>, Func, Socket &,
                                   coro::CountingSemaphore<1> &>
  decltype(auto) unsafe_write(Func &&func) {
    return std::forward<Func>(func)(*this->sock, *this->write_sem);
  }

private:
  std::shared_ptr<Socket> sock;
  std::shared_ptr<coro::CountingSemaphore<1>> read_sem, write_sem;
};
TCP_NE
#endif
