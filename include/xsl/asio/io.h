/**
 * @file io.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief IO utilities
 * @version 0.1.3
 * @date 2024-08-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_ASIO_IO
#  define XSL_ASIO_IO
#  include <xsl/error.h>

#  include <fcntl.h>
#  include <sys/sendfile.h>
#  include <sys/socket.h>
#  include <xsl/asio/def.h>
#  include <xsl/byte.h>
#  include <xsl/io.h>
#  include <xsl/io/def.h>
#  include <xsl/log.h>
#  include <xsl/sys.h>
#  include <xsl/type_traits.h>

#  include <cassert>
#  include <cstddef>
#  include <cstring>
#  include <span>
XSL_ASIO_NB

using sys::IOContext;
using sys::IOM_EVENTS;
using sys::PollHandleHint;
using sys::PollHandleHintTag;

template <IOM_EVENTS... Events>
using IOSignalStorage = StaticExactPubSubStorage<IOM_EVENTS, IOSignal, Events...>;

namespace _detail {
  template <IOM_EVENTS... Es>
  IOSignalStorage<Es...>* _extract_io_signal_storage(IOSignalStorage<Es...>*);

  template <class T>
  using extract_io_signal_storage_t
      = std::decay_t<decltype(*_extract_io_signal_storage(std::declval<T*>()))>;
}  // namespace _detail
XSL_ASIO_NE
XSL_NB
template <sys::IOM_EVENTS... Es1, sys::IOM_EVENTS... Es2>
struct is_same_pack<asio::IOSignalStorage<Es1...>, asio::IOSignalStorage<Es2...>> : std::true_type {
};
XSL_NE
XSL_ASIO_NB

template <class Inner>
  requires std::derived_from<Inner, RawOwner>
class DefaultEpollWrapper : public Inner, public sys::EpollHandler {
public:
  ~DefaultEpollWrapper() override = default;
  /// @brief remember the IOContext this device is registered in
  void bind_context(IOContext& ctx) noexcept { this->ctx_ = &ctx; }
  /// @brief whether the bound IOContext has been shut down
  bool stopped() const noexcept { return this->ctx_ != nullptr && this->ctx_->stopped(); }
  /// @brief deregister this device from its IOContext
  /// @note must be called while the caller still holds a reference to this
  ///       handler: the IOContext map keeps its own reference, so as long as
  ///       the entry exists this object can never be destroyed and the file
  ///       descriptor stays open
  void deregister() {
    if (this->ctx_ != nullptr) {
      this->ctx_->remove(this->raw());
      this->ctx_ = nullptr;
    }
  }
  PollHandleHint epoll_handle(int, IOM_EVENTS events) override {
    if (((!events) || !!(events & IOM_EVENTS::HUP))) {
      return PollHandleHintTag::DELETE;
    } else {
      this->publish([&events](IOM_EVENTS e) { return !!(events & e); });
      return PollHandleHintTag::NONE;
    }
  }
  /// @brief wake all subscribers (poller shutdown)
  void shutdown_notify() override { this->publish([](IOM_EVENTS) { return true; }); }

private:
  IOContext* ctx_ = nullptr;
};

template <class Traits, class Accessor>
class PollForCoro : public Accessor {
public:
  template <class _Accessor>
  constexpr PollForCoro(Traits, _Accessor&& a) : Accessor(std::forward<_Accessor>(a)) {}

  constexpr PollHandleHint operator()(this auto&& self, int, IOM_EVENTS events) {
    if (Traits::poll_check(events) == PollHandleHintTag::DELETE) {
      return PollHandleHintTag::DELETE;
    } else {
      self->publish([&events](IOM_EVENTS e) { return !!(events & e); });
      return PollHandleHintTag::NONE;
    }
  }
};

namespace _detail {
  template <IOM_EVENTS... Es>
  inline IOM_EVENTS or_(IOSignalStorage<Es...>*) {
    return (IOM_EVENTS::ET | ... | Es);
  }
}  // namespace _detail

Expected<void, errc> add_to_context(auto& s, IOContext& ctx, auto tag) {
  IOM_EVENTS ev = _detail::or_(&*s);
  ENSEC(ctx.add((*s).get()->raw(), ev, asio::PollForCoro{tag, std::forward<decltype(s)>(s)}));
  return {};
}

template <class Traits, class Accessor>
PollForCoro(Traits, Accessor&&) -> PollForCoro<Traits, std::remove_reference_t<Accessor>>;

template <class T>
constexpr Expected<void, errc> init_async_device(shared_memory<T>& s, RawOwner&& o,
                                                 IOContext& ctx) {
  s->template emplace<RawOwner>(std::move(o));
  s->template emplace<_detail::extract_io_signal_storage_t<T>>();
  if constexpr (requires { s->bind_context(ctx); }) {
    s->bind_context(ctx);
  }
  IOM_EVENTS ev = _detail::or_(&*s);
  ENSEC(ctx.add(s->raw(), ev, s));
  return {};
}

/**
 * @brief Receive data from a device, specialized for not connect-based device
 *
 * @tparam Pointer
 * @param _raw
 * @param buf
 * @param sig
 * @return Task<io::Result>
 */
