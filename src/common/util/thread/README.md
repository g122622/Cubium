# 线程工具模块

提供多线程任务调度和管理的基础设施。

## 目录结构

```
thread/
├── ITask.hpp              # 任务接口定义（优先级、类型、执行接口）
├── UniversalWorkerPool.hpp   # 通用任务池头文件（服务端/客户端共用）
├── UniversalWorkerPool.cpp   # 通用任务池实现
└── README.md              # 本文档
```

## 内部模块关系

```
UniversalWorkerPool
       │
       └── ITask (抽象接口)
                │
                └── ChunkGenerateTask、StorageTask 等（具体实现）
```

## 区域互斥（对齐 Moonrise 区域锁执行器）

`UniversalWorkerPool` 提供两套 `submit` 重载：

- **无坐标 `submit`**：任务可完全并行，不参与区域互斥。用于 EMPTY~INITIALIZE_LIGHT 等 parallelCapable 状态。
- **带坐标 `submit(task, callback, centerX, centerZ, writeRadius, ...)`**：任务携带矩形写入区域
  `[centerX±writeRadius, centerZ±writeRadius]`，调度器保证同一时刻不存在两个写入区域**重叠**的区域互斥任务同时执行。
  用于 FEATURES/LIGHT/SPAWN/FULL 等会写方块的状态。`writeRadius` 来源为 `ChunkStep::blockStateWriteRadius()`（FEATURES=1，LIGHT=2，其他≤0）。

区域互斥机制：
- 任务开始执行前检查 `m_runningRegions`（正在执行的区域互斥任务占据的区块键集合）。
- 若新任务写入区域的任一区块键已在 `m_runningRegions` 中 → 冲突，任务放回队列（其他空闲 worker 也可取走），本 worker 在 `m_areaReleasedCondition` 上按谓词等待，直到自己的写入区域不再冲突、或 `m_stop` 置位（关闭）。
- 无冲突 → 标记区域所有区块键为正在执行 → 执行 → 完成后在 `m_runningRegionsMutex` 内清除标记并 `notify_all`。
- 无坐标任务不进入 `m_runningRegions`，不受区域互斥约束，可与任何区域任务并行。

等待无丢失唤醒：谓词求值（`hasAreaConflictLocked`）与区域标记清除（`unmarkAreaRunningLocked`）都在 `m_runningRegionsMutex` 下完成，且清除后持锁 `notify_all`；`shutdown` 同样持该锁通知。故不存在"通知落在无人阻塞的窗口被丢弃"的可能，等待路径无需超时兜底。

`canExecuteNow(centerX, centerZ, writeRadius)`：查询某写入区域是否可立即执行（无冲突），用于调度器在提交前预检查。

区块键打包：`packChunkKey(x, z)` = `(u32)x << 32 | (u32)z`，不依赖 chunk 模块，保持 `common/util` 层级独立。

## 上下游外部依赖关系

### 上游依赖

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型（i8, i32, u64 等） |
| `perfetto` | 性能追踪 |
| `<atomic>`, `<thread>`, `<queue>` | C++ 标准库 |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `server/world/ServerChunkManager` | 区块生成任务调度 |
| `server/world/storage/task/StorageTaskManager` | 存储 IO 任务调度 |
| `server/application/MinecraftServer` | 服务器持有 ServerCompute/ServerIO 池 |
| `client/application/ClientApplication` | 客户端持有 ClientCompute 池（chunkmesh/皮肤等） |

## 容易踩的坑

### 1. 忘记启动任务池

```cpp
// ❌ 错误：任务不会执行
UniversalWorkerPool pool(4, "MyWorker", 300);
pool.submit(task, callback);

// ✅ 正确：先启动再提交
UniversalWorkerPool pool(4, "MyWorker", 300);
pool.start();  // 必须！
pool.submit(task, callback);
```

提交到未启动的池不会抛错：任务被丢弃，池只打印一条 warn 并用 `callback(false, nullptr)` 通知。**不带 callback 的提交方（如存储任务）因此永远收不到任何结果**，表现为调用方无声地永久等待——排查此类"卡住"先确认池是否已 `start()`。

