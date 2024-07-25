#pragma once
#ifndef XSL_SYS_IO
#  define XSL_SYS_IO
#  include "xsl/coro/semaphore.h"
#  include "xsl/coro/task.h"
#  include "xsl/feature.h"
#  include "xsl/logctl.h"
#  include "xsl/sync.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sys/def.h"
#  include "xsl/wheel/type_traits.h"

#  include <fcntl.h>
#  include <sys/sendfile.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <unistd.h>

#  include <cstddef>
#  include <expected>
#  include <filesystem>
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
      if (_fd == -1) {
        return;
      }
      LOG5("close fd: {}", _fd);
      close(_fd);
    }

  protected:
    int _fd;
  };

  template <class... Flags>
  class AsyncDevice;
  using AsyncReadDevice = AsyncDevice<feature::In, feature::placeholder>;
  using AsyncWriteDevice = AsyncDevice<feature::placeholder, feature::Out>;
  using AsyncReadWriteDevice = AsyncDevice<feature::In, feature::Out>;

  template <class... Flags>
  class Device;
  using ReadDevice = Device<feature::In, feature::placeholder>;
  using WriteDevice = Device<feature::placeholder, feature::Out>;
  using ReadWriteDevice = Device<feature::In, feature::Out>;

  template <>
  class Device<feature::placeholder, feature::placeholder> {
  public:
    explicit Device(NativeDevice &&dev) noexcept
        : _dev(std::make_shared<NativeDevice>(std::move(dev))) {}
    Device(std::shared_ptr<NativeDevice> dev) noexcept : _dev(std::move(dev)) {}
    Device(Device &&rhs) noexcept : _dev(std::move(rhs._dev)) {}
    Device &operator=(Device &&rhs) noexcept {
      _dev = std::move(rhs._dev);
      return *this;
    }
    ~Device() noexcept { LOG6("Device dtor, use count: {}", _dev.use_count()); }
    int raw() const noexcept { return _dev->raw(); }

  protected:
    std::shared_ptr<NativeDevice> _dev;
  };
  template <>
  class Device<feature::placeholder, feature::Out>
      : public Device<feature::placeholder, feature::placeholder> {
  private:
    using Base = Device<feature::placeholder, feature::placeholder>;

  public:
    using Base::Base;
    AsyncDevice<feature::placeholder, feature::Out> async(
        std::shared_ptr<sync::Poller> &poller) && noexcept;
  };

  template <>
  class Device<feature::In, feature::placeholder>
      : public Device<feature::placeholder, feature::placeholder> {
  private:
    using Base = Device<feature::placeholder, feature::placeholder>;

  public:
    using Base::Base;
    AsyncDevice<feature::In, feature::placeholder> async(
        std::shared_ptr<sync::Poller> &poller) && noexcept;
  };

  template <>
  class Device<feature::In, feature::Out>
      : public Device<feature::placeholder, feature::placeholder> {
  private:
    using Base = Device<feature::placeholder, feature::placeholder>;

  public:
    using Base::Base;
    std::tuple<ReadDevice, WriteDevice> split() && noexcept {
      return {ReadDevice{std::move(_dev)}, WriteDevice{std::move(_dev)}};
    }

    AsyncDevice<feature::In, feature::Out> async(std::shared_ptr<sync::Poller> &poller) && noexcept;
  };

  template <>
  class AsyncDevice<feature::In, feature::placeholder>
      : public Device<feature::In, feature::placeholder> {
  public:
    AsyncDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : Device(std::move(dev)), _sem(std::move(sem)) {}
    AsyncDevice(std::shared_ptr<NativeDevice> dev,
                std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : Device(std::move(dev)), _sem(std::move(sem)) {}
    AsyncDevice(AsyncDevice &&rhs) noexcept : Device(std::move(rhs)), _sem(std::move(rhs._sem)) {}
    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      Device::operator=(std::move(rhs));
      _sem = std::move(rhs._sem);
      return *this;
    }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
  };

  inline AsyncDevice<feature::In, feature::placeholder> Device<
      feature::In, feature::placeholder>::async(std::shared_ptr<sync::Poller> &poller) && noexcept {
    auto sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller->add(_dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
                sync::PollCallback<sync::IOM_EVENTS::IN>{sem});
    return {std::move(_dev), std::move(sem)};
  }

  template <>
  class AsyncDevice<feature::placeholder, feature::Out>
      : public Device<feature::placeholder, feature::Out> {
  public:
    AsyncDevice(NativeDevice &&dev, std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : Device(std::move(dev)), _sem(std::move(sem)) {}
    AsyncDevice(std::shared_ptr<NativeDevice> dev,
                std::shared_ptr<coro::CountingSemaphore<1>> sem) noexcept
        : Device(std::move(dev)), _sem(std::move(sem)) {}
    AsyncDevice(AsyncDevice &&rhs) noexcept : Device(std::move(rhs)), _sem(std::move(rhs._sem)) {}
    AsyncDevice &operator=(AsyncDevice &&rhs) noexcept {
      Device::operator=(std::move(rhs));
      _sem = std::move(rhs._sem);
      return *this;
    }

    coro::CountingSemaphore<1> &sem() noexcept { return *_sem; }

  protected:
    std::shared_ptr<coro::CountingSemaphore<1>> _sem;
  };

  inline AsyncDevice<feature::placeholder, feature::Out>
  Device<feature::placeholder, feature::Out>::async(
      std::shared_ptr<sync::Poller> &poller) && noexcept {
    auto sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller->add(_dev->raw(), sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
                sync::PollCallback<sync::IOM_EVENTS::OUT>{sem});
    return {std::move(_dev), std::move(sem)};
  }

  template <>
  class AsyncDevice<feature::In, feature::Out> : public Device<feature::In, feature::Out> {
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

  inline AsyncDevice<feature::In, feature::Out> Device<feature::In, feature::Out>::async(
      std::shared_ptr<sync::Poller> &poller) && noexcept {
    auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
    auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
    poller->add(
        _dev->raw(), sync::IOM_EVENTS::IN | sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
        sync::PollCallback<sync::IOM_EVENTS::IN, sync::IOM_EVENTS::OUT>{read_sem, write_sem});
    return {std::move(_dev), std::move(read_sem), std::move(write_sem)};
  }

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
  template <class Device>
    requires wheel::type_traits::existing_v<feature::In, Device>
  inline coro::Task<std::tuple<std::size_t, std::optional<RecvError>>> immediate_read(
      Device &dev, std::span<std::byte> buf) {
    using Result = std::tuple<std::size_t, std::optional<RecvError>>;
    ssize_t n;
    size_t offset = 0;
    while (true) {
      n = ::recv(dev.raw(), buf.data() + offset, buf.size() - offset, 0);
      LOG5("recv n: {}", n);
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          if (offset != 0) {
            LOG5("recv over");
            break;
          }
          LOG5("no data");
          if (!co_await dev.sem()) {
            co_return Result(offset, {{RecvErrorCategory::Unknown}});
          }
          continue;
        } else {
          LOG2("Failed to recv data, err : {}", strerror(errno));
          // TODO: handle recv error
          co_return Result(offset, {{RecvErrorCategory::Unknown}});
        }
      } else if (n == 0) {
        LOG5("recv eof");
        co_return Result(offset, {{RecvErrorCategory::Eof}});
      }
      LOG6("recv {} bytes", n);
      offset += n;
    };
    LOG5("end recv string");
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
    Closed,
  };

  class SendError {
  public:
    SendError(SendErrorCategory category) noexcept : category(category) {}
    std::string_view message() const noexcept {
      switch (this->category) {
        case SendErrorCategory::Unknown:
          return "Unknown error";
        case SendErrorCategory::Closed:
          return "Connection closed";
      }
      return "Unknown error";
    }
    SendErrorCategory category;
  };

  using SendResult = std::expected<void, SendError>;
  inline coro::Task<std::expected<void, SendError>> immediate_write(
      AsyncWriteDevice &dev, std::span<const std::byte> data) {
    while (true) {
      ssize_t n = ::send(dev.raw(), data.data(), data.size(), 0);
      if (static_cast<size_t>(n) == data.size()) {
        co_return {};
      }
      if (n > 0) {
        data = data.subspan(n);
        continue;
      }
      if (n == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)) {
        co_return std::unexpected{SendError{SendErrorCategory::Unknown}};
      }
      if (!co_await dev.sem()) {
        co_return std::unexpected{SendError{SendErrorCategory::Closed}};
      }
    }
  }
  inline coro::Task<std::expected<void, SendError>> immediate_write(AsyncWriteDevice &dev,
                                                                    std::filesystem::path path) {
    int ffd = open(path.c_str(), O_RDONLY);
    if (ffd == -1) {
      LOG2("open file failed");
      co_return std::unexpected(SendError{SendErrorCategory::Unknown});
    }
    NativeDevice file{ffd};
    struct stat st;
    if (fstat(file.raw(), &st) == -1) {
      LOG2("fstat failed");
      co_return std::unexpected(SendError{SendErrorCategory::Unknown});
    }
    off_t offset = 0;
    while (true) {
      ssize_t n = ::sendfile(dev.raw(), file.raw(), &offset, st.st_size);
      // TODO: handle sendfile error
      if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          LOG5("need write again");
          if (!co_await dev.sem()) {
            co_return std::unexpected(SendError{SendErrorCategory::Unknown});
          }
        } else {
          LOG2("[sendfile] Failed to send file");
          co_return std::unexpected(SendError{SendErrorCategory::Unknown});
        }
      } else if (n < st.st_size) {
        LOG5("[sendfile] send {} bytes", n);
        st.st_size -= n;
      }
    }
  }
}  // namespace io
SYS_NE
#endif
