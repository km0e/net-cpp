# Xsl 架构总览

本文描述代码的**实际结构**(以本仓库当前实现为准),重点是协程运行时的控制流、
所有权模型与线程模型。模块依赖图见 `CLAUDE.md`;性能数据见
[benches/http.md](benches/http.md);compose2 迁移历史与未完成项见
[migration-compose2-progress.md](migration-compose2-progress.md)。

## 1. 分层

```
xsl_asio   异步 I/O：HTTP 服务/客户端、TLS、pipe
  └── xsl_coro    协程运行时：Task/Promise/Executor/CoroContext/Signal/Channel
        └── xsl_sys     系统封装：epoll IOContext、socket、sockaddr
              └── xsl_wheel   基础设施：Rc、类型工具、字符串
xsl_net    协议解析（无 I/O）：HTTP proto、DNS、URI、router
```

依赖方向严格向下；`xsl_net` 不依赖 `xsl_coro`/`xsl_sys`，可独立测试。

## 2. 组合模型（compose2）

异步设备不是类继承树，而是** Parts 的运行时组合**：

```
AsyncSocket<Traits>
  = DirectAsyncReadWriteWrapper<shared_memory<DefaultEpollWrapper<LocalComposite<
        std::allocator<void>, sys::RawOwner, StaticExactPubSubStorage<IOM_EVENTS, IOSignal, IN, OUT>,
        AsyncConnectionUtils<Traits>, AsyncDeviceUtil, NetAsyncRx, NetAsyncTx>>>>
```

- `shared_memory<T>`：原子引用计数的堆对象；`operator->` 在 T 也有
  `operator->` 时返回链式代理（`dev->read()` 穿透到最内层 Part）
- `LocalComposite::emplace<Part>(args...)`：只重建一个 Part 的存储，
  其余 Part 的成员不受影响（`init_async_device` 依赖此语义先建 fd 再绑上下文）
- 生命周期铁律：**IOContext 的 handlers map 持有 handler 的一份引用**，
  引用计数不归零则 fd 不关闭。连接式设备结束服务时必须显式
  `deregister()`（`DefaultEpollWrapper`），否则 fd 泄漏

`Rc<T>`（wheel/rc.h）是另一个引用计数指针，用于协程上下文，
其 `ref_count` 是**有意非原子**的（安全性由 §3.5 的所有权不变量保证）；
`shared_memory` 的计数是原子的。两者不要混淆。

## 3. 协程运行时（xsl_coro）

### 3.1 三种协程 promise

| 类型 | initial_suspend | final_suspend | 用途 |
|------|-----------------|---------------|------|
| `Promise<TaskPromiseBase<T>>` | always | `final_awaiter` → 对称转移回 `_next` | `Task<T>`，可 co_await |
| `DetachPromiseBase<T>` | always | **never**（帧在完成时由 runtime 销毁） | fire-and-forget（`detach()`） |
| `BlockPromise` | never | never | （已移除）旧 `block()` 实现，现由 `_detail::BlockShell` 取代，见 §3.4 |

关键机制：

- **ctx 传播**：`Promise::await_transform` 把所有 awaiter 包进
  `AwaiterWrapper(awaiter, ctx)`；Task→Task await 时 `NextBase::next()`
  把父协程的 `Rc<CoroContext>` 拷贝进子任务 promise（`_next` 记录续体）——
  **仅当子任务未显式 `.by(ctx)`**（与 `block()` 的“不覆盖”策略一致）
- **`Reserved<T>`**：`co_await CurrentIOContext` 取 promise ctx 的
  `_reserved`（即 IOContext），经 `await_resume(CoroContext&)` 同步返回，
  不挂起
- **对称转移**：Task 完成时 `final_awaiter::await_suspend` 返回 `_next`，
  不经过调度器直接恢复等待者 —— 同一等待链的恢复是 O(1) 栈展开

### 3.4 `block()`：手动驱动 + Shell 协程

`block()` 需要在普通线程上同步等待一个 Task。实现（`coro/core/block.h`）：

1. Task 移入 `block()` 自身帧（所有权在调用方，shell 经引用读结果）；
2. 创建 `BlockShell` 协程（`initial_suspend = suspend_always`，不自动运行）；
3. 手动调用 `task.await_suspend(shell.handle)`（把 shell 记为续体）并
   `resume()` 返回的句柄 —— Task 在当前线程内联运行至挂起；
