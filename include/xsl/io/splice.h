#pragma once
#ifndef XSL_IO_SPLICE
#  define XSL_IO_SPLICE
#  include "xsl/ai.h"
#  include "xsl/coro.h"
#  include "xsl/io/def.h"
#  include "xsl/logctl.h"
#  include "xsl/wheel.h"

#  include <expected>

XSL_IO_NB

namespace impl_splice {
  template <class Executor = coro::ExecutorBase>
  Lazy<Result, Executor> splice_once(ABR& from, ABW& to, std::string& buffer) {
    auto [sz, err] = co_await from.read(xsl::as_writable_bytes(std::span(buffer)));
    if (err) {
      WARN("Failed to read data from the device, err: {}", std::make_error_code(*err).message());
      co_return {sz, err};
    }
    DEBUG("Read {} bytes from the device", sz);
    auto [s_sz, s_err] = co_await to.write(xsl::as_bytes(std::span(buffer).subspan(0, sz)));
    if (s_err) {
      WARN("Failed to write data to the device, err: {}", std::make_error_code(*s_err).message());
      co_return {s_sz, s_err};
    }
    DEBUG("Write {} bytes to the device", s_sz);
    co_return {s_sz, std::nullopt};
  }
}  // namespace impl_splice

template <class Executor = coro::ExecutorBase, PtrLike<ABR> FromPtr, PtrLike<ABW> ToPtr>
Lazy<Result, Executor> splice_once(FromPtr from, ToPtr to, std::string buffer) {
  return impl_splice::splice_once(*from, *to, buffer);
}

template <class Executor = coro::ExecutorBase, PtrLike<ABR> FromPtr, PtrLike<ABW> ToPtr>
Lazy<Result, Executor> splice(FromPtr from, ToPtr to, std::string buffer) {
  std::size_t total = 0;
  while (true) {
    auto [sz, err] = co_await impl_splice::splice_once(*from, *to, buffer);
    if (err) {
      co_return {total, err};
    }
    total += sz;
  }
  DEBUG("Spliced {} bytes", total);
}

namespace impl_splice {
  template <class... Flags>
  class Splice;

  template <class... Flags>
  using SpliceCompose = feature::organize_feature_flags_t<
      Splice<feature::Item<type_traits::is_same_pack, feature::In<void>, feature::Out<void>,
                           feature::InOut<void>>,
             feature::Dyn>,
      Flags...>;

  template <class T, PtrLike<ABR> FromPtr>
  class Splice<feature::In<byte>, T, FromPtr>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>, ai::AsyncWritable<byte>,
                                  feature::placeholder> {
  public:
    using value_type = byte;

    Splice(FromPtr from) : _from(std::move(from)) {}
    Splice(Splice&&) = default;
    Splice& operator=(Splice&&) = default;
    ~Splice() = default;

    Task<Result> write(ABW& awd) {
      co_return co_await splice(std::move(_from), &awd, std::string(4096, '\0'));
    }

  private:
    FromPtr _from;
  };

  template <class T>
  class Splice<feature::In<byte>, T>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>, ai::AsyncWritable<byte>,
                                  feature::placeholder> {
  public:
    template <PtrLike<ABR> FromPtr>
    static decltype(auto) make_unique(FromPtr from) {
      return std::make_unique<Splice<feature::In<byte>, T, FromPtr>>(std::move(from));
    }
  };

}  // namespace impl_splice
/**
@brief Splice the data from the input device to the output device

@tparam Flags, <<feature::In<byte>, feature::Out<byte>, feature::InOut<byte>>,
feature::Dyn>
 */
template <class... Flags>
using Splice = impl_splice::SpliceCompose<Flags...>;

XSL_IO_NE
#endif
