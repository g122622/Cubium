# AI 模块 (entity/ai)

本目录包含 Minecraft Reborn 的实体人工智能系统，实现了类似 Minecraft Java Edition 1.16.5 的 AI 架构。

## 目录结构

```
ai/
├── brain/                  # 大脑系统（预留，未实现）
│   ├── sensor/             # 传感器系统（预留）
│   └── task/               # 任务系统（预留）
├── controller/             # 控制器 - 实体的底层行为控制
│   ├── JumpController.hpp/cpp    # 跳跃控制器
│   ├── LookController.hpp/cpp    # 视线控制器
│   └── MovementController.hpp/cpp # 移动控制器
├── goal/                   # 目标系统 - 实体的行为决策
│   ├── Goal.hpp                  # 目标基类
│   ├── GoalConstants.hpp         # 目标常量定义
│   ├── GoalFlag.hpp              # 目标互斥标志
│   ├── GoalSelector.hpp          # 目标选择器
│   ├── PrioritizedGoal.hpp       # 带优先级的目标包装器
│   └── goals/                    # 具体目标实现
│       ├── AvoidEntityGoal.hpp/cpp    # 避开实体目标
│       ├── BreedGoal.hpp/cpp          # 繁殖目标
│       ├── FollowParentGoal.hpp/cpp   # 跟随父母目标
│       ├── LookAtGoal.hpp/cpp         # 看向目标
│       ├── MeleeAttackGoal.hpp/cpp    # 近战攻击目标
│       ├── PanicGoal.hpp/cpp          # 恐慌逃跑目标
│       ├── RandomWalkingGoal.hpp/cpp  # 随机漫步目标
│       ├── SwimGoal.hpp/cpp           # 游泳目标
│       └── TemptGoal.hpp/cpp          # 食物诱惑目标
└── pathfinding/            # 寻路系统 - 路径计算和导航
    ├── NodeProcessor.hpp         # 节点处理器基类
    ├── Path.hpp                  # 路径对象
    ├── PathFinder.hpp/cpp        # 寻路器（A* 算法）
    ├── PathHeap.hpp              # 路径点最小堆
    ├── PathNavigator.hpp/cpp     # 路径导航器
    ├── PathNodeType.hpp          # 路径节点类型
    ├── PathPoint.hpp/cpp         # 路径点
    ├── Region.hpp                # 世界区域访问接口
    └── WalkNodeProcessor.hpp/cpp # 行走节点处理器
```

## 子系统详解

### 1. 控制器系统 (controller/)

控制器负责实体的底层行为控制，是目标系统与实体物理状态之间的桥梁。

#### LookController - 视线控制器

**职责**: 控制实体的头部旋转，使其看向指定位置。

**核心方法**:
- `setLookPosition(x, y, z)` - 设置看向位置
- `setLookPosition(x, y, z, deltaYaw, deltaPitch)` - 设置看向位置（带旋转速度限制）
- `tick()` - 每帧更新，平滑旋转实体头部

**工作原理**:
1. 设置目标位置后，计算目标偏航角和俯仰角
2. 在 `tick()` 中限制旋转速度，平滑过渡
3. 使用 `clampedRotate()` 确保角度变化不超过最大速度

#### MovementController - 移动控制器

**职责**: 控制实体的移动行为，包括移动到目标位置和横向移动。

**动作类型** (`MoveAction`):
- `Wait` - 等待状态
- `MoveTo` - 移动到目标位置
- `Strafe` - 横向移动
- `Jumping` - 跳跃中

**核心方法**:
- `setMoveTo(x, y, z, speed)` - 设置移动目标
- `strafe(forward, strafe)` - 设置横向移动
- `tick()` - 每帧更新，计算方向并设置实体移动

**工作原理**:
1. 计算从当前位置到目标的方向
2. 限制旋转速度（每 tick 最多 30 度）
3. 根据实体速度属性设置移动速度
4. 检测是否需要跳跃（目标位置更高且水平距离近）

#### JumpController - 跳跃控制器

**职责**: 控制实体的跳跃行为。

**核心方法**:
- `setJumping()` - 设置跳跃状态
- `tick()` - 将跳跃状态应用到实体

**工作原理**: 简单的跳跃触发器，设置后在下一 tick 触发实体跳跃并重置状态。

---

### 2. 目标系统 (goal/)

目标系统实现了实体的行为决策机制，基于优先级和互斥标志协调多个 AI 行为。

#### Goal - 目标基类

