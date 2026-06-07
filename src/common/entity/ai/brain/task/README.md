# Brain Task System

## 目录结构

```
task/
├── Task.hpp                    # 任务基类，定义核心接口
├── README.md
└── tasks/                      # 具体任务实现
    ├── movement/
    │   └── MovementTasks.hpp   # 移动、追逐、避险任务
    ├── action/
    │   └── ActionTasks.hpp     # 攻击、繁殖、进食任务
    └── interact/
        └── InteractTasks.hpp   # 门、玩家、物品互动任务
```

## 内部模块关系

```
Task<E> (基类)
    │
    ├── 依赖 memory::MemoryModuleType (读取/写入记忆)
    │
    ├── E (实体模板参数)
    │     ├── MobEntity
    │     ├── CreatureEntity
    │     └── AgeableEntity
    │
    └── ServerWorld (世界访问)

数据流：Sensor 更新 Memory → Schedule 选择 Activity → Task 根据 Memory 执行行为
```

## 上下游外部依赖关系

**被依赖（下游）**：
- 暂无直接使用者（Task 系统框架已就绪，待具体实体集成）

**依赖（上游）**：
- `brain/Brain.hpp` - Brain 主控制器
- `brain/memory/` - 记忆模块系统
- `entity/core/MobEntity.hpp` - 生物实体基类
- `server/world/ServerWorld.hpp` - 服务端世界

## 容易踩的坑

### 1. 记忆模块依赖

任务启动前必须确保所需记忆模块已设置。Task 基类通过 `m_requiredMemoryState` 检查记忆状态，若记忆不存在或状态不匹配，任务不会启动。

### 2. 生命周期管理

任务可能被中断，必须在 `resetTask()` 中清理状态。典型问题：导航路径未清理、攻击目标未清除等。

### 3. 模板实例化

每个实体类型需要单独实例化任务模板：`ChaseTask<MobEntity>`、`ChaseTask<VillagerEntity>` 等。

### 4. 与 Goal 系统的区别

| 特性 | Goal 系统 | Brain Task 系统 |
|------|-----------|-----------------|
| 状态管理 | 手动管理 | 记忆模块驱动 |
| 优先级 | mutex 标志 | 记忆状态要求 |
| 复杂度 | 简单行为 | 复杂行为链 |
| 适用场景 | 基础生物 | 村民、铁傀儡等 |

### 5. 任务超时机制

Task 基类内置持续时间机制 (`m_durationMin`, `m_durationMax`)，任务会在超时后自动停止。不要在 `shouldContinueExecuting()` 中忽略超时逻辑。