### 1b. 被取消/被丢弃的任务必须通过 `onCancel()` 完成收尾

出队时判定为已取消的任务**不会执行 `execute()`**，线程池只调用 `ITask::onCancel()`。如果任务的 `execute()` 除了干活之外还承担"通知提交方/结清计数"的职责（存储任务就是如此），那么 `onCancel()` 必须把这些收尾做完（`StorageTask::onCancel()` 会以"已取消"信号再执行一次 executor）。否则提交方永远等不到结果。同理，`shutdown()` 会在 join 之前先取出队列中不会再执行的任务并调用它们的 `onCancel()`——否则等待这些任务结果的其他线程不响应停止位，`join` 会永久挂起。

### 2. 取消信号检查缺失

任务执行器必须定期检查 `abortSignal`，否则无法响应取消：

```cpp
bool execute(const std::atomic<bool>& abortSignal) override {
    // ❌ 错误：长时间阻塞不检查取消
    doHeavyWork();

    // ✅ 正确：定期检查
    for (auto& item : items) {
        if (abortSignal.load(std::memory_order::acquire)) {
            return false;  // 被取消
        }
        process(item);
    }
    return true;
}
```

### 3. 回调线程安全

回调在**任意工作线程**执行，必须保证线程安全：

```cpp
// ❌ 错误：非线程安全
std::vector<Result> results;
pool.submit(task, [&](bool success, ITask* task) {
    results.push_back(result);  // 数据竞争！
});

// ✅ 正确：使用互斥锁或原子操作
std::mutex resultsMutex;
pool.submit(task, [&](bool success, ITask* task) {
    std::lock_guard lock(resultsMutex);
    results.push_back(result);
});
```

### 4. 任务生命周期

不要在回调中访问已销毁的对象：

```cpp
// ❌ 错误：this 可能已销毁
class Manager {
    void submitTask() {
        pool.submit(task, [this](bool success, ITask* task) {
            this->onComplete();  // this 可能悬垂！
        });
    }
};

// ✅ 正确：使用 shared_ptr 延长生命周期
auto self = shared_from_this();
pool.submit(task, [self](bool success, ITask* task) {
    self->onComplete();
});
```

### 4.1 提交后不得再访问任务对象

`submit` 接收 `unique_ptr<ITask>`，**所有权随之转移给池**：池在任务执行完毕（含完成回调）后立即销毁该对象。提交后通过裸指针访问它（尤其是把它当成 wait/notify 的同步句柄）属于 use-after-free：该块内存未被复用时残留值可能恰好"看起来正确"而侥幸通过，一旦被复用就会永久等待（表现为测试超时）。观察任务状态请在回调内进行，或另建由调用方自己持有的同步量（原子标志 / 条件变量）。

### 5. 优先级数值越小越高

`TaskPriority::Critical = -3` 优先级最高，`TaskPriority::Background = 3` 优先级最低，不是数值越大越优先。

### 6. unique_ptr 不能用于优先队列

任务使用 `shared_ptr<ITask>` 而非 `unique_ptr`，因为 `std::priority_queue` 要求元素可复制。

### 7. `waitForCompletion()` 的完成语义包含任务回调

`waitForCompletion()` 的完成谓词是 `m_outstandingTaskCount == 0`，该计数从 `submit()` 入队起算，到**任务回调也执行完毕**（或任务被取消/裁剪/关闭清空而丢弃）为止。因此它返回后：

- 队列已空、无任务在执行，且**所有任务的回调都已跑完**；
- 在回调中访问的对象可以安全析构（`ClientWorld::destroy()` 依赖这一点在析构前等待在途回调）。

不要改回 `pendingTaskCount() == 0 && runningTaskCount() == 0`：后者有两个提前返回窗口——回调在 `m_runningTaskCount` 递减之后才执行；worker "出队"到"标记运行"之间队列已空且运行数为 0。历史上这两者会让 `EXPECT_EQ(completedCount, numTasks)` 偶发少 1，也会让"等待回调结束后再析构"的守护失效。
