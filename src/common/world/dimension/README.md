# Dimension 模块

维度系统核心模块，包含维度类型定义、维度实例、维度管理器和渲染参数等功能。

## 目录结构

```
dimension/
├── DimensionType.hpp            # 维度类型定义（坐标缩放、环境特性、高度限制等固有属性）
├── DimensionType.cpp            # 维度类型实现
├── Dimension.hpp                # 维度实例类（组合维度类型与区块生成器）
├── Dimension.cpp                # 维度实例实现
├── DimensionManager.hpp         # 维度管理器（维度实例注册表）
├── DimensionManager.cpp         # 维度管理器实现
├── DimensionRenderSettings.hpp  # 维度渲染设置（云高度、雾类型、天空、天花板等）
├── MapDimensionId.hpp           # 维度ID映射工具（MapDimensionId枚举 + dimensionIdToString/fromString/NameToId）
├── teleport/                    # 传送系统
│   ├── PortalSize.hpp           # 传送门尺寸检测
│   ├── PortalSize.cpp           # 传送门尺寸检测实现
│   ├── Teleporter.hpp           # 传送器基类和子类
│   ├── Teleporter.cpp           # 传送器实现
│   └── README.md                # 传送系统文档
├── end/                         # 末地维度战斗系统
│   ├── EndDragonFight.hpp/cpp   # 末影龙战斗管理器（击杀奖励、折跃门、龙蛋）
│   └── README.md                # 末地战斗系统文档
└── README.md                    # 本文档
```

## 内部模块关系

```
DimensionType ←─── Dimension ←─── DimensionManager
     │                 │                  │
     │                 ↓                  ↓
     │          IChunkGenerator      ServerWorld/MinecraftServer
     │
     ↓
DimensionRenderSettings ──→ CloudRenderer / 天空渲染器 / 雾渲染器
```

- **DimensionType**: 定义维度的固有属性（坐标缩放、环境特性等），是值类型
- **Dimension**: 组合维度类型与区块生成器，代表一个具体的维度实例
- **DimensionManager**: 管理所有维度实例的注册表，提供维度访问和遍历接口
- **DimensionRenderSettings**: 渲染参数，供客户端渲染器使用

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 外部模块 | 用途 |
|----------|------|
| `common/core/Types.hpp` | 基础类型定义（`f32`, `u8`, `bool` 等） |
| `world/chunk/` | 区块生成器接口（`IChunkGenerator`） |
| `world/gen/settings/` | 维度生成参数（`DimensionSettings`，注意与本模块的 `DimensionRenderSettings` 区分） |

### 依赖本模块的外部模块

| 外部模块 | 用途 |
|----------|------|
| `server/core/MinecraftServer` | 维度管理器初始化和维度访问 |
| `server/world/ServerWorld` | 服务端世界维度操作 |
| `client/renderer/` | 客户端渲染器使用 `DimensionRenderSettings` |
| `entity/` | 实体传送、维度切换 |

## 容易踩的坑

### 1. 维度ID与 MC 1.16.5 保持一致

| 维度 | ID | 说明 |
|------|-----|------|
| 主世界 | 0 | `DimensionManager::OVERWORLD` |
| 下界 | -1 | `DimensionManager::NETHER`（注意是 -1，不是 1） |
| 末地 | 1 | `DimensionManager::THE_END` |

这对存档兼容性至关重要。

### 2. -ffast-math 与 NaN 检测

项目使用 `-ffast-math` 编译选项，会破坏 IEEE 754 NaN 语义：
```cpp
// 错误！NaN 检测在 -ffast-math 下不可靠
if (!std::isnan(settings.cloudHeight)) { ... }

// 正确！使用显式的 hasClouds 布尔字段
if (settings.hasClouds) { ... }
```

### 3. DimensionRenderSettings 与 DimensionSettings 混淆

| 模块 | 位置 | 用途 |
|------|------|------|
| **DimensionRenderSettings** | `world/dimension/` | 渲染参数（云高度、雾类型等） |
| **DimensionSettings** | `world/gen/settings/` | 生成参数（噪声设置、默认方块、海平面等） |

### 4. CHUNK_HEIGHT 与 MAX_BUILD_HEIGHT 的区别

两者值不同且语义不同：
- `MAX_BUILD_HEIGHT`: 世界最大建筑高度（320）
- `MIN_BUILD_HEIGHT`: 世界最低建筑高度（-64）
- `CHUNK_HEIGHT`: 区块高度 = `MAX_BUILD_HEIGHT - MIN_BUILD_HEIGHT` = 384

### 5. 传送门触发时序（MC 1.16.5）

| 实体类型 | 传送时间 | 说明 |
|----------|----------|------|
| 玩家 | 80 ticks (4秒) | 创造模式下仅 1 tick |
| 其他实体 | 1 tick | 默认值 |

### 6. 传送冷却

- **玩家**: 10 ticks
- **其他实体**: 300 ticks (15秒)

### 7. 坐标转换比例

| 转换方向 | 缩放比例 |
|----------|----------|
| 主世界 → 下界 | 坐标 ÷ 8 |
| 下界 → 主世界 | 坐标 × 8 |
| 末地 → 主世界 | 固定出生点 (100, 49, 0) |

### 8. 传送门点燃时机

传送门点燃在 `FireBlock::onBlockAdded()` 中处理，而非 `tick()` 中，避免每 tick 检测以提高性能。

### 9. 传送门尺寸限制

- 最小宽度: 2 格，最大宽度: 21 格
- 最小高度: 3 格，最大高度: 21 格
