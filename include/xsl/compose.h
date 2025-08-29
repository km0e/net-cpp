/**
 * @file compose.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Compose utilities
 * @version 0.1.0
 * @date 2025-09-06
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_WHEEL_COMPOSE
#  define XSL_WHEEL_COMPOSE
#  include <xsl/concept.h>
#  include <xsl/def.h>
#  include <xsl/error.h>
#  include <xsl/feature.h>
#  include <xsl/type_traits.h>

XSL_NB

template <class T, class Storage>
concept SharedStorageAccessor = requires(T t) {
  { t.get() } -> std::convertible_to<Storage*>;
};

template <class S>
struct StorageUtils {};

template <class Wrapper, class Base, class... S>
class SharedStorage;

template <class... Base, class... S>
class SharedStorage<Placeholder, BaseOn<Base...>, S...> : public Base... {
  struct Inner : StorageUtils<S>... {};

  std::shared_ptr<Inner> inner_;

public:
  SharedStorage() : inner_(std::make_shared_for_overwrite<Inner>()) {}
  SharedStorage(const SharedStorage&) = default;
  SharedStorage(SharedStorage&&) noexcept = default;
  SharedStorage& operator=(const SharedStorage&) = default;
  SharedStorage& operator=(SharedStorage&&) noexcept = default;

  Inner* get() { return inner_.get(); }
  // auto&& operator*() { return *inner_; }
  auto operator->(this auto&& self) { return self.inner_.get(); }
};
template <class Interface>
struct SharedDynamicUtil {
  std::shared_ptr<Interface> into_dyn(this auto&& self)
    requires std::is_reference_v<decltype(self)>
  {
    return std::move(self.inner_);
  }
};
template <template <class> class W, class... Base, class... S>
class SharedStorage<Wrapper<W>, BaseOn<Base...>, S...> : public Base... {
  struct Inner : StorageUtils<S>... {};
  template <class Interface>
  friend struct SharedDynamicUtil;

  std::shared_ptr<W<Inner>> inner_;
  static_assert(requires(W<Inner> w) {
    { w.get() } -> std::convertible_to<Inner*>;
  });

public:
  SharedStorage()
      : inner_(std::shared_ptr<W<Inner>>(static_cast<W<Inner>*>(std::malloc(sizeof(W<Inner>))),
                                         free)) {}
  SharedStorage(const SharedStorage&) = default;
  SharedStorage(SharedStorage&&) noexcept = default;
  SharedStorage& operator=(const SharedStorage&) = default;
  SharedStorage& operator=(SharedStorage&&) noexcept = default;

  Inner* get() { return inner_.get()->get(); }
  // auto&& operator*() { return *inner_; }
  auto operator->(this auto&& self) { return self.inner_->get(); }
};

template <class... S>
using SharedStorageCompose = select_feature_flags_t<
    SharedStorage<Item<is_same_pack<Placeholder>, Wrapper<>>, Item<is_same_pack<BaseOn<>>>, Rest>,
    S...>;

XSL_NE

#endif
