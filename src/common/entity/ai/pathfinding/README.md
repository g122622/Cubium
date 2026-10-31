# Pathfinding 寻路系统

寻路系统实现 Minecraft 1.21.11 的 A* 寻路与路径跟随，为生物实体提供「从当前位置走到目标」的能力。

本目录的每个类都与原版 `net.minecraft.world.level.pathfinder` 包中的同名类一一对应（`NodeEvaluator` ↔ `NodeEvaluator`、`WalkNodeEvaluator` ↔ `WalkNodeEvaluator`、`PathFinder` ↔ `PathFinder`、`Path` ↔ `Path`、`PathNavigator` ↔ `PathNavigation`/`GroundPathNavigation`），
行为细节（代价表、判定顺序、边界条件）以原版为准。**修改本目录时必须以原版源码为对照，不得凭直觉调整常数或判定顺序**，因为很多看起来任意的细节（枚举次序、循环顺序、`>=` 与 `>`）会直接改变 A* 的取舍结果。

## 目录结构

```
src/common/entity/ai/pathfinding/
├── GroundPathNavigator.hpp/.cpp  # 地面导航器（对应原版 GroundPathNavigation）
├── FlyNodeEvaluator.hpp/.cpp     # 飞行求值器（26 方向立体移动）
├── NodeEvaluator.hpp/.cpp        # 求值器基类、PathfindingEntity 契约、PathfindingContext
├── Path.hpp/.cpp                 # 路径对象（节点序列 + 游标）
├── PathFinder.hpp/.cpp           # A* 寻路器（开放集、启发式、回溯）
├── PathHeap.hpp                  # 二叉堆（A* 开放集，对应原版 BinaryHeap）
├── PathNavigator.hpp/.cpp        # 导航器基类（建路径、跟随、卡住检测）
├── PathPoint.hpp                 # 路径节点（对应原版 Node）
├── PathType.hpp                  # 路径类型枚举 + 默认代价表
├── Region.hpp                    # 寻路只读世界视图（对应原版 CollisionGetter 子集）
├── Target.hpp                    # 目标节点（对应原版 Target）
├── WalkNodeEvaluator.hpp/.cpp    # 地面求值器（跳跃、跌落、门、栅栏、窄实体）
└── WorldRegion.hpp               # 基于 IWorld 的 Region 实现
```

## 内部模块关系

```
PathNavigator（导航器基类：建路径 / 跟随 / 卡住检测）
├── GroundPathNavigator（地面导航：地表吸附、阳光避让）
└── PathFinder（A* 核心）
    ├── NodeEvaluator（求值器基类：节点缓存、能力开关、会话管理）
    │   ├── WalkNodeEvaluator（地面：8 方向 + 跳跃/跌落 + 门/栅栏/铁轨/窄实体规则）
    │   └── FlyNodeEvaluator（飞行：26 方向立体移动）
    ├── PathHeap（开放集）
    ├── Region（世界只读视图，会话期间有效）
    │   └── WorldRegion（IWorld 适配）
    └── Path（结果）
        └── PathPoint / PathType
```

## 上下游外部依赖关系

### 上游依赖（本目录依赖的外部模块）

| 依赖项 | 用途 |
|--------|------|
| `MobEntity` | 实现 `PathfindingEntity` 契约：尺寸、抬脚高度、下落距离、代价表、包围盒、地面/水中状态 |
| `IWorld` | `WorldRegion` 从中读方块、流体、碰撞与建造高度 |
| `BlockState` / `BlockTags` / `FluidTags` | 判定方块所属的路径类型 |
| `MovementController` | 导航器每 tick 把下一个路径点交给它，由它负责实际移动 |

### 下游依赖（使用本目录的模块）

| 模块 | 使用方式 |
|------|----------|
| `MobEntity` | 持有 `PathNavigator`，通过 `navigator()->moveTo(...)` 发起寻路 |
| AI Goal / Brain Task | 读 `hasPath()` / `noPath()` / `isDone()` 决定目标是否继续 |
| `MovementController::canWalkAt` | 在会话之外现建 `WorldRegion` + `PathfindingContext` 求值 |

## 求值器契约：PathfindingEntity

求值器**不认识** `MobEntity`，只通过 `PathfindingEntity` 的九个纯虚函数读实体属性：
`pathfindingWidth/Height/StepHeight/MaxFallDistance/Malus/BoundingBox/OnGround/InWater/CanStandOnFluid`。
这样差分探针可以喂入**受控的**实体参数，把「求值器算得对不对」与「某个具体生物配置得对不对」分开验证。

生产实现就是 `MobEntity` 自己（在 `MobEntity.hpp` 中直接实现）。非 `MobEntity` 的调用方
（单元测试、差分探针）自己写一个轻量实现即可。

## 容易踩的坑

### 1. 会话生命周期

`prepare()` 打开会话、`done()` 关闭会话。会话期间求值器持有 `Region` 与实体的**裸指针**，
关闭后不得再调用 `getPathType` / `getNeighbors`。`PathFinder::findPath` 内部已保证成对调用。

