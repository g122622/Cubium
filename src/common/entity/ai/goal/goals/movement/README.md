# 移动类目标 (Movement Goals)

## 目录结构

```
movement/
├── MovementGoals.hpp/cpp                 # 水避让随机漫步、跳跃攻击、向目标移动、向家范围移动目标
├── FollowSchoolLeaderGoal.hpp/cpp        # 跟随群体领导者目标（群游鱼类）
├── FollowMobGoal.hpp/cpp                 # 跟随附近生物目标（鹦鹉等）
├── WaterAvoidingRandomFlyingGoal.hpp/cpp # 水避让随机飞行目标（飞行实体）
└── README.md                             # 本文档
```

## 内部模块关系

```
MovementGoals.hpp
├── WaterAvoidingRandomWalkingGoal    # 避开水随机行走
├── LeapAtTargetGoal                  # 跳跃攻击
├── MoveTowardsTargetGoal             # 向攻击目标移动
└── MoveTowardsRestrictionGoal        # 向家范围移动

FollowSchoolLeaderGoal.hpp
└── FollowSchoolLeaderGoal            # 依赖 AbstractGroupFishEntity

FollowMobGoal.hpp
└── FollowMobGoal                     # 通用跟随生物目标

WaterAvoidingRandomFlyingGoal.hpp
└── WaterAvoidingRandomFlyingGoal     # 飞行实体专用
```

## 上下游外部依赖关系

**本目录依赖**：
- `Goal.hpp` - 目标基类
- `GoalFlag.hpp` - 目标标志枚举
- `CreatureEntity` / `MobEntity` / `LivingEntity` - 生物实体
- `AbstractGroupFishEntity` - 群游鱼类实体（FollowSchoolLeaderGoal）
- `RandomPositionGenerator` - 随机位置生成器
- `PathNavigator` - 路径导航
- `EntityUtils` - 实体查询工具

**被依赖**：
- 各实体类在 `registerGoals()` 中注册这些目标：
  - `IronGolemEntity` → `MoveTowardsTargetGoal`
  - `GuardianEntity` → `MoveTowardsRestrictionGoal`
  - `ParrotEntity` → `FollowMobGoal`, `WaterAvoidingRandomFlyingGoal`
  - `CodEntity` / `SalmonEntity` / `TropicalFishEntity` → `FollowSchoolLeaderGoal`

## 容易踩的坑

### 1. 冷却机制（FollowSchoolLeaderGoal）

每帧都搜索会导致性能问题。MC 1.16.5 使用 `200 + random.nextInt(200) % 20` 作为冷却时间（范围 200~219 ticks，约 10~11 秒）。

### 2. 首领选举逻辑（FollowSchoolLeaderGoal）

错误地将所有鱼都设为跟随者会导致无首领状态。正确逻辑：第一个找到的可扩群首领或自己成为首领。

### 3. 群体大小限制（FollowSchoolLeaderGoal）

无限加入导致群体过大。必须使用 `canGroupGrow()` 检查是否还能扩群。

### 4. 实体查询类型

使用 `EntityUtils::findEntities<Entity>()` 会返回所有实体，应使用具体类型如 `EntityUtils::findEntities<AbstractGroupFishEntity>()`。

### 5. 避水/避岩浆逻辑

`WaterAvoidingRandomWalkingGoal` 和 `WaterAvoidingRandomFlyingGoal` 都需要在选择位置时检查是否在水或岩浆中，否则生物会走进水域。

### 6. 家范围系统（MoveTowardsRestrictionGoal）

依赖 `MobEntity` 的家范围系统：
- `setHomePosAndDistance(pos, distance)` 设置家位置和范围
- `isWithinHomeDistanceCurrentPosition()` 检查是否在家范围内
- 未设置家范围时，`shouldExecute()` 可能始终返回 false

### 7. 飞行 vs 行走的区别

`WaterAvoidingRandomFlyingGoal` 与 `WaterAvoidingRandomWalkingGoal` 的关键区别：
- 飞行目标不依赖地面导航，直接设置目标位置
- 飞行目标可以在三维空间中选择目标点
- 飞行目标使用飞行移动控制器

### 8. 目标距离范围（LeapAtTargetGoal）

跳跃攻击有距离限制：`MIN_DISTANCE = 4.0f`，`MAX_DISTANCE = 8.0f`。目标太近或太远都不会执行跳跃。