4. Task 完成时经对称转移进入 shell，shell 体为空（**无 await 表达式**，
   规避 GCC 16 协程临时量生命周期缺陷），其 `final_suspend` 的
   `await_suspend` 释放信号量（此时 shell 已完全挂起，阻塞方销毁帧是安全的）；
5. 调用方 `sem.acquire()` 后直接 `task.await_resume()` 读结果。

若 Task 在首次内联运行中抛出异常（未达 final suspend，shell 未运行），
异常在调用线程重抛，信号量跳过 acquire。

### 3.2 执行器与线程模型

```
CoroContext::dispatch(f) → ExecutorBase::schedule(f)
  NoopExecutor        f() 直接调用（在唤醒者线程内联执行）
  NewThreadExecutor   std::thread(f).detach()   // 每次唤醒一个新线程
  ThreadPoolExecutor  任务队列 + N worker（已实现，未启用）
```

**并发恢复义务（[expr.await]、cppreference 原文核对）**：协程在**进入**
`await_suspend` 之前即"被视为已挂起"，因此另一线程在 `await_suspend`
仍在执行时 resume 它是**标准明确允许的场景**（不是 UB），代价是四条义务：
① await_suspend 在把 handle 发布给其他线程后**不得再访问 awaiter 成员**
（协程可能已被恢复并销毁 awaiter）；② handle 发布用 release、恢复用
acquire；③ 同一协程不得被并发 resume（恢复权须原子独占）；④ 跨执行代理
恢复仅在代理为 `std::thread`/`jthread`/main 时良定义。本库原语已落实全部
义务（发布仅经 exchange、发布后不碰帧内成员、WAITING 槽 exchange 转移
恢复权、恢复均发生于 std::thread），并在代码中以 RULE 注释钉死①。

**执行器选择**：`ThreadPoolExecutor` 是当前推荐的多线程执行器（经
`shared_ptr` 注入）；`NewThreadExecutor` 仅测试/调试（每 dispatch 一个
无界 detached 线程）；串行化执行器（strand 语义，有序性保证与进一步
降开销）列为后续可选优化，非正确性必需。

- detached 任务的 promise 持有 `Rc<CoroContext>`，其完成线程与创建线程
  由信号握手（`SPSCSignal4` 的 acq_rel exchange）+ 线程创建同步排序
- `IOContext::shutdown()` 唤醒全部等待者并清理 handlers，但**不等待
  detached 任务帧销毁**（thread-per-dispatch 的派发线程无人 join）——
  拆除时对同一上下文的 Rc 计数操作在边缘情况下无序，见迁移文档 #2/#6

### 3.3 信号（唤醒原语）

信号族已收敛为**一个状态机、两个竞争档**（`coro/signal/core.h`）：

| 档 | 竞争模型 | 同步 |
|------|----------|------|
| `UnsafeSignal` | 单线程 | 零同步（~0.5ns/往返） |
| `MPSCSignal` | N 生产 / 1 消费（SPSC 为退化情形） | 单个 `atomic<uintptr_t>`（~7ns/往返） |

两档语义一致：四态状态机 `0=idle / 1=signaled / 2=stopped / ptr=waiting`，
`await_resume` 在 signal 时返回 true、stop 后返回 false；**stop 为粘性
终态**，之后的 release 被忽略、之后的 await 立即返回 false。

- `MPSCSignal::await_suspend`：装入续体指针；竞态中发现已 signaled/stopped
  时经**第二次 exchange 原子回收**续体（拿到指针者拥有指针），杜绝
  store+delete 式回收的 UAF
- `release`/`stop`：exchange 转移续体所有权后调用（续体经
  `promise.resume` → `ctx->dispatch` 恢复协程）；release 多生产者安全，
  并发释放合并，伪唤醒由消费端重查过滤
- `UnsafeSignal` 的续体是内联成员（无堆分配），调用前**先移出**——回调
  会内联恢复消费者，可能立即重新 await 并改写该成员

### 3.6 取消模型（std::stop_token + 自动取消）

`CoroContext` 持 `std::stop_source`；其拷贝（detach/by 时产生）共享 stop
状态（内部原子引用计数），因此**同一 CoroContext 拷贝族 = 一个取消域**，
天然支持多回调。`ctx.cancel()` = `request_stop()`：所有已注册的
stop_callback 在调用线程同步执行。

