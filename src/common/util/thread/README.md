# 线程工具模块

提供多线程任务调度和管理的基础设施。

## 目录结构

```
thread/
├── ITask.hpp              # 任务接口定义
├── ServerWorkerPool.hpp   # 服务端任务池头文件
├── ServerWorkerPool.cpp   # 服务端任务池实现
└── README.md              # 本文档
```

## 文件介绍

### ITask.hpp

定义任务相关的基础类型：

- **TaskPriority** - 任务优先级枚举
  - `Critical` (-3): 紧急任务（退出游戏、崩溃恢复）
  - `High` (-2): 高优先级（玩家附近区块、关键IO）
  - `Normal` (0): 普通任务
  - `Low` (2): 低优先级（后台任务）
  - `Background` (3): 最低优先级（导入、压缩）

- **TaskType** - 任务类型枚举
  - `ChunkGenerate`: 区块生成
  - `ChunkSave`: 区块保存
  - `ChunkLoad`: 区块加载
  - `WorldImport`: 世界导入
  - `SnapshotCreate`: 快照创建
  - `SnapshotRestore`: 快照恢复
  - `DBWrite`: 数据库写入
  - `DBRead`: 数据库读取
  - `Custom`: 自定义任务

- **ITask** - 任务抽象基类
  - `execute(cancelSignal)`: 执行任务，定期检查取消信号
  - `onCancel()`: 任务取消时的清理回调
  - `type()`: 返回任务类型
  - `description()`: 返回任务描述
  - `traceCategory()`: 返回 Perfetto 追踪类别

- **TaskCallback** - 任务完成回调函数类型

### ServerWorkerPool.hpp/cpp

服务端通用任务池实现：

**核心功能**：
- 优先级队列调度
- 协作取消机制
- Perfetto 追踪集成
- 线程命名

**主要方法**：
- `start()`: 启动工作线程
- `shutdown()`: 关闭工作线程
- `submit(task, callback, priority, cancelToken)`: 提交任务
- `cancel(taskId)`: 取消指定任务
- `pruneCancelledTasks()`: 清理已取消的排队任务
- `waitForCompletion()`: 等待所有任务完成

## 模块关系

```
ServerWorkerPool
       │
       ├── ITask (抽象接口)
       │        │
       │        └── ChunkGenerateTask (具体实现)
       │
       └── Perfetto 追踪系统
```

**被以下模块使用**：
- `src/server/world/ServerChunkManager.hpp` - 区块生成调度
- 存档系统（未来）- IO 任务调度

## 使用方法

### 自定义任务

```cpp
#include "common/util/thread/ITask.hpp"

class MyTask : public mc::util::ITask {
public:
    explicit MyTask(int data) : m_data(data) {}

    bool execute(const std::atomic<bool>& cancelSignal) override {
        if (cancelSignal.load(std::memory_order_acquire)) {
            return false;
        }
        // 执行任务...
        return true;
    }

    TaskType type() const override { return TaskType::Custom; }
    std::string description() const override {
        return "MyTask(" + std::to_string(m_data) + ")";
    }

private:
    int m_data;
};
```

### 使用任务池

```cpp
#include "common/util/thread/ServerWorkerPool.hpp"

// 创建任务池
mc::util::ServerWorkerPool pool(4, "Worker");
pool.start();

// 提交任务
auto task = std::make_unique<MyTask>(42);
pool.submit(std::move(task),
    [](bool success, mc::util::ITask* task) {
        if (success) {
            // 任务成功
        }
    },
    mc::util::TaskPriority::Normal);

// 等待完成
pool.waitForCompletion();

// 关闭
pool.shutdown();
```

### 取消任务

```cpp
auto cancelToken = std::make_shared<std::atomic<bool>>(false);

auto task = std::make_unique<MyTask>(42);
u64 taskId = pool.submit(std::move(task),
    [](bool success, mc::util::ITask*) {
        // success 为 false 表示被取消
    },
    mc::util::TaskPriority::Normal,
    cancelToken);

// 取消任务
cancelToken->store(true, std::memory_order_release);
```

## 设计决策

### 为什么使用 shared_ptr<ITask>

优先队列 `std::priority_queue` 要求元素可复制，而 `unique_ptr` 只能移动。使用 `shared_ptr` 解决此问题，同时保证任务生命周期安全。

### 优先级设计

优先级数值越小优先级越高，这是常见的实时系统设计模式：
- 负数优先级用于紧急/高优先级任务
- 零为普通优先级
- 正数为低优先级/后台任务

### 协作取消

使用 `atomic<bool>` 作为取消信号，任务执行器需要定期检查。这种设计：
- 不强制中断任务（避免资源泄漏）
- 支持优雅取消
- 线程安全

## 容易踩的坑

1. **忘记启动任务池**: 调用 `submit()` 前必须调用 `start()`，否则任务不会执行
2. **取消信号检查**: 任务执行器必须定期检查 `cancelSignal`，否则无法响应取消
3. **回调线程安全**: 回调在任意工作线程执行，必须保证线程安全
4. **任务生命周期**: 不要在回调中访问已销毁的对象

## 测试用例

测试文件位于 `tests/common/util/thread/ServerWorkerPoolTest.cpp`：

- 构造和生命周期测试
- 任务提交测试
- 优先级排序测试
- 取消机制测试
- 异常处理测试
- 线程安全测试