**核心方法**:
```cpp
virtual bool shouldExecute() = 0;           // 是否应该开始执行
virtual bool shouldContinueExecuting();      // 是否应该继续执行
virtual bool isPreemptible() const;          // 是否可以被抢占
virtual void startExecuting();               // 开始执行
virtual void resetTask();                    // 重置任务
virtual void tick();                         // 每帧更新
```

**互斥标志** (`GoalFlag`):
- `Move` - 移动
- `Look` - 视线
- `Jump` - 跳跃
- `Target` - 目标选择

共享相同标志的目标不能同时运行，用于协调互斥行为。

#### PrioritizedGoal - 带优先级的目标包装器

**职责**: 包装目标并添加优先级信息，实现抢占机制。

**抢占规则**: 高优先级（数值更小）的目标可以抢占低优先级目标，前提是被抢占目标是可抢占的。

#### GoalSelector - 目标选择器

**职责**: 管理实体的所有 AI 目标，负责选择和执行当前应该运行的目标。

**工作流程**:
1. **清理阶段**: 停止不再满足条件或被禁用标志的目标
2. **选择阶段**: 遍历所有目标，检查是否可以启动
3. **更新阶段**: 对正在运行的目标调用 `tick()`

**核心方法**:
- `addGoal(priority, goal)` - 添加目标
- `removeGoal(goal)` - 移除目标
- `tick()` - 每帧更新
- `disableFlag(flag)` / `enableFlag(flag)` - 禁用/启用标志

#### GoalConstants - 常量定义

定义了各类目标使用的距离、时间、概率、速度常量。

**距离常量**:
| 常量 | 值 | 用途 |
|------|-----|------|
| `DEFAULT_FOLLOW_DISTANCE` | 10.0f | 默认跟随距离 |
| `BREED_DETECTION_RANGE` | 8.0f | 繁殖检测范围 |
| `TEMPT_RANGE` | 10.0f | 诱惑检测范围 |
| `AVOID_DETECTION_RANGE` | 16.0f | 避开检测范围 |
| `MELEE_ATTACK_REACH` | 2.0f | 近战攻击范围 |

**时间常量**:
| 常量 | 值 | 用途 |
|------|-----|------|
| `MELEE_ATTACK_COOLDOWN` | 20 | 攻击冷却（tick） |
| `PATH_RECALCULATE_INTERVAL` | 5 | 路径重算间隔 |
| `TEMPT_COOLDOWN` | 100 | 诱惑冷却（tick） |

#### 具体目标实现

| 目标类 | 互斥标志 | 功能描述 |
|--------|----------|----------|
| `AvoidEntityGoal` | Move | 避开特定类型的实体 |
| `BreedGoal` | Move, Look | 两只动物靠近并繁殖 |
| `FollowParentGoal` | Move | 幼体动物跟随成年动物 |
| `FollowOwnerGoal` | Move | 驯服动物跟随主人 |
| `SitGoal` | Jump | 驯服动物坐下 |
| `BegGoal` | Look | 狼向玩家乞求食物 |
| `LookAtGoal` | Look | 看向附近的实体 |
| `LookRandomlyGoal` | Look | 随机看向某个方向 |
| `MeleeAttackGoal` | Move, Look | 近战攻击目标实体 |
| `RangedAttackGoal` | Move, Look | 远程攻击目标实体 |
| `RangedBowAttackGoal` | Move, Look | 使用弓箭攻击 |
| `PanicGoal` | Move | 受伤或着火时随机逃跑 |
| `RandomWalkingGoal` | Move | 随机选择方向移动 |
| `SwimGoal` | Jump | 在水中或岩浆中向上游动 |
| `TemptGoal` | Move, Look | 被玩家手持物品诱惑 |
| `EatGrassGoal` | Move, Look | 羊吃草 |
| `FlyGoal` | Move | 飞行目标 |
| `SleepGoal` | Move, Look, Jump | 村民睡觉 |
| `WorkAtPoiGoal` | Move | 村民工作 |
| `FindShelterGoal` | Move | 寻找遮蔽 |
| `FleeSunGoal` | Move | 亡灵逃离阳光 |
| `ReturnToHomeGoal` | Move | 返回家 |
| `TradeWithPlayerGoal` | Move, Look | 村民交易 |
| `ShowWaresGoal` | Look | 村民展示商品 |
| `HurtByTargetGoal` | Target | 被攻击后反击 |
| `NearestAttackableTargetGoal` | Target | 攻击最近目标 |

