# Teleport 传送系统

处理维度间传送，包括下界传送门和末地传送门。

## 目录结构

```
teleport/
├── PortalSize.hpp      # 传送门尺寸检测
├── PortalSize.cpp      # 传送门尺寸检测实现
├── Teleporter.hpp      # 传送器基类和子类
├── Teleporter.cpp      # 传送器实现
└── README.md           # 本文档
```

## 文件详解

### PortalSize.hpp/cpp

**职责**: 检测传送门框架尺寸，参考 MC 1.16.5 PortalSize。

**常量定义**:
```cpp
static constexpr i32 MIN_WIDTH = 2;       // 最小宽度
static constexpr i32 MAX_WIDTH = 21;      // 最大宽度
static constexpr i32 MIN_HEIGHT = 3;      // 最小高度
static constexpr i32 MAX_HEIGHT = 21;     // 最大高度
static constexpr i32 MAX_SEARCH_DOWN = 21; // 向下搜索最大深度
```

**PortalSizeResult 结构**:
| 字段 | 类型 | 说明 |
|------|------|------|
| `corner` | `BlockPos` | 传送门内部左下角位置 |
| `width` | `i32` | 内部宽度 (2-21) |
| `height` | `i32` | 内部高度 (3-21) |
| `axis` | `Direction` | 传送门朝向 (East/South) |
| `portalBlockCount` | `i32` | 已存在的传送门方块数量 |
| `valid` | `bool` | 是否有效 |

**主要方法**:
| 方法 | 描述 |
|------|------|
| `findNetherPortal(world, pos, preferXAxis)` | 寻找下界传送门框架 |
| `lightNetherPortal(world, portal)` | 点燃下界传送门（放置传送门方块）|
| `canConnect(state)` | 检查方块是否可作为传送门内部（空气/火/传送门方块）|
| `isPortalFrame(state)` | 检查方块是否为框架方块（黑曜石）|

**检测算法（MC 1.16.5）**:
1. 从火焰位置向下搜索最多 21 格找到内部底部
2. 向左搜索找到左边框架（黑曜石）
3. 计算宽度：向右检查内部方块和底部框架
4. 计算高度：向上检查左右框架和内部方块
5. 验证顶部框架完整性
6. 返回传送门位置和尺寸

**使用示例**:
```cpp
// 寻找传送门框架
auto result = PortalSize::findNetherPortal(world, pos, true);
if (result.has_value() && result->valid && result->portalBlockCount == 0) {
    // 找到未点燃的传送门框架，点燃它
    PortalSize::lightNetherPortal(world, *result);
}
```

### Teleporter.hpp/cpp

**职责**: 处理实体在维度间的传送。

**类层次**:
```
Teleporter (基类)
├── NetherTeleporter (下界传送器)
└── EndTeleporter (末地传送器)
```

**Teleporter 基类**:
```cpp
class Teleporter {
public:
    virtual ~Teleporter() = default;

    virtual bool teleport(Entity& entity, DimensionId targetDim) = 0;
    virtual std::optional<PortalInfo> findPortal(IWorld& world, const Vector3d& pos) = 0;
    virtual PortalInfo createPortal(IWorld& world, const Vector3d& pos) = 0;
    virtual f32 getCoordinateScale() const { return 1.0f; }

    // 静态坐标转换
    static Vector3d transformPosition(const Vector3d& pos, const DimensionType& from, const DimensionType& to);

    // 传送门搜索半径
    static constexpr i32 NETHER_SEARCH_RADIUS = 128;

    // 末地出生位置
    static Vector3d getEndSpawnPosition() { return Vector3d(100.0, 49.0, 0.0); }

protected:
    static std::vector<BlockPos> searchPortalBlocks(IWorld& world, const BlockPos& center, i32 radius);
    static void placePortalBlocks(IWorld& world, const BlockPos& corner, i32 width, i32 height, Direction axis);
};
```

