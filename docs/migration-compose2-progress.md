# compose2 迁移进展

## 目标

用 `shared_memory` + `LocalComosite2` 替代 `SharedComosite` + `StorageMixin`。

## 新架构（当前实现）

```
shared_memory<T>                    — 单类型共享所有权 (atomic refcount)
  ├── emplace(args...) → T&       — 原地重建 T（自主管理，不先析构）
  ├── destroy()                    — 析构
  ├── get() → T*                   — 直接访问 T
  ├── operator*() → T&             — 解引用
  └── operator->() → T* | Chain    — 链式代理：T 有 operator->() 时链式调用

LocalComosite2<Alloc, S...> : public S...
  ├── : S...                       — 直接继承所有 Parts（无 BaseOn，无嵌套 Inner）
  ├── emplace<Part>(args...)       — 原地重建 Part（自主管理）
  ├── destroy<Part>()              — 析构 Part
  └── abandon<Parts...>()          — 批量析构
```

## 已完成

### 1. compose2.h — 核心框架
- `shared_memory<T>`: atomic refcount + chainable operator-> + emplace/destroy
- `LocalComosite2`: 直接继承 S... + emplace<Part>/destroy<Part>/abandon
- `LocalCompose2`: select_feature_flags_t alias

### 2. shared_memory 去 Alloc
- `shared_memory<T>` 单模板参数，用 `new`/`delete` 替代 allocator
- 删除 `_detail::ControlBlock<Alloc>`
- 更新所有 `shared_memory<std::allocator<void>, T>` → `shared_memory<T>`

### 3. 链式 operator->
- `shared_memory::operator->()` 在 T 有 `operator->()` 时返回链式代理
- examples、http 模块全部适配 `dev->method()`

### 4. StorageMixin 合并到 Storage 类型
- `StaticExactPubSubStorage` — 合并 PubSubUtil + signal<>() + for_each()
- `ExactPubSubStorage` — 从 alias 改为 struct，合并 PubSubUtil + 构造函数
- 删除 `StorageMixin<StaticExactPubSubStorage<...>>` 特化
- 删除 `StorageMixin<ExactPubSubStorage<...>>` 特化
- 删除 `StorageMixin<RawOwner>` 特化
- `IOSignalStorage` 直接用作 Part 类型

### 5. SPSCSignal 迁移
- `SPSCSignal4 = LocalCompose2<Awaiter4, SPSCSignalStorage4>`
- `SPSCSignal2 = SPSCSignalStorage2<MaxSignals>` — 简化为直接类型

### 6. socket.h 迁移
- `AsyncSocket` / `DynAsyncSocket` → `shared_memory<T>`
- `DefaultEpollWrapper` 替代 `SocketEpollWrapper`
- `is_connection_based` 在 `AsyncConnectionUtils` 主模板和偏特化中显式提供

### 7. tls.h 迁移
- `TLSLayer` / `DynTLSLayer` → `shared_memory<LocalCompose2<...>>`
- `into_dyn()` 已注释

### 8. io.h 适配
- `init_async_device` 适配 `shared_memory<T>` 单参数
- `DefaultEpollWrapper` 只检查 `std::derived_from<Inner, RawOwner>`

### 9. 测试
- `test/unit/compose2.cpp`: 18 tests
- 24/24 全测试通过