---

### 3. 寻路系统 (pathfinding/)

寻路系统实现 A* 算法，支持地面行走实体的路径计算和导航。

#### PathPoint - 路径点

**属性**:
- 位置坐标 (x, y, z)
- 代价信息: `costFromStart` (g), `heuristic` (h), `totalCost` (f)
- `distanceToNext` - 到下一个路径点的距离（MC: 存储 h*1.5）
- `walkedDistance` - 行走距离（MC: field_222861_j），用于限制搜索范围
- `costMalus` - 节点类型代价惩罚
- `nodeType` - 节点类型
- `parent` - 父节点（用于重建路径）
- `heapIndex` - 在堆中的索引

**方法**:
- `distanceTo(other)` - 直线距离（MC: distanceTo）
- `distanceManhattan(other)` - 曼哈顿距离（MC: func_224757_c）
- `distanceToSq(other)` - 直线距离平方
- `distanceToSq(x, y, z)` - 直线距离平方（避免创建临时对象）
- `clone()` - 克隆节点（不复制寻路状态）
- `cloneMove(x, y, z)` - 创建移动克隆（复制所有寻路状态）

#### PathNodeType - 路径节点类型

| 类型 | 描述 | 代价惩罚 |
|------|------|----------|
| `Blocked` | 完全阻塞 | 0.0（不可通行） |
| `Walkable` | 可行走地面 | 0.0 |
| `Water` | 水 | 0.0 |
| `Lava` | 岩浆 | -1.0（极度危险） |
| `DangerFire` | 火焰危险 | -1.0 |
| `DangerFall` | 跌落危险 | -1.0 |
| `Climbable` | 可攀爬 | 0.0 |

#### PathHeap - 路径点最小堆

用于 A* 算法的开放列表，按总代价排序。

**核心操作**:
- `insert(point)` - 插入节点
- `pop()` - 弹出最小代价节点
- `update(point)` - 更新节点位置（代价改变后）

#### Path - 路径对象

**功能**: 存储从起点到终点的完整路径。

**核心方法**:
- `getCurrentTarget()` - 获取当前目标路径点
- `advance()` - 前进到下一个路径点
- `isFinished()` - 检查是否到达终点
- `buildFromEnd(endPoint)` - 从终点反向构建路径
- `reachesTarget(x, y, z, tolerance)` - 检查是否到达目标位置

#### NodeProcessor - 节点处理器基类

**职责**: 生成和缓存路径节点，计算相邻节点。

**核心方法**:
- `getNodeType(x, y, z)` - 获取指定位置的节点类型
- `getStartNode(x, y, z)` - 获取起始节点
- `getNeighbors(current)` - 获取相邻节点

#### WalkNodeProcessor - 行走节点处理器

**支持的移动类型**:
- 水平移动（4 方向 + 4 对角线）
- 跳跃上台阶
- 跌落
- 攀爬（梯子、藤蔓）
- 水中游泳

**配置选项**:
- `setCanSwim(bool)` - 是否可以游泳
- `setCanOpenDoors(bool)` - 是否可以开门
- `setCanEnterDoors(bool)` - 是否可以通过门
- `setCanClimb(bool)` - 是否可以攀爬
- `setMaxFallDistance(i32)` - 最大跌落距离
- `setAvoidWater(bool)` - 是否避免水
- `setAvoidSun(bool)` - 是否避免阳光

#### PathFinder - 寻路器

**职责**: 实现 A* 算法寻找最优路径。

**核心方法**:
- `findPath(startX, startY, startZ, targetX, targetY, targetZ, maxDistance)` - 寻找到坐标的路径
- `findPathToRange(start, target, range)` - 寻找到目标范围内的路径

**算法流程**:
1. 初始化开放列表，加入起始节点
2. 循环取出代价最小的节点
3. 检查是否到达目标
4. 扩展相邻节点，更新代价
5. 直到找到路径或搜索完所有节点

**启发式函数**: 曼哈顿距离

#### PathNavigator - 路径导航器

**职责**: 管理路径计算、沿路径移动实体、处理路径中断和卡住检测。

**核心方法**:
- `moveTo(x, y, z, speed)` - 计算并开始路径
- `tick()` - 每帧更新，沿路径移动
- `recomputePath()` - 重新计算路径
- `hasPath()` / `noPath()` - 检查路径状态
- `isDone()` - 检查是否完成
- `isStuck()` - 检查是否卡住（MC 1.16.5: func_244428_t_）

