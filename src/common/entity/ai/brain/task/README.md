# Brain Task System

## 目录结构

```
task/
├── Task.hpp                    # 任务基类
├── tasks/                      # 具体任务实现
│   ├── movement/               # 移动相关任务
│   │   └── MovementTasks.hpp   # 移动、追逐、避险任务
│   ├── action/                 # 行动相关任务
│   │   └── ActionTasks.hpp     # 攻击、繁殖、进食任务
│   └── interact/               # 互动相关任务
│       └── InteractTasks.hpp   # 门、玩家、物品互动任务
└── README.md                   # 本文档
```

## 文件介绍

### Task.hpp

任务基类，定义了 Brain 系统中任务的核心接口：

- **TaskStatus**: 任务状态枚举 (STOPPED, RUNNING)
- **Task<E>**: 模板基类，E 为实体类型
  - `shouldExecute()`: 判断是否应该执行
  - `shouldContinueExecuting()`: 判断是否继续执行
  - `startExecuting()`: 开始执行回调
  - `updateTask()`: 每帧更新
  - `resetTask()`: 重置任务

### MovementTasks.hpp

移动相关任务：

| 任务 | 说明 | 参考 |
|------|------|------|
| `MoveToTargetTask` | 移动到目标位置 | MC MoveToTargetTask |
| `StrollTask` | 随机游走 | MC StrollTask |
| `LookAtEntityTask` | 看向实体 | MC LookAtEntityTask |
| `FindHiddenBlockTask` | 寻找隐蔽点 | MC FindHiddenBlockTask |
| `ChaseTask` | 追逐目标 | MC ChaseTask |
| `FleeTask` | 避险逃离 | MC FleeTask |

### ActionTasks.hpp

行动相关任务：

| 任务 | 说明 | 参考 |
|------|------|------|
| `AttackTask` | 攻击目标 | MC AttackTask |
| `BreedTask` | 繁殖行为 | MC BreedTask |
| `EatTask` | 进食行为 | MC EatTask |
| `PlayDeadTask` | 装死行为 | MC PlayDeadTask |
| `JumpTask` | 跳跃行为 | MC JumpTask |
| `KickTask` | 踢攻击 | MC KickTask |

### InteractTasks.hpp

互动相关任务：

| 任务 | 说明 | 参考 |
|------|------|------|
| `VillagerInteractTask` | 村民互动 | MC VillagerInteractTask |
| `InteractWithDoorTask` | 门互动 | MC InteractWithDoorTask |
| `FollowOwnerTask` | 跟随主人 | MC FollowOwnerTask |
| `ProtectOwnerTask` | 保护主人 | MC ProtectOwnerTask |
| `PickupItemTask` | 拾取物品 | MC PickupItemTask |
| `FollowParentTask` | 跟随父母 | MC FollowParentTask |
| `TemptTask` | 诱惑行为 | MC TemptTask |

## 模块关系

```
Task (基类)
    │
    ├── memory::MemoryModuleType (读取/写入记忆)
    │
    ├── E (实体模板参数)
    │     ├── MobEntity
    │     ├── CreatureEntity
    │     └── AgeableEntity
    │
    └── ServerWorld (世界访问)
```

## 使用方法

```cpp
// 创建任务
auto chaseTask = std::make_unique<ChaseTask<MobEntity>>(1.5f, 2.0f);

// 任务由 Brain 系统自动调度
// 当记忆模块满足条件时自动启动
```

## 任务状态转换

```
STOPPED ──start()──> RUNNING
    ↑                    │
    └─────stop()─────────┘
```

## 与 Goal 系统的区别

| 特性 | Goal 系统 | Brain Task 系统 |
|------|-----------|-----------------|
| 状态管理 | 手动管理 | 记忆模块驱动 |
| 优先级 | mutex 标志 | 记忆状态要求 |
| 复杂度 | 简单行为 | 复杂行为链 |
| 适用场景 | 基础生物 | 村民、铁傀儡等 |

## 容易踩的坑

1. **记忆模块依赖**: 任务启动前必须确保所需记忆模块已设置
2. **生命周期管理**: 任务可能被中断，需要在 `resetTask()` 中清理状态
3. **模板实例化**: 每个实体类型需要单独实例化任务模板

## 实现状态

| 组件 | 状态 |
|------|------|
| Task 基类 | ✅ 完成 |
| MovementTasks | ⚠️ 框架完成，TODO 需填充 |
| ActionTasks | ⚠️ 框架完成，TODO 需填充 |
| InteractTasks | ⚠️ 框架完成，TODO 需填充 |