**自动取消**：`AwaiterWrapper`（await_transform 拦截点）对满足
`ForceReleasable`（有跨线程 `release()`）且 `await_suspend`/`await_resume`
返回 bool 的 awaiter 自动注册 stop_callback——**plain `co_await sig` 即可
取消**，不再需要 `cancellable()` 包装（已删除）：

- 注册发生在 `await_suspend`（真正挂起才付出，`await_ready` 快路径零成本；
  注册+注销 ≈ 22ns @ libstdc++）；注销 RAII，不存在泄漏路径
- `await_resume`：先注销，再查 `stop_requested()`——**取消赢竞态**（D2：
  返回值传播，不抛异常）
- 注销仅在回调正在**别的线程**执行时阻塞——NoopExecutor 内联恢复时回调
  就在本线程，不会自我死锁（有测试钉死）

**D1（取消域继承）**：`co_yield` 分叉用 `new_child_context()`，**共享**父域
（父取消 → 连接级子任务一并取消）；逃逸口为 `new_independent_context()`。

**IO 绑定（Phase 4）**：`IOContext` 持 stop_source；`asio_ctx()` 把它绑进
CoroContext ⇒ `IOContext::shutdown()` 域级取消所有自动可取消的 IO await
（与信号的粘性 stop 双保险）；`IOContext::cancel_contexts()` 可单独取消
而不拆除 poller。仅原子档可取消（取消线程相当于额外生产者）。
**acq_rel 语义使"await 挂起前的副作用（如 Rc 计数 ++）对恢复线程可见"**，
这是热路径上引用计数操作无需额外同步的原因。

### 3.5 `Rc<CoroContext>` 所有权不变量（线程安全契约）

`Rc` 的 `ref_count` 非原子，其线程安全不依赖原子操作，而依赖以下设计原则：

> **一个 `Rc<CoroContext>` 的 Inner 同一时刻只属于一条协程链。**
> 跨任务/跨链传递上下文必须传 `CoroContext`（拷贝 → 生成新 Inner），
> 禁止把 `Rc<CoroContext>` 本体传给另一个任务的 `detach()`/`by()`。

链内跨线程迁移时，计数 ++/-- 全部有序，无需原子化：

- **对称转移**：Task 完成经 `final_awaiter` 直接恢复等待者，一条链上任意
  时刻只有一个线程在执行，计数增减由程序序排序；
- **信号握手**：链经 `SPSCSignal4` 挂起/唤醒时，`release` 与
  `await_suspend` 之间的 acq_rel exchange 建立 happens-before，挂起前的
  计数操作对恢复线程可见（§3.3）；
- **线程创建**：`NewThreadExecutor` 派发经 `std::thread` 启动，自带同步。

`CoroContext` 的共享状态（`_e`、`_reserved` 为 `shared_ptr`，`_stop`
内部亦为原子引用计数句柄）在拷贝时全部共享——**拷贝 CoroContext 与共享
Inner 在功能上等价**（执行器、IOContext、取消状态照常共享），
传 `CoroContext` 不损失任何能力，只是每条链拿到独立 Inner。

**API 规则**：

```cpp
MUST(asio_ctx(NewThreadExecutor{}), ctx);   // ctx: Rc<CoroContext>
task.detach(*ctx);   // ✓ 传 CoroContext：拷贝 → 新 Inner，遵守不变量
task.detach(ctx);    // ✗ 传 Rc：共享 Inner，两条链并发 ++/-- → 数据竞争
```

- `asio_ctx()` 返回 `Rc<CoroContext>` 是让 main 线程持有 ctx 以
  `run()`/`cancel()`；注入任务链时**必须解引用**（或 `std::move` 一个独占的 Rc）。
- **落实方式：契约 + debug 断言，不做编译期约束**。`detach()`/`by()` 在
  契约注释中要求传入独占所有的 ctx；debug 构建下 `Detach::operator()` 与
  `NextBase::by()` 断言 `Rc::use_count() == 1`（`Rc::unique()`，检查点在
  dispatch 之前，读取计数本身无竞争）。
- 违反后果：两条并发执行的链对同一 Inner 做非原子 ++/-- → 丢失更新
  （泄漏）或提前归零（UAF）。