**NetherTeleporter**:
- 坐标转换: 主世界 ↔ 下界 1:8
- 传送门搜索半径: 128 格（主世界和下界统一）
- 自动创建黑曜石框架和传送门方块
- `findPortal()`: 搜索已存在的传送门方块
- `createPortal()`: 创建新传送门（黑曜石框架 + 传送门方块）

**EndTeleporter**:
- 固定出生位置: (100, 49, 0)
- 无坐标缩放
- 自动创建黑曜石平台
- `findPortal()`: 返回固定位置（末地无传送门方块搜索）
- `createPortal()`: 创建黑曜石出生平台

**使用示例**:
```cpp
// 下界传送：搜索或创建传送门
NetherTeleporter teleporter;
auto portalInfo = teleporter.findPortal(world, targetPos);
if (!portalInfo.has_value() || !portalInfo->valid) {
    portalInfo = teleporter.createPortal(world, targetPos);
}

// 坐标转换
Vector3d netherPos = Teleporter::transformPosition(
    overworldPos,
    DimensionType::overworld(),
    DimensionType::nether());

// 获取末地出生点
Vector3d endSpawn = Teleporter::getEndSpawnPosition();
```

## 坐标转换

### 主世界 ↔ 下界

```
主世界坐标 → 下界坐标: 坐标 ÷ 8
下界坐标 → 主世界坐标: 坐标 × 8
```

```cpp
// 主世界 (800, 64, 200) → 下界 (100, 64, 25)
Vector3d netherPos = netherType.scaleFromOverworld(Vector3d(800, 64, 200));

// 下界 (100, 64, 25) → 主世界 (800, 64, 200)
Vector3d overworldPos = netherType.scaleToOverworld(Vector3d(100, 64, 25));
```

### 主世界 ↔ 末地

```
无坐标缩放
末地出生点固定为 (100, 49, 0)
```

## 传送流程

### 下界传送门传送

```
实体进入传送门方块
        │
        ▼
检测传送计时器 (80 tick = 4 秒)
        │
        ├─ 计时未满 → 等待
        │
        └─ 计时已满 → 开始传送
                │
                ▼
        确定目标维度
        (主世界 ↔ 下界)
                │
                ▼
        计算目标坐标
        (主世界×8 或 下界÷8)
                │
                ▼
        搜索已存在传送门
        (半径 16 或 128)
                │
                ├─ 找到 → 使用该传送门位置
                │
                └─ 未找到 → 创建新传送门
                        │
                        ▼
                放置黑曜石框架
                        │
                        ▼
                点燃传送门方块
                        │
                        ▼
        发送维度切换包
                        │
                        ▼
        卸载旧维度区块
                        │
                        ▼
        加载新维度区块
                        │
                        ▼
        更新实体位置和维度
```

### 末地传送门传送

```
实体进入传送门方块
        │
        ▼
立即传送 (无计时)
        │
        ▼
确定目标维度
(主世界 ↔ 末地)
        │
        ├─ 主世界 → 末地
        │       │
        │       ▼
        │   目标位置固定 (100, 49, 0)
        │   创建出生平台
        │
        └─ 末地 → 主世界
                │
                ▼
        目标位置 = 重生点或床
        显示终末之诗 (首次)
```

## 与其他模块的关系

| 模块 | 关系 |
|------|------|
| `DimensionType` | 提供坐标转换比例 |
| `ServerDimensionManager` | 调用传送器进行维度切换 |
| `PortalSize` | 检测传送门框架 |
| `ServerWorld` | 放置/移除方块 |
| `Entity` | 被传送的实体 |
| `DimensionPackets` | 同步维度切换到客户端 |

## 容易踩的坑

