# AI 模块 (entity/ai)

本目录包含 Cubium 的实体人工智能系统，实现了类似 Minecraft Java Edition 1.16.5 的 AI 架构。

## 目录结构

```
ai/
├── EntitySenses.hpp/cpp          # 实体感知系统（检测附近实体）
├── brain/                        # 大脑系统（高级AI，预留框架）
│   ├── Brain.hpp                 # 大脑主类模板
│   ├── memory/                   # 记忆模块
│   │   ├── Memory.hpp            # 记忆容器（支持TTL）
│   │   ├── MemoryModuleType.hpp/cpp # 记忆模块类型（46+种）
│   │   ├── MemoryModuleStatus.hpp # 记忆状态枚举
│   │   ├── BlockPosTarget.hpp    # 方块位置目标
│   │   ├── IPositionTarget.hpp   # 位置目标接口
│   │   ├── WalkTarget.hpp        # 行走目标
│   │   └── README.md
│   ├── schedule/                 # 日程系统
│   │   ├── Activity.hpp/cpp      # 活动类型（15种）
│   │   ├── DutyTime.hpp          # 时间段定义
│   │   └── Schedule.hpp/cpp      # 日程管理
│   ├── sensor/                   # 传感器系统
│   │   ├── Sensor.hpp            # 传感器基类
│   │   ├── SensorType.hpp        # 传感器类型工厂
│   │   └── Sensors.hpp/cpp       # 具体传感器实现
│   └── task/                     # 任务系统
│       ├── Task.hpp              # 任务基类
│       └── tasks/                # 具体任务实现
│           ├── action/           # 行动任务
│           ├── interact/         # 互动任务
│           └── movement/         # 移动任务
├── controller/                   # 控制器（底层行为控制）
│   ├── JumpController.hpp/cpp    # 跳跃控制器
│   ├── LookController.hpp/cpp    # 视线控制器
│   ├── MovementController.hpp/cpp # 移动控制器
│   ├── GhastMovementController.hpp/cpp # 恶魂飞行控制器
│   └── VexMovementController.hpp/cpp   # 恼鬼飞行控制器
├── goal/                         # 目标系统（行为决策）
│   ├── Goal.hpp                  # 目标基类
│   ├── GoalConstants.hpp         # 目标常量（距离、时间、概率等）
│   ├── GoalFlag.hpp              # 目标互斥标志（Move/Look/Jump/Target）
│   ├── GoalSelector.hpp          # 目标选择器
│   ├── PrioritizedGoal.hpp       # 带优先级的目标包装器
│   └── goals/                    # 具体目标实现
│       ├── AvoidEntityGoal.hpp/cpp    # 避开实体
│       ├── BreedGoal.hpp/cpp          # 繁殖
│       ├── EatGrassGoal.hpp/cpp       # 吃草
│       ├── FindWaterGoal.hpp/cpp      # 寻找水源
│       ├── FishSwimGoal.hpp/cpp       # 鱼类游泳
│       ├── FollowParentGoal.hpp/cpp   # 跟随父母
│       ├── LookAtGoal.hpp/cpp         # 看向目标
│       ├── MeleeAttackGoal.hpp/cpp    # 近战攻击
│       ├── PanicGoal.hpp/cpp          # 恐慌逃跑
│       ├── RandomSwimmingGoal.hpp/cpp # 随机游泳
│       ├── RandomWalkingGoal.hpp/cpp  # 随机漫步
│       ├── SwimGoal.hpp/cpp           # 游泳（防溺水）
│       ├── SwimUpGoal.hpp/cpp         # 向上游动
│       ├── TemptGoal.hpp/cpp          # 食物诱惑
│       ├── AdditionalGoals.hpp/cpp    # 通用目标集合
│       ├── attack/                    # 攻击类目标
│       │   └── RangedAttackGoals.hpp/cpp # 远程攻击目标
│       ├── interact/                 # 交互类目标
│       │   ├── LandOnOwnersShoulderGoal.hpp/cpp # 落在主人肩上
│       │   └── TameableGoals.hpp/cpp  # 驯服动物目标
│       ├── movement/                 # 移动类目标
│       │   ├── FollowMobGoal.hpp/cpp  # 跟随生物
│       │   ├── FollowSchoolLeaderGoal.hpp/cpp # 跟随鱼群首领
│       │   ├── MovementGoals.hpp/cpp  # 通用移动目标
│       │   └── WaterAvoidingRandomFlyingGoal.hpp/cpp # 避水飞行
│       ├── special/                  # 特殊实体目标（按实体类型）
│       │   ├── AxolotlGoals.hpp/cpp   # 美西螈
│       │   ├── BatGoals.hpp/cpp       # 蝙蝠
│       │   ├── BeeGoals.hpp/cpp       # 蜜蜂
│       │   ├── BlazeFireballAttackGoal.hpp/cpp # 烈焰人火球
│       │   ├── DolphinGoals.hpp/cpp   # 海豚
│       │   ├── EndermanGoals.hpp/cpp  # 末影人
│       │   ├── EvokerGoals.hpp/cpp    # 唤魔者
│       │   ├── FoxGoals.hpp/cpp       # 狐狸
│       │   ├── GhastGoals.hpp/cpp     # 恶魂
│       │   ├── GuardianAttackGoal.hpp/cpp # 守卫者攻击
│       │   ├── IllusionerGoals.hpp/cpp # 幻术师
│       │   ├── IronGolemGoals.hpp/cpp # 铁傀儡
│       │   ├── MoveToLavaGoal.hpp/cpp # 移动到岩浆
│       │   ├── PandaGoals.hpp/cpp     # 熊猫
│       │   ├── PatrolGoals.hpp/cpp    # 巡逻
│       │   ├── PhantomGoals.hpp/cpp   # 幻翼
│       │   ├── RavagerGoals.hpp/cpp   # 劫掠兽
│       │   ├── ShulkerGoals.hpp/cpp   # 潜影贝
│       │   ├── SilverfishGoals.hpp/cpp # 蠹虫
│       │   ├── SlimeGoals.hpp/cpp     # 史莱姆
│       │   ├── SpecialGoals.hpp/cpp   # 通用特殊目标
│       │   ├── SquidGoals.hpp/cpp     # 鱿鱼
│       │   ├── TurtleGoals.hpp/cpp    # 海龟
│       │   ├── VexGoals.hpp/cpp       # 恼鬼
│       │   └── WanderingTraderGoals.hpp/cpp # 流浪商人
│       ├── target/                   # 目标选择类
│       │   └── TargetGoals.hpp/cpp    # 攻击目标选择
│       └── villager/                 # 村民目标
│           └── VillagerGoals.hpp/cpp  # 村民行为目标
├── pathfinding/                  # 寻路系统（A* 算法）
│   ├── FlaggedPathPoint.hpp      # 带标志的路径点
│   ├── NodeProcessor.hpp         # 节点处理器基类
│   ├── Path.hpp/cpp              # 路径对象
│   ├── PathFinder.hpp/cpp        # A* 寻路器
│   ├── PathHeap.hpp              # 路径点最小堆
│   ├── PathNavigator.hpp/cpp     # 路径导航器
│   ├── PathNodeType.hpp          # 路径节点类型（Walkable/Water/Lava等）
│   ├── PathPoint.hpp/cpp         # 路径点（代价、父节点等）
│   ├── RavagerNodeProcessor.hpp/cpp # 劫掠兽节点处理器
│   ├── Region.hpp/cpp            # 世界区域访问接口
│   └── WalkNodeProcessor.hpp/cpp # 行走节点处理器
└── util/                         # AI工具类
    ├── RandomPositionGenerator.hpp/cpp # 随机位置生成器
    └── README.md
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        MobEntity / CreatureEntity               │
│                     （持有 GoalSelector 和 PathNavigator）       │
└───────────────────────────┬─────────────────────────────────────┘
                            │ tick()
            ┌───────────────┼───────────────┐
            ▼               ▼               ▼
    ┌───────────────┐ ┌───────────────┐ ┌───────────────┐
    │ GoalSelector  │ │   Controller  │ │ PathNavigator │
    │ (选择目标)    │ │ (执行动作)    │ │ (沿路径移动)  │
    └───────┬───────┘ └───────────────┘ └───────┬───────┘
            │                                 │
    ┌───────┴───────┐                         │
    ▼               ▼                         ▼
┌─────────┐   ┌─────────┐             ┌─────────────┐
│ Goal    │   │ Goal    │             │ PathFinder  │
│ (优先级)│   │ (优先级)│             │ (A* 算法)   │
└─────────┘   └─────────┘             └──────┬──────┘
                                            │
                                    ┌───────┴───────┐
                                    ▼               ▼
                            ┌─────────────┐ ┌─────────────┐
                            │NodeProcessor│ │  Region     │
                            │(节点生成)   │ │(世界访问)   │
                            └─────────────┘ └─────────────┘
```