- 曾违反该契约的调用点（传 Rc 本体）已全部修正为 `detach(*ctx)`/
  `by(*ctx)`：5 个 examples、`test/benches/http/server_xsl.cpp`、
  `test/integration/http_compare/compare.cpp`、`test/integration/asio/bind.cpp`、
  `test/integration/asio/connect.cpp`（`.by(*this->ctx)`）。

与迁移文档的关系：本不变量确立后，迁移文档 #6（Rc 原子化）**转为不实施**
（保留非原子换热路径零开销）；#2 中"对同一上下文的 Rc 计数无序 --"仅在
违反本不变量时发生，修正上述调用点后该 Rc 风险消除（#2 的剩余部分是
执行器生命周期本身）。

## 4. I/O 层（xsl_sys + xsl_asio）

`IOContext`（每个对应一个 epoll fd）：

- `run()`：`epoll_pwait`（100ms 超时 + 屏蔽 SIGINT/TERM/QUIT），事件派发时
  **先 pin 住 handler 的 shared_memory 再释放锁**（handler 可能在派发期间
  被注销），DELETE hint → `remove(fd)`
- `add/remove`：epoll_ctl + `ShardRes` 分片锁保护的 handler map
- `shutdown()`：置 `stopped` 标志 → `shutdown_notify()` 对所有订阅信号执行
  **粘性 stop** → 挂起的 IO await 以 false 恢复，激活 recv/send 循环的
  `!co_await` 取消分支（不再依赖 fd 关闭时序）→ 清空 map → 关闭 epoll

读写路径（`asio/io.h`）：先试 syscall（`recv/send`），`EAGAIN` 时
`co_await read_signal()/write_signal()` 挂起，epoll 就绪后由信号恢复重试。
连接式设备的 `accept()` 循环内检查 `stopped()`，poller 关闭后不会永久挂起。

## 5. HTTP 服务器栈与每请求控制流

```
HttpServer::serve_connection        accept 循环（Task，detached）
  └─ co_yield per-connection        yield_value → detach 到子上下文
       └─ imm_serve_connection      keep-alive 循环：Request::read → HttpService → ResponseBuilder::sendto
            └─ HttpService          router 查路由 → handler（Task）→ HandleContext
```

一次请求（NoopExecutor 语义下全部发生在 poller 线程内联；NewThreadExecutor
下每次唤醒派发新线程）：

```
connect/read 就绪 → epoll → handler → publish(IN) → 信号 release
  → 续体 → dispatch → 协程恢复 → recv → 解析（Message::read → us_map）
  → HttpService::operator()（路由 → handler）→ easy_resp（string body + Content-Length）
  → checkout（补 Date）→ to_string → write 头 + write 体 → 循环等待下一请求
```

## 6. 已知设计权衡（基准实证，详见 benches/http.md）

- **每请求固定成本 ~50µs**：响应 head/body 两次 `write`、`to_string()` 的
  1024 字节拼接、`us_map` 构造、每请求 `std::format` 日期 —— dispatch 模型
  （Noop vs NewThread）实测不是瓶颈（19K vs 20K RPS 持平）
- **TCP_NODELAY 必须开启**：accepted socket 缺省 Nagle 时 keep-alive 响应被
  delayed ACK 卡住 ~40ms（已修复：accept 流程默认 `no_delay()`）
- **线程内联 vs 并行**：NoopExecutor 把整个服务器钉在一个核（4×4 反而
  -20%）；NewThreadExecutor 用线程创建换来并行。多核扩展需池化或多 poller
- **GCC 16 协程代码gen**：await 表达式临时量的生命周期标记有缺陷（协程帧
  清理时双重析构），`block()` 为此采用手动驱动 + 无 await 表达式的 shell
  协程；协程体内没有 `co_return`/`co_await` 时 GCC 不按协程编译（静默内联）

## 7. 测试与验证入口

| 内容 | 位置 |
|------|------|
| 两个 HTTP 实现的行为一致性 | `test/integration/http_compare/`（ctest: it_http_compare） |
| xsl vs asio 吞吐/延迟 | `test/benches/http/bench.sh`（数据: benches/http.md） |
| TCP accept/echo | `test/integration/asio/bind.cpp`（ctest: it_bind） |
| 协程原语单元测试 | `test/unit/coro/` |
