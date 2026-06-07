# 线程工具模块

提供多线程任务调度和管理的基础设施。

## 目录结构

```
thread/
├── ITask.hpp              # 任务接口定义（优先级、类型、执行接口）
├── ServerWorkerPool.hpp   # 服务端任务池头文件
├── ServerWorkerPool.cpp   # 服务端任务池实现
└── README.md              # 本文档
```

## 内部模块关系

```
ServerWorkerPool
       │
       └── ITask (抽象接口)
                │
                └── ChunkGenerateTask、StorageTask 等（具体实现）
```

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
| `server/application/MinecraftServer` | 服务器启动/关闭管理 |

## 容易踩的坑

### 1. 忘记启动任务池

```cpp
// ❌ 错误：任务不会执行
ServerWorkerPool pool(4);
pool.submit(task, callback);

// ✅ 正确：先启动再提交
ServerWorkerPool pool(4);
pool.start();  // 必须！
pool.submit(task, callback);
```

### 2. 取消信号检查缺失

任务执行器必须定期检查 `cancelSignal`，否则无法响应取消：

```cpp
bool execute(const std::atomic<bool>& cancelSignal) override {
    // ❌ 错误：长时间阻塞不检查取消
    doHeavyWork();

    // ✅ 正确：定期检查
    for (auto& item : items) {
        if (cancelSignal.load(std::memory_order_acquire)) {
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
