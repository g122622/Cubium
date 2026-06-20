# Brain 移动任务 (movement/)

Brain 系统的移动类任务，控制实体的导航、漫步、视线、追逐、逃跑等行为。

## 目录结构

```
movement/
├── MovementTasks.hpp    # 6个移动任务模板类
└── README.md            # 本文件
```

## 任务类概览

所有任务均为模板类 `Task<E>`，E 为实体类型（如 VillagerEntity），通过 Brain 的记忆模块通信。

| 任务类 | 对应 MC 行为 | 记忆依赖 | 功能说明 |
|--------|-------------|---------|---------|
| `MoveToTargetTask<E>` | MoveToTargetSink | WALK_TARGET(present), PATH(registered) | 读取 WALK_TARGET 记忆执行实际寻路和移动，所有行走移动的核心执行器 |
| `StrollTask<E>` | RandomStroll | WALK_TARGET(absent) | WALK_TARGET 缺失时随机选择目标位置，设置到记忆中由 MoveToTargetTask 执行 |
| `LookAtEntityTask<E>` | SetEntityLookTarget / LookAtTargetSink | LOOK_TARGET(absent) | 概率触发，搜索附近实体并设置 LOOK_TARGET 记忆，持续更新视线方向 |
| `FindHiddenBlockTask<E>` | LocateHidingPlace | HIDING_PLACE(absent), WALK_TARGET(absent) | 被伤害或听到铃声时触发，从 HOME/NEAREST_BED 记忆寻找隐蔽点 |
| `ChaseTask<E>` | SetWalkTargetFromAttackTargetIfTargetOutOfRange | ATTACK_TARGET(present), WALK_TARGET(registered), LOOK_TARGET(registered) | 追踪攻击目标，动态更新 WALK_TARGET 和视线方向 |
| `FleeTask<E>` | SetWalkTargetAwayFrom | AVOID_TARGET(present), WALK_TARGET(absent) | 逃离威胁，使用 RandomPositionGenerator 生成远离方向的位置 |

## 数据流

```
传感器(Sensor) ──写入──→ Brain记忆(Memory) ──读取──→ 任务(Task) ──写入──→ Brain记忆
                                                    │
                                                    └──→ 控制器(Controller)
                                                         ├── PathNavigator::moveTo()
                                                         ├── LookController::setLookPosition()
                                                         └── MovementController::setMoveTo()
```

关键设计模式：**任务之间通过记忆模块解耦**。例如：
- `StrollTask` 设置 `WALK_TARGET` → `MoveToTargetTask` 读取并执行导航
- `ChaseTask` 设置 `WALK_TARGET` 和 `LOOK_TARGET` → `MoveToTargetTask` 执行移动
- `FleeTask` 设置 `WALK_TARGET` → `MoveToTargetTask` 执行逃跑

## 上下游依赖

### 上游依赖
- `Brain<E>` — 任务调度和记忆管理
- `Task<E>` — 任务基类，提供生命周期管理
- `MemoryModuleTypes` — 记忆类型定义
- `WalkTarget` / `IPositionTarget` / `BlockPosTarget` — 位置目标类型

### 下游依赖
- `PathNavigator` — 实际寻路执行
- `LookController` — 视线控制
- `CreatureEntity` — 陆地生物基类（`tryMoveTo`、`getPathWeight`）
- `RandomPositionGenerator` — 随机位置生成
- `GoalConstants` — 共享常量（距离、概率、速度等）

## 容易踩的坑

1. **WALK_TARGET 记忆冲突**：同一 Activity 中不能同时有多个任务写入 WALK_TARGET，优先级控制哪个任务先执行。
2. **PATH 记忆同步**：MoveToTargetTask 在 updateTask 中同步 PATH 记忆，其他任务不应直接操作 PATH。
3. **卡住检测**：MoveToTargetTask 内置卡住检测（100 tick 内移动距离 < 2.25 格），卡住时清除 WALK_TARGET。
4. **随机数生成**：当前使用 `math::Random(id ^ currentTick)` 创建临时随机对象，后续应改用实体自带的随机数生成器。
5. **IWorld* 而非 ServerWorld***：任务方法签名使用 `IWorld*`，与 Task 基类一致，不能使用 ServerWorld 独有的 API。
