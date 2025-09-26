/**
 * @file def.h
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Common channel for coroutines
 * @version 0.1.0
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef XSL_CORO_CHANNEL_DEF
#  define XSL_CORO_CHANNEL_DEF
#  include <xsl/coro/def.h>

#  include <cassert>

XSL_CORO_NB

template <class Storage>
struct ChannelAwaiterTraits;

template <class Storage>
class ChannelAwaiter : public ChannelAwaiterTraits<Storage> {
  friend struct ChannelAwaiterTraits<Storage>;

private:
  Storage &storage;

public:
  template <class _Storage>
    requires(!std::same_as<std::remove_cvref_t<_Storage>, ChannelAwaiter>)
  explicit constexpr ChannelAwaiter(_Storage &&storage)
      : storage(std::forward<_Storage>(storage)) {}
  constexpr ChannelAwaiter(ChannelAwaiter &&) = default;
  constexpr ChannelAwaiter &operator=(ChannelAwaiter &&) = default;
};

template <class Storage, std::size_t MaxElements>
struct ChannelTraits;

/// @brief Signal sender
template <class StorageType, std::size_t MaxElements>
class AnyChannel : public ChannelTraits<StorageType, MaxElements>,
                    public ChannelAwaiterTraits<StorageType> {
public:
  using awaiter_type = ChannelAwaiter<StorageType>;

private:
  friend struct ChannelTraits<StorageType, MaxElements>;
  friend struct ChannelAwaiterTraits<StorageType>;
  StorageType storage = {MaxElements};

public:
  constexpr AnyChannel() = default;
  template <class _Storage>
    requires(!std::same_as<std::remove_cvref_t<_Storage>, AnyChannel>)
  constexpr AnyChannel(_Storage &&storage) : storage(std::forward<decltype(storage)>(storage)) {}
  constexpr AnyChannel(AnyChannel &&) = default;
  constexpr AnyChannel &operator=(AnyChannel &&) = default;
  constexpr ~AnyChannel() {}
  awaiter_type operator co_await() { return awaiter_type(storage); }
};

XSL_CORO_NE
#endif
