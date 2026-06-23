# 并发工具模块

提供区块生成调度所需的并发原语，对齐 Moonrise 的 `concurrentutil` 设计。

## 目录结构

```
concurrent/
├── ReentrantAreaLock.hpp   # 按区块坐标分区的可重入区域锁（头文件）
├── ReentrantAreaLock.cpp   # 区域锁实现
└── README.md               # 本文档
```

## 内部模块关系

```
ReentrantAreaLock
       │
       └── Node (RAII 锁句柄，析构自动释放)
```

## 上下游外部依赖关系

### 上游依赖

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型（`ChunkCoord`、`i32`、`u64`） |
| `common/util/assert/AssertAll.hpp` | 断言（`MC_ASSERT_RELEASE`） |
| `<mutex>` / `<condition_variable>` | 互斥与等待 |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `server/world/ChunkTaskScheduler` | 保护 `schedule`/`checkNeighbour`/`onChunkGenComplete` 的原子性，覆盖 `[x±accessRadius, z±accessRadius]` 区域 |
| `server/world/ServerChunkManager` | 通过 `ChunkTaskScheduler` 间接使用 |

## ReentrantAreaLock 设计要点

- **section 分区粒度**：区块坐标右移 `coordinateShift` 位得到 section 键，`coordinateShift = 0` 时一区块一锁条目（最细粒度）。
- **Node 所有权**：一次 `lock` 创建一个 `Node`，代表调用线程对区域内所有 section 的占有。同线程对已占有 section 的重入不阻塞。
- **不相交不变量**：不同线程不能同时持有相交区域；相交时后到者阻塞等待。
- **RAII**：`lock`/`tryLock` 返回 `std::unique_ptr<Node>`，析构自动 `unlock`。
- **坐标打包**：section 键 `(sectionX, sectionZ)` 打包为 `u64`（高 32 位 Z，低 32 位 X），对齐 Moonrise `IntPairUtil.key`。

## 容易踩的坑

1. **持锁等待避免 use-after-free**：`lock` 冲突等待时取 `shared_ptr<SectionWaiters>` 副本再 `cv.wait`，因为其他等待者被 `notify_all` 唤醒后可能先擦除 `m_waiters[key]`，直接持有引用会悬空。
2. **重入只覆盖同线程**：同线程对已占有 section 的重入不增加 `areaAffectedLen`，`unlock` 只释放本 Node 实际写入的 section。嵌套锁必须按"内层先释放"的顺序析构（RAII 作用域自然保证）。
3. **`unlock` 必须由对应 Node 析构触发**：`Node` 析构调用 `m_lock.unlock(*this)`，幂等（`areaAffectedLen == 0` 时直接返回）。纯重入 Node（未写入任何 section）析构无副作用。
4. **负坐标**：`sectionKey` 用算术右移（`i32 >> shift`），负坐标向负无穷取整，与 Java `>>` 一致，分区边界正确。
