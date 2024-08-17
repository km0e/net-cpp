#pragma once
#ifndef XSL_SYS_IO_DEV
#  define XSL_SYS_IO_DEV
#  include "xsl/sys/io/def.h"
#  include "xsl/sys/raw.h"

#  include <unistd.h>

#  include <cassert>
#  include <tuple>
#  include <utility>
XSL_SYS_IO_NB
class RawDeviceOwner {
public:
  RawDeviceOwner(int fd) noexcept : _dev(std::make_shared<RawDevice>(fd)) {}

  template <class... Args>
  RawDeviceOwner(Args &&...args) noexcept : _dev(std::forward<Args>(args)...) {}

  RawDeviceOwner(RawDeviceOwner &&rhs) noexcept : _dev(std::move(rhs._dev)) {}

  RawDeviceOwner &operator=(RawDeviceOwner &&rhs) noexcept {
    _dev = std::move(rhs._dev);
    return *this;
  }

  ~RawDeviceOwner() noexcept {}

  int raw() const noexcept { return _dev->raw(); }

protected:
  std::shared_ptr<RawDevice> _dev;

  template <class Derived>
  std::tuple<std::shared_ptr<RawDevice>, std::shared_ptr<RawDevice>> split(
      this Derived self) noexcept {
    assert(self._dev != nullptr && "dev is nullptr");
    return {self._dev, self._dev};
  }
};
XSL_SYS_IO_NE
#endif
