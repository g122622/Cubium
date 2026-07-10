# 并发工具模块

提供区块生成调度所需的并发原语，对齐 Moonrise 的 `concurrentutil` 设计。

## 目录结构

```
concurrent/
├── ReentrantAreaLock.hpp                # 按区块坐标分区的可重入区域锁（头文件）
├── ReentrantAreaLock.cpp                # 区域锁实现
├── ConcurrentLong2ObjectHashTable.hpp   # 每桶 mutex 分段锁哈希表（u64 key → shared_ptr<Node>）
├── MultiThreadedQueue.hpp               # 无锁单链表 FIFO 队列（Node 的等待线程队列）
├── LockSupport.hpp                      # park/unpark 线程阻塞原语（对齐 Java LockSupport）
└── README.md                            # 本文档
```

## 内部模块关系

```
ReentrantAreaLock
       │
       ├── ReentrantAreaLockNode (RAII 锁句柄，继承 MultiThreadedQueue<ThreadHandle*>，自身即等待队列)
       │        │
       │        └── LockHandle (持有 shared_ptr<Node>，析构 unlock + 释放引用)
       │
       ├── ConcurrentLong2ObjectHashTable<shared_ptr<Node>>  (sectionKey → Node，每桶 mutex 分段锁)
       │
       ├── MultiThreadedQueue<LockSupport::ThreadHandle*>     (Node 的等待队列基类)
       │
       └── LockSupport (park/unpark，permit 语义)
```

## 上下游外部依赖关系

### 上游依赖

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型（`ChunkCoord`、`i32`、`u64`） |
| `common/util/assert/AssertAll.hpp` | 断言（`MC_ASSERT_RELEASE` / `MC_ASSERT_RELEASE_MSG`） |
| `common/perfetto/TraceEvents.hpp` | `MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ReentrantAreaLock::unlock")` |
| `<atomic>` / `<memory>` / `<thread>` / `<mutex>` / `<condition_variable>` | 原语与线程 |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `server/world/ChunkTaskScheduler` | 保护 `schedule`/`checkNeighbour`/`onChunkGenComplete` 的原子性，覆盖 `[x±accessRadius, z±accessRadius]` 区域 |
| `server/world/ServerChunkManager` | 通过 `ChunkTaskScheduler` 间接使用 |

## ReentrantAreaLock 设计要点

- **section 分区粒度**：区块坐标右移 `coordinateShift` 位得到 section 键。`coordinateShift = 0` 时一区块一锁条目（最细粒度）；`coordinateShift = N` 时每 `(1<<N)×(1<<N)` 区块共用一个 section 锁。由调用方按场景选择——`ChunkTaskScheduler` 用 6（对齐 Moonrise `getChunkSystemLockShift()`），使 `2*maxAccessRadius` 锁只触达 1~4 个 section。
- **Node 所有权（shared_ptr）**：一次 `lock` 创建一个 `Node`（`shared_ptr` 管理），代表调用线程对区域内所有 section 的占有。`m_nodes` 哈希表、持有线程的 `LockHandle`、阻塞线程的 `park` 引用共同保活 Node，对齐 Moonrise 的 GC 语义（C++ 无 GC，用 `shared_ptr` + `enable_shared_from_this` 等价实现）。
- **不相交不变量**：不同线程不能同时持有相交区域；相交时后到者阻塞等待。同线程重入只允许"完全被覆盖"的子区域（相交但不被覆盖触发断言）。
- **RAII**：`lock`/`tryLock` 返回 `LockHandle`（持有 `shared_ptr<Node>`），析构时 `unlock`（移除 section + 排空等待队列 + unpark）再释放 `shared_ptr`。
- **坐标打包**：section 键 `(sectionX, sectionZ)` 打包为 `u64`（高 32 位 Z，低 32 位 X），对齐 Moonrise `IntPairUtil.key`。
- **分段锁实现**：`m_nodes` 用 `ConcurrentLong2ObjectHashTable`（每桶 mutex 分段锁 putIfAbsent/remove/get，对齐 Moonrise `synchronized(node)`），等待队列用 per-Node 的 `MultiThreadedQueue` + `LockSupport` park/unpark，替代旧的 mutex+condition_variable，消除 `unlock:notify` 的 per-key notify_all 惊群。

## 无锁等待协议（park/unpark + MultiThreadedQueue）

冲突线程的阻塞与唤醒流程（对齐 Moonrise `ReentrantAreaLock.lock`）：

1. `lock` 遍历区域 section，`putIfAbsent` 占有；遇到他人占有的 section 得到 `park = prev`（`shared_ptr` 保活）。
2. 回滚已占有的 section（`remove(key, node)` 值校验），排空本 Node 等待队列（`pollOrBlockAdds` 循环 unpark）。
3. 退避后（`failures > 128`）`park->add(currThreadHandle)` 把自己入队到冲突 Node，`LockSupport::park()` 阻塞。
4. 持有线程 `unlock`：`remove` 所有 section，`pollOrBlockAdds` 循环取出等待者并 `unpark`。
5. `pollOrBlockAdds` 的"取队首 or 阻止入队"原子语义保证不丢唤醒：unlock 排空时若队列空则 preventAdds，lock 的 `add` 返回 false（被阻止）直接重试，不 park；若 lock 先 `add` 成功，unlock 的 `pollOrBlockAdds` 必取到该线程并 unpark。

## 容易踩的坑

