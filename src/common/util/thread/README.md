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
- 若新任务写入区域的任一区块键已在 `m_runningRegions` 中 → 冲突，任务放回队列，等待 `m_areaReleasedCondition`（带 1ms 超时防丢失通知）。
- 无冲突 → 标记区域所有区块键为正在执行 → 执行 → 完成后清除标记并 `notify_all`。
- 无坐标任务不进入 `m_runningRegions`，不受区域互斥约束，可与任何区域任务并行。

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
| `common/world/storage/StorageTaskManager` | 存储 IO 任务调度 |
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

### 5. 优先级数值越小越高

`TaskPriority::Critical = -3` 优先级最高，`TaskPriority::Background = 3` 优先级最低，不是数值越大越优先。

### 6. unique_ptr 不能用于优先队列

任务使用 `shared_ptr<ITask>` 而非 `unique_ptr`，因为 `std::priority_queue` 要求元素可复制。
