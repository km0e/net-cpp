#pragma once
#ifndef XSL_IO_SPLICE
#  define XSL_IO_SPLICE
#  include "xsl/ai/dev.h"
#  include "xsl/coro/lazy.h"
#  include "xsl/io/def.h"

#  include <expected>
#  include <memory>

XSL_IO_NB
template <class Executor = coro::ExecutorBase>
coro::Lazy<ai::Result> splice(ai::AsyncDevice<feature::In<std::byte>>& from,
                              ai::AsyncDevice<feature::Out<std::byte>>& to, std::string& buffer) {
  std::size_t total = 0;
  while (true) {
    auto [sz, err] = co_await from.read(std::as_writable_bytes(std::span(buffer)));
    if (err) {
      co_return {total, err};
    }
    auto [s_sz, s_err] = co_await to.write(std::as_bytes(std::span(buffer).subspan(0, sz)));
    if (s_err) {
      co_return {total, s_err};
    }
    total += s_sz;
  }
}
namespace impl_splice {
  template <class... Flags>
  class Splice;

  template <class... Flags>
  using SpliceCompose = feature::origanize_feature_flags_t<
      Splice<feature::Item<wheel::type_traits::is_same_pack, feature::In<void>, feature::Out<void>,
                           feature::InOut<void>>,
             feature::Dyn>,
      Flags...>;

  template <class T>
  class Splice<feature::In<std::byte>, T>
      : public std::conditional_t<std::is_same_v<T, feature::Dyn>, ai::AsyncWritable<std::byte>,
                                  feature::placeholder> {
  public:
    using value_type = std::byte;

    Splice(std::shared_ptr<ai::AsyncDevice<feature::In<value_type>>> from) : _from(from) {}
    Splice(Splice&&) = default;
    Splice& operator=(Splice&&) = default;
    ~Splice() = default;

    coro::Task<ai::Result> write(ai::AsyncDevice<feature::Out<value_type>>& awd) {
      std::string buffer;
      co_return co_await splice(*_from, awd, buffer);
    }

  private:
    std::shared_ptr<ai::AsyncDevice<feature::In<value_type>>> _from;
  };
}  // namespace impl_splice
/**
@brief Splice the data from the input device to the output device

@tparam Flags, <<feature::In<std::byte>, feature::Out<std::byte>, feature::InOut<std::byte>>,
feature::Dyn>
 */
template <class... Flags>
using Splice = impl_splice::SpliceCompose<Flags...>;

XSL_IO_NE
#endif