### 2. 求值器缓存在会话之间必须清空

`m_pathTypeCache` / `m_collisionCache` / 节点表都只在一个会话内有效。`prepare()` 与 `done()`
都会清空它们；如果需要长时间持有求值器（导航器就是这样），不要跨世界复用同一个会话。

### 3. 路径必须按值拷贝

`Path` 持有的是 `std::vector<PathPoint>` 的值拷贝，而不是指向求值器节点缓存里那批
`PathPoint` 的指针——因为 `done()` 会释放那批节点。任何「返回节点指针」的写法都会悬垂。

### 4. `PathType` 的枚举次序是语义的一部分

`getPathTypeOfMob` 在多个格子类型同代价时「后遍历者胜出」，而原版用 `EnumSet`
（按枚举序号升序）遍历。因此这里用 `std::set<PathType>` 而不是 `unordered_set`：

- 改成 `unordered_set` 会让 `OPEN` 与 `WALKABLE`（代价都是 0）的胜负随机，
  表现为**实体找不到起点**（起点被判成 `OPEN` → 没有可展开的邻居 → 路径为空）。

### 5. 窄实体规则只改写空气类类型

`getPathTypeOfMob` 末尾的「宽 <= 1 格 + 代价 0 + 该格本身是 OPEN → 返回 OPEN」是原版的
边缘规则，用来处理「站在台阶边缘」。它绝不能把 `WALKABLE` 改写成 `OPEN`，
否则实体永远找不到可起步的落脚点。实现里已把改写范围限定为 `TRAPDOOR` / `POWDER_SNOW`。

### 6. `WALKABLE` 需要脚下有碰撞形状

`getPathTypeStatic` 只有在 `y - 1` 那格的碰撞形状**非空**时才返回 `WALKABLE`。
所以自定义 `Region` 必须让 `getBlockState()` 返回真实的方块状态（`STONE` 是可碰撞的），
而不是返回一个「什么都没有」的占位状态。

### 7. 不可达时仍会返回路径

与原版一致：一个目标都没到达时，`PathFinder` 返回「走得最远」的那条路径，只是
`Path::canReach()` 为 `false`。判断「能不能到」请用 `canReach()`，不要用 `has_value()`。

### 8. 节点上限是取节点之前自增

```
if (++searched >= limit) break;
```

`limit = maxVisitedNodes * multiplier`。当 `limit <= 1` 时一个节点都不展开。
`PathNavigator::setMaxVisitedNodesMultiplier` 影响的是**下一次** `moveTo`，
`resetMaxVisitedNodesMultiplier()` 恢复 1.0。

### 9. 门 / 栅栏 / 铁轨 / 窄实体的规则位置

| 规则 | 位置 |
|------|------|
| 关闭木门 + `canOpenDoors` + `canPassDoors` → `WALKABLE_DOOR` | `pathTypeWithinMobBb` |
| 打开的门 + `!canPassDoors` → `BLOCKED` | `pathTypeWithinMobBb` |
| 铁轨未与实体当前位置（或其下一格）对齐 → `UNPASSABLE_RAIL` | `pathTypeWithinMobBb` |
| 集合里出现 `FENCE` → 整格按 `FENCE` 处理 | `getPathTypeOfMob` |
| 对角线不允许穿过 `WALKABLE_DOOR` | `isDiagonalValid` |
| 宽 < 0.5 格可挤过两根栅栏柱 | `isDiagonalValid` |

门能力的设置入口是导航器（`setCanPassDoors` / `setCanOpenDoors` / `setCanFloat` /
`setCanWalkOverFences`），它们转发到当前挂载的 `NodeEvaluator`。注意求值器是按值挂载在
`PathFinder` 里的，所以这些开关必须在导航器存活期间设置。

### 10. 最大下落距离由实体决定

`pathfindingMaxFallDistance()` 不是常数 3：有攻击目标时按「剩余血量 + 难度」折算，
无目标时才退回默认值。搜索过程中由 `WalkNodeEvaluator::tryFindFirstGroundNodeBelow`
读取它来截断下跌搜索。

### 11. 阳光避让

`GroundPathNavigator::_trimPath` 先做锅（cauldron）路径点抬高，再做阳光截断。
截断只在 `avoidsSun()` 为真时生效（`RestrictSunGoal` 通过 `setAvoidSun(true)` 设置），
且实体自身已经暴露在阳光下时不截断，否则它会连逃离的路径都没有。

### 12. 时间量用内部 tick 计数

`PathNavigator` 用自增的 `m_tick` 而不是 `world->getGameTime()`，这样同一套逻辑
在无世界的单元测试与差分探针里也能确定性地运行。

### 13. 飞行求值器的起点枚举是确定性的

`FlyNodeEvaluator::getStart` 用固定的候选列枚举代替原版的随机采样。
这是一处**有意的偏差**：非确定性会让差分对比失去意义。其余逻辑与原版一致。