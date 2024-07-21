#pragma once
#ifndef XSL_SYS_IO
#  define XSL_SYS_IO
#  include "xsl/coro/semaphore.h"
#  include "xsl/coro/task.h"
#  include "xsl/logctl.h"
#  include "xsl/sync.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys/def.h"

#  include <sys/socket.h>
#  include <unistd.h>

#  include <cstddef>
#  include <expected>
#  include <memory>
#  include <optional>
#  include <span>
#  include <string_view>
#  include <tuple>
#  include <utility>
SYS_NB
namespace io {
  const size_t MAX_SINGLE_RECV_SIZE = 1024;
  class NativeDevice {
  public:
    NativeDevice(int fd) noexcept : _fd(fd) {}
    NativeDevice(NativeDevice &&rhs) noexcept : _fd(std::exchange(rhs._fd, -1)) {}
    NativeDevice &operator=(NativeDevice &&rhs) noexcept {
      _fd = std::exchange(rhs._fd, -1);
      return *this;
    }
    int raw() const noexcept { return _fd; }

    ~NativeDevice() noexcept {
      DEBUG("close fd: {}", _fd);
      close(_fd);
    }

  protected:
    int _fd;
  };
  class AsyncReadDevice;
  class ReadDevice {
  public:
    explicit ReadDevice(NativeDevice &&dev) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))) {}
    ReadDevice(std::shared_ptr<NativeDevice> dev) noexcept : _dev(std::move(dev)) {}
    ReadDevice(ReadDevice &&rhs) noexcept : _dev(std::move(rhs._dev)) {}
    ReadDevice &operator=(ReadDevice &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      return *this;
    }
    int raw() const noexcept { return _dev->raw(); }
    AsyncReadDevice async(std::shared_ptr<sync::Poller> &poller) && noexcept;

  protected:
    std::shared_ptr<NativeDevice> _dev;
  };

  class AsyncWriteDevice;
  class WriteDevice {
  public:
    WriteDevice(NativeDevice &&dev) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))) {}
    WriteDevice(std::shared_ptr<NativeDevice> dev) noexcept : _dev(std::move(dev)) {}
    WriteDevice(WriteDevice &&rhs) noexcept : _dev(std::move(rhs._dev)) {}
    WriteDevice &operator=(WriteDevice &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      return *this;
    }
    int raw() const noexcept { return _dev->raw(); }
    AsyncWriteDevice async(std::shared_ptr<sync::Poller> &poller) && noexcept;

  protected:
    std::shared_ptr<NativeDevice> _dev;
  };

  class AsyncReadDevice : public ReadDevice {
  public:
    AsyncReadDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : ReadDevice(std::move(dev)), _sem(std::move(sem)) {}
    AsyncReadDevice(std::shared_ptr<NativeDevice> dev,
                    std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : ReadDevice(std::move(dev)), _sem(std::move(sem)) {}
    AsyncReadDevice(AsyncReadDevice &&rhs) noexcept
        : ReadDevice(std::move(rhs)), _sem(std::move(rhs._sem)) {}
    AsyncReadDevice &operator=(AsyncReadDevice &&rhs) noexcept {
      ReadDevice::operator=(std::move(rhs));
      _sem = std::move(rhs._sem);
      return *this;
    }
    decltype(auto) raw() const noexcept { return ReadDevice::raw(); }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

    bool is_valid() const noexcept { return !this->_sem.unique(); }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
  };

  inline AsyncReadDevice ReadDevice::async(std::shared_ptr<sync::Poller> &poller) && noexcept {
    auto sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller->add(_dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
                sync::PollCallback<sync::IOM_EVENTS::IN>{sem});
    return AsyncReadDevice{std::move(_dev), std::move(sem)};
  }

  class AsyncWriteDevice : public WriteDevice {
  public:
    AsyncWriteDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : WriteDevice(std::move(dev)), _sem(std::move(sem)) {}
    AsyncWriteDevice(std::shared_ptr<NativeDevice> dev,
                     std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : WriteDevice(std::move(dev)), _sem(std::move(sem)) {}
    AsyncWriteDevice(AsyncWriteDevice &&rhs) noexcept
        : WriteDevice(std::move(rhs)), _sem(std::move(rhs._sem)) {}
    AsyncWriteDevice &operator=(AsyncWriteDevice &&rhs) noexcept {
      WriteDevice::operator=(std::move(rhs));
      _sem = std::move(rhs._sem);
      return *this;
    }

    decltype(auto) raw() const noexcept { return WriteDevice::raw(); }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

    bool is_valid() const noexcept { return !this->_sem.unique(); }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
  };

  inline AsyncWriteDevice WriteDevice::async(std::shared_ptr<sync::Poller> &poller) && noexcept {
    auto sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller->add(_dev->raw(), sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                sync::PollCallback<sync::IOM_EVENTS::OUT>{sem});
    return AsyncWriteDevice{std::move(_dev), std::move(sem)};
  }

  class AsyncDevice;
  class Device {
  public:
    explicit Device(NativeDevice &&dev) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))) {}
    Device(std::shared_ptr<NativeDevice> dev) noexcept : _dev(std::move(dev)) {}
    Device(Device &&rhs) noexcept : _dev(std::move(rhs._dev)) {}
    Device &operator=(Device &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      return *this;
    }
    int raw() const noexcept { return _dev->raw(); }

    std::tuple<ReadDevice, WriteDevice> split() && noexcept {
      return {ReadDevice{std::move(_dev)}, WriteDevice{std::move(_dev)}};
    }

    AsyncDevice async(std::shared_ptr<sync::Poller> &poller) && noexcept;

  protected:
    std::shared_ptr<NativeDevice> _dev;
  };

  class AsyncDevice : public Device {
  public:
    AsyncDevice(std::shared_ptr<NativeDevice> dev,
                std::shared_ptr<coro::CountingSemaphore<1>> read_sem,
                std::shared_ptr<coro::CountingSemaphore<1>> write_sem) noexcept
        : Device(std::move(dev)),
          _read_sem(std::move(read_sem)),
          _write_sem(std::move(write_sem)) {}
    AsyncDevice(AsyncDevice &&rhs) noexcept
        : Device(std::move(rhs)),
          _read_sem(std::move(rhs._read_sem)),
          _write_sem(std::move(rhs._write_sem)) {}
    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      Device::operator=(std::move(rhs));
      _read_sem = std::move(rhs._read_sem);
      _write_sem = std::move(rhs._write_sem);
      return *this;
    }

    decltype(auto) raw() const noexcept { return Device::raw(); }

    coro::CountingSemaphore<1> &read_sem() noexcept { return *_read_sem; }
    coro::CountingSemaphore<1> &write_sem() noexcept { return *_write_sem; }

    std::tuple<AsyncReadDevice, AsyncWriteDevice> split() && noexcept {
      return {AsyncReadDevice{_dev, std::move(_read_sem)},
              AsyncWriteDevice{_dev, std::move(_write_sem)}};
    }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _read_sem;
    std::shared_ptr<coro::CountingSemaphore<1>> _write_sem;
  };

  inline AsyncDevice Device::async(std::shared_ptr<sync::Poller> &poller) && noexcept {
    auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
    auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller->add(
        _dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
        sync::PollCallback<sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>{read_sem, write_sem});
    return AsyncDevice{std::move(_dev), std::move(read_sem), std::move(write_sem)};
  }

  class Buffer {
  public:
    Buffer() = default;
    Buffer(std::size_t size) : _size(size), _buf(std::make_unique<std::byte[]>(size)) {}
    operator std::byte *() noexcept { return _buf.get(); }
    size_t size() const noexcept { return _size; }

  protected:
    std::size_t _size;
    std::unique_ptr<std::byte[]> _buf;
  };

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
  using RecvResult = std::expected<std::size_t, RecvError>;
  inline coro::Task<std::tuple<std::size_t, std::optional<RecvError>>> unsafe_exact_read(
      AsyncReadDevice &dev, std::span<std::byte> buf) {
    ssize_t n;
    size_t offset = 0;
    while (true) {
      n = ::recv(dev.raw(), buf.data() + offset, buf.size() - offset, 0);
      DEBUG("recv n: {}", n);
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          if (offset != 0) {
            DEBUG("recv over");
            break;
          }
          DEBUG("no data");
          co_await dev.sem();
          continue;
        } else {
          ERROR("Failed to recv data, err : {}", strerror(errno));
          // TODO: handle recv error
          co_return std::make_tuple(
              offset, std::optional<RecvError>{RecvError{RecvErrorCategory::Unknown}});
        }
      } else if (n == 0) {
        DEBUG("recv eof");
        co_return std::make_tuple(offset,
                                  std::optional<RecvError>{RecvError{RecvErrorCategory::Eof}});
      }
      TRACE("recv {} bytes", n);
      offset += n;
    };
    DEBUG("end recv string");
    co_return std::make_tuple(offset, std::nullopt);
  }
  // inline coro::Task<std::tuple<std::size_t, std::optional<RecvError>>> unsafe_read(
  //     Device &dev, coro::CountingSemaphore<1> &sem, std::forward_list<Buffer> &bufs) {
  //   std::size_t size = 0;
  //   for (auto &buf : bufs) {
  //     auto [n, err] = co_await unsafe_exact_read(dev, sem, buf);
  //     size += n;
  //     if (err) {
  //       co_return std::make_tuple(size, err);
  //     } else if (n < buf.size()) {
  //       break;
  //     }
  //   }
  //   co_return std::make_tuple(size, std::nullopt);
  // }
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
  inline coro::Task<std::expected<void, SendError>> write(AsyncWriteDevice &dev,
                                                          std::span<const std::byte> data) {
    while (true) {
      ssize_t n = ::send(dev.raw(), data.data(), data.size(), 0);
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          DEBUG("need write again");
          co_await dev.sem();
          continue;
        } else {
          co_return std::unexpected{SendError{SendErrorCategory::Unknown}};
        }
      } else if (n == 0) {
        DEBUG("need write again");
        co_await dev.sem();
        continue;
      } else if (static_cast<size_t>(n) == data.size()) {
        co_return {};
      }
      data = data.subspan(n);
    }
  }
}  // namespace io
SYS_NE
#endif
