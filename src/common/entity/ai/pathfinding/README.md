# Pathfinding 寻路系统

寻路系统实现了 A* 算法，为游戏中的实体提供智能路径规划能力。该系统参考 Minecraft 1.16.5 的寻路架构设计。

## 目录结构

```
src/common/entity/ai/pathfinding/
├── FlaggedPathPoint.hpp        # 带标记的路径点（多目标寻路）
├── NodeProcessor.hpp           # 节点处理器基类
├── Path.hpp                    # 路径对象
├── Path.cpp                    # 路径对象实现
├── PathFinder.hpp              # 寻路器（A*算法核心）
├── PathFinder.cpp              # 寻路器实现
├── PathHeap.hpp                # 路径点最小堆（A*开放列表）
├── PathNavigator.hpp           # 路径导航器（路径跟随）
├── PathNavigator.cpp           # 路径导航器实现
├── PathNodeType.hpp            # 路径节点类型枚举
├── PathPoint.hpp               # 路径点（A*节点）
├── PathPoint.cpp               # 路径点实现
├── RavagerNodeProcessor.hpp    # 劫掠兽节点处理器（可穿过树叶）
├── RavagerNodeProcessor.cpp    # 劫掠兽节点处理器实现
├── Region.hpp                  # 世界区域访问接口
├── Region.cpp                  # 世界区域访问实现
├── WalkNodeProcessor.hpp       # 行走节点处理器
└── WalkNodeProcessor.cpp       # 行走节点处理器实现
```

## 内部模块关系

```
PathNavigator（路径导航器，路径跟随）
    └── PathFinder（寻路器，A*算法核心）
        ├── NodeProcessor（节点处理器基类）
        │   ├── WalkNodeProcessor（行走处理器，支持跳跃/攀爬/游泳）
        │   │   └── RavagerNodeProcessor（劫掠兽处理器，可穿过树叶）
        │   └── Region（世界区域访问接口）
        ├── PathHeap（优先队列）
        └── Path（路径对象）
            └── PathPoint（路径点/A*节点）
                └── PathNodeType（节点类型枚举）

FlaggedPathPoint（多目标寻路辅助类，独立使用）
```

## 上下游外部依赖关系

### 上游依赖（本目录依赖的外部模块）

| 依赖项 | 用途 |
|--------|------|
| `Entity` / `LivingEntity` / `MobEntity` | 实体基类，获取位置和属性 |
| `MovementController` | 移动控制器，执行实际移动 |
| `core/Types.hpp` | 基础类型定义 |
| `goal/GoalConstants.hpp` | AI目标相关常量 |
| `BlockState` / `BlockTags` | 方块状态和标签检测 |
| `math/MathConstants.hpp` | 数学常量（如SQRT2） |

### 下游依赖（使用本目录的模块）

| 模块 | 使用方式 |
|------|----------|
| `MobEntity` | 持有 PathNavigator，调用 moveTo() 进行导航 |
| `AI Goal 系统` | 通过实体获取导航器，设置移动目标 |
| `ServerWorld` | 提供 Region 实现供寻路器访问世界数据 |

## 容易踩的坑

### 1. 忘记设置 Region

寻路前必须通过 `setRegion()` 设置世界区域，否则所有节点返回 `Blocked`，导致寻路失败。

### 2. NodeProcessor 生命周期

`PathFinder` 接管 `NodeProcessor` 的所有权后，原始指针变为 nullptr。如需访问，使用 `finder.getNodeProcessor()`。

### 3. 坐标类型混淆

实体位置是 `f64`，而寻路使用 `i32` 块坐标。调用寻路接口前需要用 `std::floor()` 转换。

### 4. Path 对象生命周期

`PathNavigator` 持有 `Path` 的所有权，调用 `clearPath()` 或新的 `moveTo()` 会释放旧路径。获取 `getPath()` 后不要再修改导航状态。

### 5. 实体尺寸未设置

必须通过 `setEntitySize()` 设置正确的实体宽度和高度，否则碰撞检测不准确，可能导致穿墙或卡住。

### 6. 搜索距离过大

`setMaxSearchDistance(1000)` 或 `setMaxNodes(50000)` 会导致性能问题甚至内存爆炸。推荐值：距离 100，节点 2000。

### 7. 对角线移动被阻挡

对角线移动需要两个相邻方向都可通行。WalkNodeProcessor 会自动检查，但如果自定义处理器需注意此逻辑。

### 8. m_distanceToNext 存储的是 h*1.5

PathPoint 的 `m_distanceToNext` 字段存储的是 `启发式值 * 1.5`（MC 1.16.5 的 distanceToNext），不是到下一个点的实际距离。这是 A* 算法的优化，避免重复计算。

### 9. CHAIN 和 WATER 相关节点类型未实现

`PathNodeType` 中部分类型（如 `WalkableDoor`, `Trapdoor`, `FenceGate`）已定义但 WalkNodeProcessor 可能未完全处理，使用前需确认具体实现。

### 10. Region 接口需要完整实现

自定义 Region 时，`getBlockStateId()` 和 `isLoaded()` 是核心方法，必须正确实现。默认的 `getBlockState()` 依赖 `getBlockStateId()`。