Task<io::Result> imm_recv(RawHandle _raw, byte* data, std::size_t size, auto& sig) {
  log_trace("{} try to recv {} bytes", _raw, size);
  do {
    ssize_t n = ::recv(_raw, data, size, 0);
    log_trace("{} recv {} bytes", _raw, n);
    if (n >= 0) {
      co_return {static_cast<size_t>(n)};
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      log_trace("{} need wait for recv", _raw);
      if (!co_await sig) co_return {0, {errc::not_connected}};
    } else
      co_return {0, {errc(errno)}};
  } while (true);
}
/**
 * @brief Receive data from a device
 *
 * @tparam Pointer
 * @param _raw
 * @param buf
 * @param sig
 * @return Task<io::Result>
 */
decltype(auto) recv(RawHandle _raw, byte* data, std::size_t size, auto& sig) {
  return imm_recv(_raw, data, size, sig).then([&](io::Result res) -> io::Result {
    if (res.size == 0) {
      log_trace("recv {} bytes, not connected", res.size);
      return {0, errc::not_connected};
    }
    return res;
  });
}
/**
 * @brief Receive data from a device
 *
 * @tparam Pointer
 * @param _raw
 * @param buf
 * @param sig
 * @return Task<io::Result>
 */
decltype(auto) recv(RawHandle _raw, std::span<byte>& buf, auto& sig) {
  return imm_recv(_raw, buf.data(), buf.size(), sig).then([&](io::Result res) -> io::Result {
    if (res.size == 0) {
      log_trace("recv {} bytes, not connected", res.size);
      return {0, errc::not_connected};
    }
    buf = buf.subspan(res.size);
    return res;
  });
}
/**
 * @brief Receive data from a device
 *
 * @tparam Pointer
 * @param _raw
 * @param buf
 * @param sig
 * @return Task<io::Result>
 */
decltype(auto) recv(RawHandle _raw, const std::span<byte>& buf, auto& sig) {
  return imm_recv(_raw, buf.data(), buf.size(), sig).then([&](io::Result res) -> io::Result {
    if (res.size == 0) {
      log_trace("recv {} bytes, not connected", res.size);
      return {0, errc::not_connected};
    }
    return res;
  });
}

struct NetAsyncRx {
  decltype(auto) read(this auto&& self, std::span<byte>& buf) {
    if constexpr (self.is_connection_based()) {
      return recv(self.raw(), buf, self.read_signal());
    } else {
      return imm_recv(self.raw(), buf, self.read_signal());
    }
  }
  decltype(auto) read(this auto&& self, byte* data, std::size_t size) {
    if constexpr (self.is_connection_based()) {
      return recv(self.raw(), data, size, self.read_signal());
    } else {
      return imm_recv(self.raw(), data, size, self.read_signal());
    }
  }

