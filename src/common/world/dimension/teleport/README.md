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

**职责**: 检测传送门框架尺寸。

**主要方法**:
- `findNetherPortal(world, pos)` - 寻找下界传送门框架
- `lightNetherPortal(world, portal)` - 点燃下界传送门
- `findEndPortal(world, pos)` - 寻找末地传送门框架
- `activateEndPortal(world, portal)` - 激活末地传送门

**下界传送门规则**:
- 框架由黑曜石构成
- 最小尺寸: 2x3
- 最大尺寸: 21x21
- 内部必须为空气

**使用示例**:
```cpp
// 寻找传送门框架
auto result = PortalSize::findNetherPortal(world, pos);
if (result.has_value() && result->valid) {
    // 点燃传送门
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
    virtual bool teleport(Entity& entity, DimensionId targetDim) = 0;
    virtual std::optional<PortalInfo> findPortal(ServerWorld& world, const Vector3d& pos) = 0;
    virtual PortalInfo createPortal(ServerWorld& world, const Vector3d& pos) = 0;
    virtual f32 getCoordinateScale() const;
    static Vector3d transformPosition(const Vector3d& pos, const DimensionType& from, const DimensionType& to);
};
```

**NetherTeleporter**:
- 坐标转换: 主世界 ↔ 下界 1:8
- 传送门搜索半径: 主世界 128 格，下界 16 格
- 自动创建黑曜石框架

**EndTeleporter**:
- 固定出生位置: (100, 49, 0)
- 无坐标缩放
- 自动创建黑曜石平台

**使用示例**:
```cpp
// 下界传送
NetherTeleporter teleporter;
bool success = teleporter.teleport(entity, DimensionManager::NETHER);

// 末地传送
EndTeleporter endTeleporter;
endTeleporter.teleport(entity, DimensionManager::THE_END);
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
2. **传送门方向**: 创建传送门时要正确设置轴向
3. **传送冷却**: 避免传送后立即再次传送
4. **实体状态**: 传送后需要重置实体的某些状态（如骑乘、着火等）
5. **区块加载**: 确保目标位置的区块已加载

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