**数据流**：
1. `GoalSelector` 按 tick 调度，选择当前应执行的目标
2. 目标通过 `Controller`（Look/Move/Jump）控制实体行为
3. 移动类目标通过 `PathNavigator` 计算和执行路径
4. `PathNavigator` 调用 `PathFinder` 进行 A* 寻路
5. `PathFinder` 通过 `NodeProcessor` 和 `Region` 访问世界数据

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

| 模块 | 依赖内容 |
|------|----------|
| `entity/core` | `MobEntity`、`CreatureEntity`、`LivingEntity` - AI 目标持有者 |
| `entity/attribute` | `Attributes` - 移动速度、跟随范围等属性 |
| `world` | `IWorld` - 世界查询接口 |
| `util/math` | `Random` - 随机数生成、`Vector3` - 位置计算 |

### 下游依赖（依赖本模块的外部模块）

| 模块 | 使用方式 |
|------|----------|
| `entity/entities/passive/*` | 各种动物使用 Goal 系统（猪、牛、羊等） |
| `entity/entities/monster/*` | 各种怪物使用 Goal 系统（僵尸、骷髅等） |
| `entity/entities/villager/` | 村民使用 Brain + Goal 系统 |
| `entity/entities/passive/tamable/` | 驯服动物使用特殊目标（狼、猫等） |