1. **Node 生命周期（use-after-free）**：阻塞线程持有冲突 Node 的 `shared_ptr`（`park`）期间遍历其 `MultiThreadedQueue` 链表（`add` 的 CAS），持有线程可能并发 `unlock` + 释放 `LockHandle`。`shared_ptr` 保证 Node 在 `add` + `park` 期间存活，`~MultiThreadedQueue` 链表析构延迟到最后一个 `shared_ptr` 释放。**不要**把 Node 改回 `unique_ptr` 或裸指针——会 reintroduce use-after-free。
2. **`park` 必须是 `shared_ptr`**：`putIfAbsent` 返回 `shared_ptr`（拷贝保活），阻塞线程的 `park` 局部变量持有该拷贝。若用裸指针，持有线程释放 Node 后 `park->add` 遍历已释放链表。
3. **`unlock` 的值校验 remove**：`m_nodes.remove(key, self)` 用 `shared_from_this()` 做值校验，防误删（同 key 被其他线程的 Node 占有时不删）。`unlock` 必须用 `shared_from_this`，不能用裸 `this` 构造 `shared_ptr`（会创建新控制块）。
4. **重入只覆盖同线程**：同线程对已占有 section 的重入不增加 `areaAffectedLen`，`unlock` 只释放本 Node 实际写入的 section。嵌套锁必须按"内层先释放"的顺序析构（RAII 作用域自然保证）。
5. **`pollOrBlockAdds` 排空无条件执行**：lock 失败回滚路径和 unlock 路径都无条件循环 `pollOrBlockAdds` + `unpark`（即使本线程未 add 任何等待者）。被 `preventAdds` 阻止的 `add` 会返回 false，阻塞线程落入退避分支不 park，由 `allowAdds`（lock 重试前）恢复入队能力。
6. **`MultiThreadedQueue` 元素生命周期**：队列存 `ThreadHandle*` 裸指针。被 park 的线程在队列成员身份期间不会退出（park 阻塞），与 Java GC 保持 Thread 存活语义等价。`unpark` 在 `pollOrBlockAdds` 取出后立即调用，被唤醒线程在 unpark 完成前不会退出。
7. **负坐标**：`sectionKey` 用算术右移（`i32 >> shift`），负坐标向负无穷取整，与 Java `>>` 一致，分区边界正确。
8. **`ConcurrentLong2ObjectHashTable` 不 resize**：固定 4096 桶 + 链表。区块 section 数量有上界（活跃区块范围），无需 resize。每桶 `std::mutex` 分段锁，物理摘除在锁内完成（无逻辑删除节点堆积、无 ABA），对齐 Moonrise `synchronized(node)`。

## 性能要点（重构动机）

旧实现（mutex + condition_variable）的 `ReentrantAreaLock::unlock:notify` 是 Perfetto 追踪的最大自耗时热点（635ms / 332 次），根因：

- 全局 `std::mutex` 串行化所有 lock/tryLock/isHeldByCurrentThread/unlock。
- `unlock` 对每个 key 逐个 `notify_all`（半径 22 → 2025 key），被唤醒的线程立刻争抢同一把 mutex，惊群效应。

中间实现（std::atomic<shared_ptr> 无锁哈希表）解决了 notify 惊群，但引入 livelock：

- `std::atomic<shared_ptr<BucketNode>>` 在 MSVC 下每原子操作持对象内部 mutex，高争用下竞争该 mutex。
- `physicallyUnlink` 不重试 → 逻辑删除节点堆积 → 桶链变长 → get/putIfAbsent 遍历变慢（CPU 升高）。
- `onChunkGenComplete` 持 2*maxAccessRadius（2025 sections）巨型锁，多 worker 并发 EMPTY 生成争用同批 section，
  无锁方案的 CAS 重试 + 链遍历无退避导致单核 100% 忙等（livelock）。

当前实现（每桶 mutex 分段锁 + per-Node 等待队列 + park/unpark）：

- `m_nodes` 每桶独立 `std::mutex`（4096 桶），不同桶完全并行，同桶争用时阻塞睡眠（不占 CPU）。物理摘除在锁内完成，链表无堆积。
- `unlock` 是 O(areaAffected) 的 `remove`（每 section 一次桶锁）+ 一次 `pollOrBlockAdds` 排空逐个 `unpark`，无 per-key cv、唤醒精确到 Node 局部。
- 不相交区域的 lock/unlock 完全并行（不同桶、不同 Node），不被大区域 unlock 阻塞。
- 与 Moonrise `ConcurrentChainedLong2ReferenceHashTable` 的 `synchronized(node)` 分段锁语义一致。

**`coordinateShift` 是 `lock` 性能的关键调节项**：`lock(center, radius)` 的 section 操作数 = `((2*radius)>>shift + 1)²`。`ChunkTaskScheduler` 的 `2*maxAccessRadius=22` 锁在 shift=0 下触达 45²=2025 个 section（每次 `putIfAbsent` 取一次桶锁，冲突时回滚最多 2025 次 `remove` + 退避重试），是 `ReentrantAreaLock::lock` 成为 Perfetto #1 热点（10.35s/357 次）的根因。shift=6 后同一锁只触达 1~4 个 section，开销降低约 500 倍。Moonrise 用 `getChunkSystemLockShift()=6`（`SECTION_SHIFT=6`，64×64 区块/section），Cubium 对齐之。

## 测试

`tests/common/util/concurrent/` 下：

- `ReentrantAreaLockTest.cpp`：区域锁功能 + 并发回归（含大区域 unlock 不阻塞远处区域、N 线程严格互斥、park/unpark 不丢唤醒）。
- `LockSupportTest.cpp`：park/unpark permit 语义、unpark-before-park 不丢唤醒、跨线程唤醒、permit 不累加。
- `MultiThreadedQueueTest.cpp`：add/poll FIFO、preventAdds/allowAdds、pollOrBlockAdds 排空阻塞、forceAdd、并发不丢元素。
- `ConcurrentLong2ObjectHashTableTest.cpp`：putIfAbsent/remove(key,expected)/get、并发 putIfAbsent 单胜者、ABA 安全。
