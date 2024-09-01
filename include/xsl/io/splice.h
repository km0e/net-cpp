/**
 * @file splice.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Splice the data from the input device to the output device
 * @version 0.11
 * @date 2024-08-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#ifndef XSL_IO_SPLICE
#  define XSL_IO_SPLICE
#  include "xsl/coro.h"
#  include "xsl/feature.h"
#  include "xsl/io/def.h"
#  include "xsl/logctl.h"
#  include "xsl/type_traits.h"
#  include "xsl/wheel.h"

#  include <expected>

XSL_IO_NB

namespace impl_splice {
  template < ABRL ABR, ABWL ABW>
  Task<Result> splice_once(ABR& from, ABW& to, std::string& buffer) {
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

template < ABRL ABR, ABWL ABW>
Task<Result> splice_once(ABR* from, ABR* to, std::string buffer) {
  return impl_splice::splice_once(*from, *to, buffer);
}

template < ABRL FromPtr, ABWL ToPtr>
Task<Result> splice(FromPtr* from, ToPtr* to, std::string buffer) {
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

// namespace impl_splice {
//   template <class... Flags>
//   class Splice;

//   template <class... Flags>
//   using SpliceCompose
//       = organize_feature_flags_t<Splice<Item<is_same_pack, In<void>, Out<void>, InOut<void>>, Dyn>,
//                                  Flags...>;

//   template <class T, PtrLike<ABR> FromPtr>
//   class Splice<In<byte>, T, FromPtr>
//       : public std::conditional_t<std::is_same_v<T, Dyn>, ai::AsyncWritable<byte>, Placeholder> {
//   public:
//     using value_type = byte;  ///< the value type
//     /**
//      * @brief Construct a new Splice object
//      *
//      * @param from the input device pointer
//      */
//     Splice(FromPtr from) : _from(std::move(from)) {}
//     Splice(Splice&&) = default;             ///< move constructor
//     Splice& operator=(Splice&&) = default;  ///< move assignment
//     ~Splice() = default;
//     /**
//      * @brief write the data from the input device to the output device
//      *
//      * @param awd the byte writer device
//      * @return Task<Result>
//      */
//     Task<Result> write(ABW& awd) {
//       co_return co_await splice(std::move(_from), &awd, std::string(4096, '\0'));
//     }

//   private:
//     FromPtr _from;
//   };

//   template <class T>
//   class Splice<In<byte>, T>
//       : public std::conditional_t<std::is_same_v<T, Dyn>, ai::AsyncWritable<byte>, Placeholder> {
//   public:
//     /**
//      * @brief Unique Splice object construct helper
//      *
//      * @tparam FromPtr
//      * @param from
//      * @return decltype(auto)
//      */
//     template <PtrLike<ABR> FromPtr>
//     static decltype(auto) make_unique(FromPtr from) {
//       return std::make_unique<Splice<In<byte>, T, FromPtr>>(std::move(from));
//     }
//   };

// }  // namespace impl_splice
// /**
// @brief Splice the data from the input device to the output device

// @tparam Flags, <<In<byte>, Out<byte>, InOut<byte>>,
// Dyn>
//  */
// template <class... Flags>
// using Splice = impl_splice::SpliceCompose<Flags...>;

XSL_IO_NE
#endif