1. **坐标取整**: 坐标转换后需要取整到合适的方块位置
2. **传送门方向**: 创建传送门时要正确设置轴向（使用 `BlockStateProperties::HORIZONTAL_AXIS`）
3. **传送冷却**: 避免传送后立即再次传送（300 tick 冷却）
4. **实体状态**: 传送后需要重置实体的某些状态（如骑乘、着火等）
5. **区块加载**: 确保目标位置的区块已加载
6. **传送门点燃时机**: 传送门在 `FireBlock::onBlockAdded()` 中点燃，而非 tick() 中（性能优化）
7. **维度检查**: 下界传送门只能在主世界和下界点燃，末地不能点燃下界传送门
8. **传送门方块计数**: `PortalSizeResult::portalBlockCount` 记录已存在的传送门方块数量，点燃前检查是否为 0
9. **玩家传送时间**: 玩家需要 80 tick (4 秒) 在传送门中才能传送，其他实体只需要 1 tick
10. **离开传送门**: 离开传送门时 `portalTime` 每tick减少 4，而非立即重置为 0

## 相关类和文件

### Entity 传送门系统 (common/entity/core/Entity.hpp/cpp)

实体基类提供传送门计时和传送核心逻辑：

| 方法 | 描述 |
|------|------|
| `portalCooldown()` | 获取传送冷却时间（tick） |
| `setPortalCooldown()` | 设置传送冷却时间 |
| `canTeleport()` | 检查是否可以传送（冷却是否完成） |
| `portalTime()` | 获取在传送门中的累计时间 |
| `setPortalTime()` | 设置传送门累计时间 |
| `resetPortalTime()` | 重置传送门计时为 0 |
| `isInPortal()` | 检查是否在传送门中 |
| `setInPortal(bool)` | 设置是否在传送门中 |
| `getMaxInPortalTime()` | 获取传送所需最大时间（玩家 80 tick，其他实体 1 tick） |
| `tickPortal()` | 每帧调用，更新传送门计时，返回 true 时触发传送 |
| `onPortalTriggered()` | 传送触发回调，子类重写以实现维度切换 |
| `triggerPortalCooldown()` | 触发传送冷却（300 tick） |
| `portalPos()` | 获取传送门方块位置 |
| `setPortalPos()` | 设置传送门方块位置 |

### ServerPlayer 维度切换 (server/player/ServerPlayer.hpp/cpp)

服务端玩家重写 `onPortalTriggered()` 实现维度切换：

| 方法 | 描述 |
|------|------|
| `onPortalTriggered()` | 重写自 Entity，处理传送门触发 |
| `changeDimension(targetDim)` | 执行维度切换 |

**维度切换流程**：
1. 检查骑乘状态并解除
2. 计算目标位置（坐标缩放）
3. 重置传送门状态和触发冷却
4. 调用 `ServerDimensionManager::transferPlayerToDimension()`
5. 更新实体位置和维度属性

### ServerDimensionManager (server/dimension/ServerDimensionManager.hpp/cpp)

服务端维度管理器提供维度切换：

| 方法 | 描述 |
|------|------|
| `transferPlayerToDimension(playerId, targetDim, position)` | 将玩家传送到另一个维度 |
| `playerJoinDimension(playerId, dimId)` | 玩家加入维度 |
| `playerLeaveDimension(playerId)` | 玩家离开维度 |
| `getPlayerDimension(playerId)` | 获取玩家当前维度 |
| `sendDimensionChangePacket(playerId, newDim, pos)` | 发送维度切换数据包 |

## 测试用例

测试文件位置: `tests/common/test_entity.cpp`

| 测试用例 | 说明 |
|----------|------|
| `Entity.PortalCooldown` | 传送冷却测试 |
| `Entity.PortalTime` | 传送门时间测试 |
| `Entity.GetMaxInPortalTime` | 玩家/非玩家传送时间差异测试 |
| `Entity.TickPortalNotInPortal` | 离开传送门时时间递减测试 |
| `Entity.TickPortalInPortal` | 进入传送门触发测试 |
| `Entity.TickPortalInPortalWithCooldown` | 冷却阻止传送测试 |
| `Entity.TickPortalPlayer` | 玩家需要 80 tick 测试 |
| `Entity.TickPortalPlayerInterrupted` | 中断后时间递减测试 |
| `Entity.PortalPos` | 传送门位置记录测试 |
| `Entity.TickPortalCooldownDecrement` | 冷却递减测试 |
| `Entity.OnPortalTriggered` | 触发回调测试 |
