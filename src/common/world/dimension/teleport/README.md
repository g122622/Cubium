# Teleport 传送系统

处理维度间传送，包括下界传送门和末地传送门。

## 目录结构

```
teleport/
├── PortalSize.hpp       # 传送门框架尺寸检测工具
├── PortalSize.cpp       # 传送门尺寸检测实现
├── Teleporter.hpp       # 传送器基类及下界/末地传送器子类
├── Teleporter.cpp       # 传送器实现
└── README.md            # 本文档
```

## 内部模块关系

```
PortalSize ←─── NetherTeleporter
     │               │
     │               ↓
     │         Teleporter (基类)
     │               ↑
     │               │
     └─────── EndTeleporter
```

- **PortalSize**: 纯静态工具类，检测黑曜石框架尺寸和点燃传送门
- **Teleporter**: 抽象基类，定义传送接口和坐标转换
- **NetherTeleporter**: 主世界 ↔ 下界传送，支持坐标缩放和传送门搜索/创建
- **EndTeleporter**: 主世界 ↔ 末地传送，固定出生点 (100.5, 50.0, 0.5)

EndTeleporter 提供以下功能：
- `createEndSpawnPlatform()`: 创建末地出生黑曜石平台（5×5，Y=48），清空上方 4 层空间
- `createExitPortal(pos, active)`: 创建末地出口传送门讲台（基岩柱 + 传送门环 + 火把），active 控制是否放置传送门方块
- `placeEndPortalFrame(center)`: 放置 12 个末影之眼框架 + 3×3 传送门方块（用于要塞传送门房间）

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 外部模块 | 用途 |
|----------|------|
| `common/core/Types.hpp` | 基础类型定义 |
| `common/world/IWorld.hpp` | 世界接口，用于方块操作和区块访问 |
| `common/world/block/BlockPos.hpp` | 方块位置类型 |
| `common/util/Direction.hpp` | 方向和轴向枚举 |
| `common/util/math/random/Random.hpp` | 随机数生成（传送门轴向选择） |
| `dimension/DimensionType.hpp` | 维度类型，提供坐标转换方法 |
| `dimension/DimensionManager.hpp` | 维度管理器，获取维度实例 |

### 依赖本模块的外部模块

| 外部模块 | 用途 |
|----------|------|
| `entity/core/Entity.hpp` | 实体传送门计时和触发逻辑 |
| `server/player/ServerPlayer.hpp` | 服务端玩家维度切换实现 |
| `server/dimension/ServerDimensionManager.hpp` | 服务端维度管理，调用传送器 |
| `block/FireBlock.hpp` | 火焰方块点燃时调用 `PortalSize::findNetherPortal()` |

## 容易踩的坑

### 1. 传送门搜索半径不对称

| 方向 | 搜索半径 | 原因 |
|------|----------|------|
| 主世界 → 下界 | 128 格 | 主世界坐标 ÷ 8 = 下界坐标，范围缩小 |
| 下界 → 主世界 | 16 格 | 下界坐标 × 8 = 主世界坐标，范围扩大 |

代码中常量：`OVERWORLD_TO_NETHER_SEARCH_RADIUS = 128`，`NETHER_TO_OVERWORLD_SEARCH_RADIUS = 16`。

### 2. 坐标取整问题

坐标转换后需取整到合适的方块位置，使用 `math::floorTo<BlockCoord>()` 而非直接强制转换。

### 3. 传送门轴向设置

创建传送门时要正确设置轴向，使用 `BlockStateProperties::HORIZONTAL_AXIS()`，而非 `HORIZONTAL_FACING`。

### 4. 传送冷却时间

| 实体类型 | 传送冷却 |
|----------|----------|
| 玩家 | 10 ticks |
| 其他实体 | 300 ticks (15秒) |

### 5. 传送门时间

| 实体类型 | 所需时间 | 说明 |
|----------|----------|------|
| 玩家 | 80 ticks (4秒) | 创造模式仅 1 tick |
| 其他实体 | 1 tick | 默认值 |

离开传送门时 `portalTime` 每tick减少 4，而非立即重置为 0。

### 6. 传送门点燃时机

传送门在 `FireBlock::onBlockAdded()` 中点燃，而非 `tick()` 中（性能优化）。

### 7. 维度检查

下界传送门只能在主世界和下界点燃，末地不能点燃下界传送门。

### 8. portalBlockCount 检查

`PortalSizeResult::portalBlockCount` 记录已存在的传送门方块数量。点燃前检查是否为 0，避免重复点燃。

### 9. 末地出生点

末地出生位置固定为 `(100.5, 50.0, 0.5)`（方块中心），平台生成在 Y=48。

### 10. CHUNK_HEIGHT 与 MAX_BUILD_HEIGHT

两者值不同（384 vs 320）且语义不同。`CHUNK_HEIGHT = MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT`。搜索传送门方块时使用 `world::MIN_BUILD_HEIGHT` 和 `world::MAX_BUILD_HEIGHT` 作为高度范围。