## 容易踩的坑

### 1. 目标优先级理解错误

**问题**：以为优先级数值越大优先级越高。

**正确理解**：优先级数值**越小**优先级越高。优先级 0 比优先级 10 更高，会优先执行。

### 2. 互斥标志冲突

**问题**：两个目标设置了相同的互斥标志，导致只有一个能运行。

**解决方案**：检查 `GoalFlag` 设置。共享相同标志的目标不能同时运行：
- `GoalFlag::Move` - 移动类目标
- `GoalFlag::Look` - 视线类目标
- `GoalFlag::Jump` - 跳跃类目标
- `GoalFlag::Target` - 目标选择类

### 3. PathNavigator 需要有效的 Region

**问题**：`PathNavigator::moveTo()` 总是返回 false 或路径为空。

**原因**：`PathFinder` 通过 `NodeProcessor` 访问世界数据，如果 `Region` 未设置，所有节点都返回 `Blocked`。

**解决方案**：确保在寻路前设置有效的 `Region`。

### 4. 移动控制器未每帧更新

**问题**：调用了 `setMoveTo()` 但实体不移动。

**原因**：`MovementController::tick()` 未被调用。

**解决方案**：确保在实体 `tick()` 方法中调用所有控制器的 `tick()`。

### 5. 控制器引用的实体被销毁

**问题**：控制器持有的 `m_mob` 指针变成悬空指针。

**解决方案**：确保控制器生命周期与实体同步，实体销毁时清除控制器。

### 6. 寻路性能问题

**问题**：寻路计算耗时过长。

**解决方案**：
- 设置合理的 `maxSearchDistance`
- 使用 `searchRange` 限制搜索范围
- 缓存常用路径
- 在后台线程计算路径（需要额外实现）

### 7. Brain 系统框架未完全实现

**问题**：尝试使用 Brain 系统时发现功能不完整。

**状态**：Brain 框架已完成基础结构，但具体任务和部分传感器有 TODO。对于需要复杂 AI 的实体（村民、劫掠兽等），建议先用 Goal 系统实现。

### 8. 特殊移动控制器

**注意**：`GhastMovementController` 和 `VexMovementController` 是特殊的飞行控制器，不要与普通 `MovementController` 混用。飞行实体需要使用专门的移动控制器。

### 9. 特殊节点处理器

**注意**：`RavagerNodeProcessor` 是专门为劫掠兽设计的节点处理器，它可以破坏某些方块。普通实体使用 `WalkNodeProcessor`。

### 10. GoalConstants 使用

**建议**：在实现新目标时，优先使用 `GoalConstants` 中定义的常量（距离、时间、概率、速度等），而不是硬编码数值，便于统一调整和保持一致性。
