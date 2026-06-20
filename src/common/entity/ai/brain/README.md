# Brain AI 系统

Brain系统是Minecraft 1.16.5引入的高级AI控制框架，用于更复杂的行为管理（如村民、猪灵等）。

## 目录结构

```
brain/
├── Brain.hpp                    # Brain主类 - 高级AI控制器（模板类）
├── memory/                      # 记忆模块
│   ├── Memory.hpp               # 内存存储容器(带TTL)
│   ├── MemoryModuleStatus.hpp   # 内存状态枚举
│   ├── MemoryModuleType.hpp     # 记忆类型定义模板
│   ├── MemoryModuleType.cpp     # 记忆类型注册（85+种）
│   ├── MemoryModules.hpp        # 村民记忆模块便捷别名
│   ├── IPositionTarget.hpp      # 位置目标抽象接口
│   ├── BlockPosTarget.hpp       # 方块位置目标实现
│   ├── WalkTarget.hpp           # 行走目标封装
│   └── README.md                # Memory模块说明
├── schedule/                    # 日程系统
│   ├── Activity.hpp             # 活动类型定义（15种）
│   ├── Activity.cpp             # 活动类型实现
│   ├── Schedule.hpp             # 日程安排
│   ├── Schedule.cpp             # 日程实现（4种预定义日程）
│   └── DutyTime.hpp             # 值班时间定义（离散duty时间片）
├── sensor/                      # 传感器系统
│   ├── Sensor.hpp               # 传感器基类
│   ├── SensorType.hpp           # 传感器类型工厂
│   ├── Sensors.hpp              # 传感器声明（9种）
│   └── Sensors.cpp              # 传感器实现
├── task/                        # 任务系统
│   ├── Task.hpp                 # 任务基类
│   ├── README.md                # Task模块说明
│   └── tasks/                   # 具体任务实现
│       ├── movement/            # 移动相关任务（6个已实现）
│       │   ├── MovementTasks.hpp
│       │   └── README.md
│       ├── action/              # 行动相关任务
│       │   └── ActionTasks.hpp
│       └── interact/            # 互动相关任务
│           └── InteractTasks.hpp
└── README.md
```

## 内部模块关系

```
Brain.hpp (主控制器)
    ├── memory/ (记忆存储)
    │   ├── Memory.hpp (基础值包装)
    │   ├── MemoryModuleType.hpp (类型标识)
    │   ├── IPositionTarget.hpp → BlockPosTarget.hpp → WalkTarget.hpp (位置目标链)
    │   └── MemoryModules.hpp (便捷别名)
    │
    ├── schedule/ (日程调度)
    │   ├── Activity.hpp (活动枚举)
    │   ├── DutyTime.hpp (时间片)
    │   └── Schedule.hpp (日程表)
    │
    ├── sensor/ (环境感知)
    │   ├── Sensor.hpp (基类)
    │   └── Sensors.hpp (具体传感器)
    │
    └── task/ (行为执行)
        ├── Task.hpp (基类)
        └── tasks/ (具体任务)
            ├── movement/ (6个移动任务已实现：MoveToTargetTask, StrollTask, LookAtEntityTask, FindHiddenBlockTask, ChaseTask, FleeTask)
            ├── action/ (行动任务)
            └── interact/ (互动任务)
```

数据流：`Sensor` 更新 `Memory` → `Schedule` 选择 `Activity` → `Task` 根据 `Memory` 执行行为

## 上下游外部依赖关系

**被依赖（下游）**：
- `entity/entities/villager/VillagerEntity.hpp` - 村民使用 Brain 系统，已集成全部 6 个移动任务
- `entity/entities/monster/nether/PiglinEntity.hpp` - 猪灵使用 Brain 系统（框架就绪，待集成）

**依赖（上游）**：
- `entity/core/MobEntity.hpp` - 生物实体基类
- `entity/core/LivingEntity.hpp` - 活体实体基类
- `world/IWorld.hpp` / `server/world/ServerWorld.hpp` - 世界访问
- `world/GlobalPos.hpp` - 全局位置（记忆存储）
- `world/village/poi/` - 兴趣点系统（工作站点、床等）

## 任务系统

移动类任务（6个）已实现并集成到 VillagerEntity：

| 任务类 | 记忆依赖 | 功能 |
|--------|---------|------|
| `MoveToTargetTask` | WALK_TARGET(present) | 执行实际寻路和移动 |
| `StrollTask` | WALK_TARGET(absent) | 随机漫步 |
| `LookAtEntityTask` | LOOK_TARGET(absent) | 看向附近实体 |
| `FindHiddenBlockTask` | HIDING_PLACE(absent), WALK_TARGET(absent) | 寻找隐蔽点 |
| `ChaseTask` | ATTACK_TARGET(present) | 追逐攻击目标 |
| `FleeTask` | AVOID_TARGET(present), WALK_TARGET(absent) | 逃离威胁 |

任务之间通过记忆模块解耦：StrollTask/ChaseTask/FleeTask 写入 WALK_TARGET → MoveToTargetTask 读取并执行导航。

## 与 Goal 系统的区别

| 特性 | Goal系统 | Brain系统 |
|------|----------|-----------|
| 复杂度 | 简单 | 复杂 |
| 记忆 | 无 | 有(TTL支持) |
| 传感器 | 无 | 有(自动感知) |
| 日程 | 无 | 有(时间活动) |
| 适用实体 | 大多数生物 | 村民、猪灵等 |

## 容易踩的坑

### 1. 记忆类型必须先初始化

使用任何记忆类型前必须调用 `MemoryModuleTypes::initialize()`，否则所有指针都是 nullptr。这是全局单例注册模式。

### 2. 记忆类型不匹配

获取记忆时类型必须与注册时完全匹配。`HOME` 是 `GlobalPos` 类型，不能用 `BlockPos` 获取。

### 3. 记忆类型使用指针作为键

必须使用全局注册的类型指针（如 `MemoryModuleTypes::HOME`），不能创建局部实例。

### 4. hasRequiredMemories 返回 false 的情况

当活动没有配置记忆要求时，`hasRequiredMemories()` 返回 `false`（与 MC 1.16.5 一致）。所有需要条件启动的活动都必须显式配置记忆要求。

### 5. OPENED_DOORS 类型

类型是 `std::unordered_set<GlobalPos>`，不是 `std::vector<GlobalPos>`。

### 6. WalkTarget 构造断言

WalkTarget 构造时会断言 target 非空，确保传入有效的 PositionTargetPtr。

### 7. Schedule 使用离散时间片

`Schedule` / `ScheduleDuties` 使用离散 duty 时间片语义，不是连续插值。参考 MC 1.16.5 的 `Schedule` 类。

### 8. 传感器更新频率

传感器的构造参数是更新间隔（tick），不要设置过小。例如 `NearestPlayersSensor` 默认 20tick 更新一次。

### 9. 任务生命周期

任务可能被中断，必须在 `resetTask()` 中清理状态。任务状态只有 STOPPED 和 RUNNING 两种。

### 10. Brain 模板实例化

每个实体类型需要单独实例化 Brain 模板：`Brain<VillagerEntity>`、`Brain<PiglinEntity>` 等。