  template <class Self, sys::net::SocketTraitsCompatible<typename Self::socket_traits_type> Up>
  Task<io::Result> recvfrom(this Self& self, sys::net::SockAddr<Up>& addr, byte* buf,
                            std::size_t n) {
    do {
      ssize_t sz = ::recvfrom(self.raw(), buf, n, 0, &addr.addr(), &addr.len());
      if (sz >= 0) {
        co_return {static_cast<size_t>(sz)};
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (!co_await self.read_signal()) {
          co_return {0, {errc::not_connected}};
        }
      } else {
        co_return {0, {errc(errno)}};
      }
    } while (true);
  }
  template <class Self, sys::net::SocketTraitsCompatible<typename Self::traits_type> Up>
  decltype(auto) recvfrom(this Self& self, sys::net::SockAddr<Up>& addr, std::span<byte> buf) {
    return self.recvfrom(addr, buf.data(), buf.size()).then([&](io::Result res) -> io::Result {
      buf = buf.subspan(res.size);
      return res;
    });
  }
};
/**
 * @brief Send data to a device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param data the data to send
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class Signal>
Task<io::Result> send(RawHandle _raw, const byte* data, std::size_t size, Signal& sig) {
  std::size_t total = 0;
  do {
    ssize_t n = ::send(_raw, data + total, size - total, 0);
    log_debug("send {} bytes", n);
    if (n > 0) {
      total += n;
    } else if (n == 0) {
      break;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return {total, {errc::not_connected}};
      }
    } else {
      co_return {total, {errc(errno)}};
    }
  } while (total < size);
  co_return {total};
}
/**
 * @brief Send data to a device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param data the data to send
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
template <class Signal>
Task<io::Result> send(RawHandle _raw, std::span<const byte>& data, Signal& sig) {
  return send(_raw, data.data(), data.size(), sig).then([&](io::Result res) -> io::Result {
    if (res.size == 0) {
      log_trace("send {} bytes, not connected", res.size);
      return {0, errc::not_connected};
    }
    data = data.subspan(res.size);
    return res;
  });
}
/**
 * @brief Send data to a device
 *
 * @tparam Dev the device type
 * @param dev the device
 * @param data the data to send
 * @return Task<io::Result>
 */
template <class Dev>
constexpr Task<io::Result> send(Dev& dev, std::span<const byte> data) {
  return dev.send(data);
}

/**
 * @brief write file to device
 *
 * @tparam Pointer the pointer type, typically is a shared_ptr
 * @param _raw the raw handle
 * @param hint the hint to write file
 * @param sig the signal receiver
 * @return Task<io::Result>
 */
Task<io::Result> send_file(RawHandle _raw, io::WriteFileHint hint, auto& sig) {
  int ffd = open(hint.path.c_str(), O_RDONLY | O_CLOEXEC);
  if (ffd == -1) {
    log_error("open file failed");
    co_return io::Result{0, errc(errno)};
  }
  Defer defer{[ffd] { close(ffd); }};
  off_t offset = hint.offset;
  off_t end = hint.size + offset;
  do {
    ssize_t n = ::sendfile(_raw, ffd, &offset, end - offset);
    if (n > 0) {
      log_debug("[sendfile] send {} bytes", n);
    } else if (n == 0) {
      log_trace("{} send {} bytes file", _raw, n);
      if (offset != end) {
        break;
      }
      co_return io::Result(offset - hint.offset, errc::no_message);
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (!co_await sig) {
        co_return io::Result(offset - hint.offset, errc::not_connected);
      }
    } else {
      co_return io::Result(offset - hint.offset, errc(errno));
    }
  } while (offset != end);
  co_return io::Result(offset - hint.offset);
}

struct NetAsyncTx {
  /// @brief Send data to a device
  Task<io::Result> write(this auto&& self, std::span<const byte> data) {
    return send(self.raw(), data, self.write_signal());
  }
  /// @brief Send data to a device
  Task<io::Result> write(this auto&& self, const byte* data, std::size_t size) {
    return send(self.raw(), data, size, self.write_signal());
  }
  /// @brief Send data to a specific address through a device
  template <class Self, sys::net::SocketTraitsCompatible<typename Self::socket_traits_type> Up>
  Task<io::Result> sendto(this Self& self, sys::net::SockAddr<Up>& addr, const byte* buf,
                          std::size_t n) {
    auto begin = buf;
    do {
      ssize_t sz = ::sendto(self.raw(), begin, n, 0, &addr.addr(), addr.len());
      if (sz >= 0) {
        begin += sz;
        n -= sz;
      } else if (sz == 0) {
        break;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (!co_await self.write_signal()) {
          co_return {static_cast<std::size_t>(begin - buf), {errc::not_connected}};
        }
      } else {
        co_return {static_cast<std::size_t>(begin - buf), {errc(errno)}};
      }
    } while (n > 0);
    co_return {static_cast<std::size_t>(begin - buf)};
  }
  /// @brief Send data to a specific address through a device
  template <class Self, sys::net::SocketTraitsCompatible<typename Self::socket_traits_type> Up>
  Task<io::Result> sendto(this Self& self, sys::net::SockAddr<Up>& addr,
                          std::span<const byte> data) {
    return self.sendto(addr, data.data(), data.size()).then([&](io::Result res) -> io::Result {
      data = data.subspan(res.size);
      return res;
    });
  }
  /// @brief write file to device
  Task<io::Result> write_file(this auto&& self, WriteFileHint&& hint) {
    return send_file(self.raw(), std::move(hint), self.write_signal());
  }
};

XSL_ASIO_NE
#endif
