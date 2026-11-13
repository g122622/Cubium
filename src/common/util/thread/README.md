# 线程工具模块

提供多线程任务调度和管理的基础设施。

## 目录结构

```
thread/
├── ITask.hpp              # 任务接口定义（优先级、类型、执行接口）
├── UniversalWorkerPool.hpp   # 通用任务池头文件（服务端/客户端共用）
├── UniversalWorkerPool.cpp   # 通用任务池实现
├── ParallelFor.hpp        # 临时线程扇出的并行遍历（一次性大块并行，不经过任务池）
├── ParallelFor.cpp        # 动态取号的并行遍历实现
└── README.md              # 本文档
```

## 内部模块关系

```
UniversalWorkerPool            ParallelFor
       │                            │
       └── ITask (抽象接口)          └── std::thread（临时线程，调用线程参与）
                │
                └── ChunkGenerateTask、StorageTask 等（具体实现）
```

## 上下游外部依赖关系

### 上游依赖

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型（i8, i32, u64 等） |
| `common/util/assert/AssertAll.hpp` | `parallelFor` 的参数校验断言 |
| `perfetto` | 性能追踪 |
| `<atomic>`, `<thread>`, `<queue>` | C++ 标准库 |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `server/world/chunk/ServerChunkManager` | 通过区块生成调度器提交任务 |
| `server/world/storage/task/StorageTaskManager` | 存储 IO 任务调度 |
| `server/application/MinecraftServer` | 服务器持有 ServerCompute/ServerIO 池 |
| `client/application/ClientApplication` | 客户端持有 ClientCompute 池（chunkmesh/皮肤等） |
| `server/world/gen/structure/StructurePlacementState` | `parallelFor` 并行算 128 个要塞的群系搜索 |

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

### 6. 职责边界

通用池只处理普通任务的执行、优先级与取消。服务端世界通过 `AreaTaskScheduler` 管理区块区域依赖，只有就绪任务才提交计算池。IO 和客户端 mesh 池不维护区块区域状态。

### 7. `waitForCompletion()` 的完成语义包含任务回调

`waitForCompletion()` 的完成谓词是 `m_outstandingTaskCount == 0`，该计数从 `submit()` 入队起算，到**任务回调也执行完毕**（或任务被取消/裁剪/关闭清空而丢弃）为止。因此它返回后：

- 队列已空、无任务在执行，且**所有任务的回调都已跑完**；
- 在回调中访问的对象可以安全析构（`ClientWorld::destroy()` 依赖这一点在析构前等待在途回调）。

不要改回 `pendingTaskCount() == 0 && runningTaskCount() == 0`：后者有两个提前返回窗口——回调在 `m_runningTaskCount` 递减之后才执行；worker "出队"到"标记运行"之间队列已空且运行数为 0。历史上这两者会让 `EXPECT_EQ(completedCount, numTasks)` 偶发少 1，也会让"等待回调结束后再析构"的守护失效。

### 8. 池内任务要并行子任务时用 `parallelFor`，不要提交回同一个池

池的工作线程里若"提交子任务 → 阻塞等结果"，会自锁：池的全部工作线程都可能正等着自己提交的子任务，而子任务还排在队列里没人执行。也不要在等待时"顺手执行池里的其它任务"——`ChunkGenerationScheduler` 依赖线程局部的调度上下文（`thread_local SyncSchedulingContext`），让工作线程跨任务执行会破坏该前提。

`util::parallelFor` 用独立线程扇出（调用线程自己参与），只跑本次传入的 body，不触碰池内状态，因此在池内调用是安全的。代价是每次调用都有线程创建开销（毫秒级），只适合一次性、粒度较大的并行，不要在区块级热路径上反复调用。
