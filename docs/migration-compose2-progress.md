# compose2 迁移进展

## 目标

用 `shared_memory` + `LocalComposite2` 替代 `SharedComosite` + `StorageMixin`。

## 新架构（当前实现）

```
shared_memory<T>                    — 单类型共享所有权 (atomic refcount)
  ├── emplace(args...) → T&       — 原地重建 T（自主管理，不先析构）
  ├── destroy()                    — 析构
  ├── get() → T*                   — 直接访问 T
  ├── operator*() → T&             — 解引用
  └── operator->() → T* | Chain    — 链式代理：T 有 operator->() 时链式调用

LocalComposite2<Alloc, S...> : public S...
  ├── : S...                       — 直接继承所有 Parts（无 BaseOn，无嵌套 Inner）
  ├── emplace<Part>(args...)       — 原地重建 Part（自主管理）
  ├── destroy<Part>()              — 析构 Part
  └── abandon<Parts...>()          — 批量析构
```

整体架构说明见 [architecture.md](architecture.md)。

## 已完成

### 1. compose2.h — 核心框架
- `shared_memory<T>`: atomic refcount + chainable operator-> + emplace/destroy
- `LocalComposite2`: 直接继承 S... + emplace<Part>/destroy<Part>/abandon
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

### 10. 协程运行时修复（迁移收尾时实证发现）

以下问题均在集成测试（`it_http_compare`、`it_bind`）与基准验证中实证，修复后
ctest 26-27/27 通过（残留项见下文"未完成"）。

| # | 位置 | 问题 | 修复 |
|---|------|------|------|
| 1 | `coro/core/context.h` | `Reserved::await_ready()` 恒 true，`await_suspend` 永不执行 → `co_await CurrentIOContext` 返回空引用（example http_server、it_bind 100% 段错误的根因） | 新增 `await_resume(CoroContext&)`，`AwaiterWrapper` 优先选择 ctx 感知重载 |
| 2 | `coro/core/context.h` | `new_child_context()` 以 `decltype(auto)` 返回临时对象的右值引用（悬垂），且未继承 `_reserved` | 按值返回并继承 `_reserved` |
| 3 | `coro/core/task.h` | `Task::detach()` 无参调用构造空 `Rc<CoroContext>` → dispatch 到空上下文段错误 | `static_assert(sizeof...(Args) != 0)`，修正 `it_bind` 调用点 |
| 4 | `coro/core/detach.h` | `DetachPromiseBase::final_suspend()` 手动 `_self.destroy()` + `suspend_never` → 双重释放 | 移除手动 destroy（runtime 在 final_suspend 完成后自动销毁） |
| 5 | `coro/core/task.h` + `wheel/rc.h` | `Task::block()` 无条件 `by(CoroContext{})` 覆盖 `.by(ctx)` 设置的上下文 | `Rc` 增加显式 bool 转换 + `has_ctx()`，仅在无上下文时设默认值 |
| 6 | `coro/core/block.h` | 重写：Task 归属调用方帧 + 无 `co_await` 的 shell 协程在 final_suspend 释放信号量。绕开 GCC 16 协程临时量生命周期缺陷（await 表达式临时量跨恢复引用/双重析构）；注意协程体内没有 `co_return`/`co_await` 就不是协程（GCC 静默按普通函数编译） | 手动驱动 `await_suspend` + `BlockShell`（`FinalAwaiter::await_suspend` 中 release） |
| 7 | `asio/net/socket.h` + `asio/io.h` | 连接关闭后 fd 永不释放：IOContext handlers map 持有 handler 引用，析构永不触发 | `DefaultEpollWrapper::bind_context()/deregister()`，`imm_serve_connection` 结束时注销；`run()` 事件派发对已注销 fd 安全跳过（pin + find） |
| 8 | `sys/io/context/epoll.h` | `shutdown()` 的 `NONE` 事件 publish 谓词匹配不到任何 key → 等待者永不唤醒；且不清理 handlers map（监听 fd 泄漏、端口无法复用） | `EpollHandler::shutdown_notify()` 唤醒全部订阅者 + 清空 map + `stopped` 原子标志（在唤醒**之前**设置，防任务重新挂起） |
| 9 | `asio/http/server.h` | accept 循环出错时无限 `continue`：shutdown 后死循环占用监听 fd、烧 CPU | `stopped()` / 非瞬态错误 → `co_return` |
| 10 | `sys/net/socket.h` + `asio/net/socket.h` | accepted socket 无 `TCP_NODELAY`：keep-alive 下每个响应被客户端 delayed ACK 卡住 ~40ms（25 RPS/连接，**1000× 退化**，基准实测） | `SocketOptions::no_delay()`（TCP-only），accept 流程默认启用 |

## 未完成 / 后续建议