**卡住检测** (MC 1.16.5):
- 每 100 tick 检查实体是否移动超过 1.5 格
- 超时检测：同一节点停留时间超过预期 3 倍

**工作流程**:
1. 调用 `PathFinder` 计算路径
2. 在 `tick()` 中沿路径移动
3. 检测是否到达当前路径点
4. 前进到下一个路径点
5. 如果目标移动过远，重新计算路径

---

### 4. 大脑系统 (brain/)

大脑系统提供比Goal系统更高级的AI控制，支持记忆、传感器、任务和日程。

**实现状态**: 框架完成，具体实现待完善

| 组件 | 状态 | 说明 |
|------|------|------|
| Brain | ✅ | 模板类完成 |
| Memory | ✅ | 支持TTL |
| MemoryModuleType | ⚠️ | 46种（计划85+种） |
| Activity | ✅ | 15种活动 |
| Schedule | ✅ | 4种日程 |
| Sensor | ⚠️ | 8种传感器，update()有TODO |
| Task | ⚠️ | 基类完成，无具体任务 |

#### Brain - 大脑核心

**核心组件**:
- **Memory**: 记忆模块，存储短期和长期记忆
- **Sensor**: 传感器，自动更新记忆
- **Task**: 任务，基于记忆执行行为
- **Schedule**: 日程，基于时间切换活动

```cpp
// 使用Brain
Brain<VillagerEntity> brain;
brain.registerMemory(MemoryModuleTypes::HOME);
brain.registerMemory(MemoryModuleTypes::JOB_SITE);
brain.registerSensor(std::make_unique<NearestPlayersSensor<VillagerEntity>>());
brain.setSchedule(villagerSchedule);
```

#### Sensor - 传感器

| 传感器 | 功能 |
|--------|------|
| `NearestPlayersSensor` | 检测附近玩家 |
| `NearestVisibleLivingEntitySensor` | 检测可见生物 |
| `HurtBySensor` | 检测伤害来源 |
| `MobSensor` | 检测附近生物 |
| `WorkStationSensor` | 检测工作站点 |
| `VillagePoiSensor` | 检测床和集会点 |
| `BabySensor` | 检测幼年和成年实体 |
| `AvoidEntitySensor` | 检测避险目标 |

#### MemoryModuleType - 记忆模块类型

| 类型 | 描述 |
|------|------|
| `HOME` | 家的位置 |
| `JOB_SITE` | 工作站点 |
| `MOBS` | 附近生物列表 |
| `VISIBLE_MOBS` | 可见生物列表 |
| `NEAREST_PLAYERS` | 附近玩家 |
| `ATTACK_TARGET` | 攻击目标 |
| `HURT_BY_ENTITY` | 受伤来源 |
| `PATH` | 当前路径 |

---

## 模块整体职责

### 输入
- **实体状态**: 位置、速度、属性、当前目标
- **世界信息**: 方块、其他实体、物品
- **外部事件**: 受伤、玩家交互、环境变化

### 输出
- **移动指令**: 通过 `MovementController` 设置移动方向和速度
- **视线方向**: 通过 `LookController` 设置头部旋转
- **跳跃指令**: 通过 `JumpController` 触发跳跃
- **行为决策**: 选择和切换 AI 目标

### 依赖项
- `mc::entity::MobEntity` - 生物实体基类
- `mc::entity::CreatureEntity` - 生物实体（有 AI）
- `mc::entity::LivingEntity` - 生物实体（有生命值）
- `mc::entity::attribute::Attributes` - 属性系统
- `mc::world::IWorld` - 世界接口
- `mc::math::Random` - 随机数生成器

---

## 使用方法

### 1. 为实体添加 AI 目标

```cpp
// 在实体构造函数中
void MyMob::registerGoals() {
    // 添加目标（优先级越小越高）
    m_goalSelector.addGoal(0, std::make_unique<SwimGoal>(this));
    m_goalSelector.addGoal(1, std::make_unique<PanicGoal>(this, 1.5));
    m_goalSelector.addGoal(2, std::make_unique<RandomWalkingGoal>(this, 1.0));
    m_goalSelector.addGoal(3, std::make_unique<LookAtGoal>(this, 8.0f));
}
```

### 2. 每帧更新

```cpp
void MyMob::tick() {
    MobEntity::tick();  // 调用基类

    // 更新 AI 目标
    m_goalSelector.tick();

    // 更新控制器
    if (m_lookController) m_lookController->tick();
    if (m_moveController) m_moveController->tick();
    if (m_jumpController) m_jumpController->tick();
}
```

### 3. 使用寻路系统

```cpp
// 创建导航器
auto navigator = std::make_unique<PathNavigator>(this);

// 移动到目标位置
navigator->moveTo(targetX, targetY, targetZ, speed);

// 每帧更新
navigator->tick();

// 检查状态
if (navigator->noPath()) {
    // 没有找到路径或路径完成
}
```

---

## 容易踩的坑

### 1. 目标优先级理解错误

**问题**: 以为优先级数值越大优先级越高。

**正确理解**: 优先级数值**越小**优先级越高。例如，优先级 0 比 优先级 10 更高。

### 2. 互斥标志冲突

**问题**: 两个目标设置了相同的互斥标志，导致只有一个能运行。

**解决方案**: 检查 `GoalFlag` 设置，确保不需要同时运行的目标使用相同标志，需要同时运行的 goals 使用不同标志。

### 3. 目标未设置互斥标志

**问题**: 所有目标都没有设置互斥标志，导致所有满足条件的目标同时运行。

**解决方案**: 根据行为类型设置正确的互斥标志：
- 移动类目标设置 `GoalFlag::Move`
- 视线类目标设置 `GoalFlag::Look`
- 跳跃类目标设置 `GoalFlag::Jump`

### 4. PathNavigator 需要有效的 Region

**问题**: `PathNavigator::moveTo()` 总是返回 false。

**原因**: `PathFinder` 需要通过 `NodeProcessor` 访问世界数据，如果 `Region` 未设置，所有节点都会返回 `Blocked`。

**解决方案**: 确保在使用寻路前设置有效的 `Region`。

### 5. 移动控制器未每帧更新

**问题**: 调用了 `setMoveTo()` 但实体不移动。

**原因**: `MovementController::tick()` 未被调用。

**解决方案**: 确保在实体 `tick()` 方法中调用控制器的 `tick()`。

### 6. 目标的 shouldExecute() 缓存状态

**问题**: `shouldExecute()` 每帧都被调用，但每次都重新计算昂贵的状态。

**优化建议**: 在 `shouldExecute()` 中缓存计算结果，避免重复计算。

### 7. 寻路性能问题

**问题**: 寻路计算耗时过长。

**解决方案**:
- 设置合理的 `maxSearchDistance`
- 使用 `searchRange` 限制搜索范围
- 缓存常用路径
- 在后台线程计算路径

### 8. 控制器引用的实体被销毁

**问题**: 控制器持有的 `m_mob` 指针变成悬空指针。

**解决方案**: 确保控制器生命周期与实体同步，或在实体销毁时清除控制器。

---

## 测试用例

测试文件位于 `tests/entity/` 目录：

### GoalTests.cpp
- `EnumSetTest` - 枚举集合测试
- `GoalTest` - 目标基类测试
- `PrioritizedGoalTest` - 优先级目标测试
- `GoalSelectorTest` - 目标选择器测试
- `GoalFlagTest` - 目标标志测试

### PathfindingTests.cpp
- `PathNodeTypeTest` - 节点类型测试
- `PathPointTest` - 路径点测试
- `PathHeapTest` - 路径堆测试
- `PathTest` - 路径对象测试

### RandomWalkingGoalTest.cpp
- `RandomWalkingGoalTest` - 随机漫步目标测试
- `CreatureEntityMoveTest` - 生物实体移动测试
- `MovementControllerTest` - 移动控制器测试
- `RandomWalkingGoalIntegrationTest` - 集成测试

---

## 参考实现

本模块参考 Minecraft Java Edition 1.16.5 的 AI 系统设计：

- `Goal` 对应 `net.minecraft.entity.ai.goal.Goal`
- `GoalSelector` 对应 `net.minecraft.entity.ai.goal.GoalSelector`
- `PrioritizedGoal` 对应 `net.minecraft.entity.ai.goal.PrioritizedGoal`
- `PathNavigator` 对应 `net.minecraft.pathfinding.PathNavigator`
- `PathFinder` 对应 `net.minecraft.pathfinding.PathFinder`
- `NodeProcessor` 对应 `net.minecraft.pathfinding.NodeProcessor`
- `LookController` 对应 `net.minecraft.entity.ai.controller.LookController`
- `MovementController` 对应 `net.minecraft.entity.ai.controller.MovementController`
- `JumpController` 对应 `net.minecraft.entity.ai.controller.JumpController`