按对基准（`docs/benches/http.md`，xsl ~105K RPS vs asio ~400K RPS @ 4×4）的影响排序。

### 1. 响应构建零拷贝（预估收益最大）
- `ResponseBuilder::sendto` 先把 head 拼进 `std::string`（`reserve(1024)`），head 与 body **分两次 `write()`**；body 又是 `std::function<Task<Result>(W&)>` 捕获 `std::string` 拷贝
- 方向：head/body 拼接后单次 `writev`；header 用 `string_view`/静态表；`to_date_string`（每次 `std::format`）按秒缓存
- 文件：`include/xsl/asio/http/response.h`、`src/asio/http/response.cpp`

### 2. 执行器生命周期管理
- `shutdown()` 唤醒并停止 poller，但**无法等待在途 detached 任务销毁完成**（thread-per-dispatch 的派发线程无人 join）
- 后果：fixture/ctx 先于 detached 任务帧销毁 → 对同一 `CoroContext` 的 Rc 计数无序 `--`（丢失更新 → 泄漏；与并发 `++` 竞争 → 潜在 UAF）
- 方向：执行器跟踪在途派发数，`shutdown` 时 drain；或 detached 任务完成后回调注册表
- 关联：`NewThreadExecutor::schedule` 每次唤醒 `std::thread(...).detach()`（`src/coro/core/executor.cpp`）

### 3. 唤醒调度模型（实测后优先级下调）
- 实测（`docs/benches/http.md`）：`NoopExecutor`（内联）vs `NewThreadExecutor`（每唤醒一线程）——1×1 持平（19.2K vs 20.5K），4×4 内联反而 -20%（单核钉死）
- 结论：**dispatch 不是当前瓶颈**（每请求 ~50µs 固定成本中线程创建占比小），先做 #1；多核扩展用线程池（`ThreadPoolExecutor` 已实现未启用，`executor.cpp`）或多 poller
- 前置：Rc 原子化（#8）、shutdown 收拢（#2）

### 4. HTTP 协议补全
- header 大小写不敏感（RFC 9110 §5.1）：`us_map` 是普通 `unordered_map`（`wheel/utils.h`），`Connection: close` 仅精确大小写匹配生效
- 无 handler 的路由命中（如 `POST /hello` 只注册了 GET）→ `rt_assert(false)` 崩溃，应返回 405（`asio/http/service.h`）
- 请求体：`Message::read` 过读的字节无法回退（`parsed_end` 是局部变量），POST body 会破坏 keep-alive（`asio/http/request.h`、`net/http/msg.h` 有 TODO）

### 5. 构建一致性
- CMake 定义的日志宏是 `QUILL_COMPILE_ACTIVE_XSL_LOG_LEVEL`，而 `xsl/log.h` 只读 quill 的 `QUILL_COMPILE_ACTIVE_LOG_LEVEL`（未定义时 quill 默认 -1 = 全部编译）→ CMake 侧 `XSL_LOG_LEVEL` 实际不生效；xmake 侧用 `--log_level=none` 是生效的
- 两侧宏名统一即可（`CMakeLists.txt`）

### 6. Rc 引用计数原子化（廉价加固）
- `wheel/rc.h` 的 `ref_count` 是普通 `size_t`；await 链（`NextBase::next`）会把父协程的 `Rc<CoroContext>` 拷贝进子任务 promise，增减发生在不同线程
- 实测热路径上这些操作被信号握手链（acq_rel exchange）+ 线程创建同步排序，**未观察到竞争**；但 detached 任务与持有者的销毁顺序无同步（见 #2），边缘场景存在丢失更新 → 泄漏 / UAF
- `fetch_add(relaxed)` / `fetch_sub(acq_rel)` + fence，几行改动
- **已决议：不原子化**。安全性由"单 Inner 单链"所有权不变量保证（`docs/architecture.md` §3.5）；`detach()`/`by()` 以契约注释 + debug 断言（`Rc::unique()`）落实，违规调用点已修正为 `detach(*ctx)`

### 7. 已知残留测试问题
- ~~`it_bind` 偶发慢启动~~ **已根治**：真正根因是 listen socket 在 listen() 之前注册进 poller，pre-listen 的 `EPOLLOUT|EPOLLHUP` 事件触发 handler 的 DELETE 分支将设备永久注销（与调度延迟无关）。修复：`AsyncSocketCreator::cb` 先 listen 再注册（并行 12 轮全绿）
- `connect.cpp` 仍被注释（`io.h` recv span 重载缺失，见该文件内注释）

## 附：验证入口

- 正确性对比：`it_http_compare`（ctest）
- 性能对比与实测数据：`docs/benches/http.md`、`test/benches/http/bench.sh`
- 架构与线程模型：`docs/architecture.md`
