# Dimension 模块

维度系统核心模块，包含维度类型定义、维度实例、维度管理器和渲染参数等功能。

## 目录结构

```
dimension/
├── DimensionType.hpp            # 维度类型定义
├── DimensionType.cpp            # 维度类型实现
├── Dimension.hpp                # 维度实例类
├── Dimension.cpp                # 维度实例实现
├── DimensionManager.hpp         # 维度管理器
├── DimensionManager.cpp         # 维度管理器实现
├── DimensionRenderSettings.hpp  # 维度渲染设置
└── README.md                    # 本文档
```

## 文件详解

### DimensionType.hpp/cpp

**职责**: 定义维度类型的固有属性，如坐标缩放、环境特性、高度限制等。

参考 MC 1.16.5 DimensionType。

**主要属性**:

| 属性 | 类型 | 说明 |
|------|------|------|
| `m_id` | `DimensionId` | 维度ID |
| `m_name` | `String` | 维度名称 |
| `m_hasCeiling` | `bool` | 是否有天花板（下界） |
| `m_hasSkyLight` | `bool` | 是否有天空光照 |
| `m_ultraWarm` | `bool` | 是否超热（水蒸发） |
| `m_natural` | `bool` | 是否自然维度 |
| `m_bedWorks` | `bool` | 床是否可用 |
| `m_respawnAnchorWorks` | `bool` | 重生锚是否可用 |
| `m_coordinateScale` | `f32` | 坐标缩放比例（下界=8） |
| `m_minHeight` | `i32` | 最低建筑高度 |
| `m_maxHeight` | `i32` | 最高建筑高度 |
| `m_logicalHeight` | `i32` | 逻辑高度上限 |
| `m_ambientLight` | `f32` | 环境光照强度 |
| `m_fixedTime` | `std::optional<i64>` | 固定时间值 |

**静态工厂方法**:
- `overworld()` - 主世界类型
- `nether()` - 下界类型
- `theEnd()` - 末地类型

**坐标转换方法**:
- `scaleFromOverworld(pos)` - 从主世界坐标转换
- `scaleToOverworld(pos)` - 转换到主世界坐标
- `transformPosition(pos, from, to)` - 通用坐标转换

**使用示例**:
```cpp
// 获取下界维度类型
auto nether = DimensionType::nether();

// 检查属性
if (nether.ultraWarm()) {
    // 水会蒸发
}

// 坐标转换
Vector3d netherPos(800, 64, 200);
Vector3d overworldPos = nether.scaleToOverworld(netherPos);
// overworldPos = (6400, 64, 1600)

// 检查床是否可用
if (!nether.bedWorks()) {
    // 床会爆炸
}
```

### Dimension.hpp/cpp

**职责**: 维度实例类，组合维度类型、区块生成器和生物群系提供者。

**主要成员**:

| 成员 | 类型 | 说明 |
|------|------|------|
| `m_id` | `DimensionId` | 维度ID |
| `m_type` | `DimensionType` | 维度类型 |
| `m_generator` | `IChunkGenerator*` | 区块生成器 |
| `m_biomeProvider` | `BiomeProvider*` | 生物群系提供者 |
| `m_spawnPoint` | `Vector3d` | 出生点位置 |

**工厂方法**:
- `createOverworld(seed)` - 创建主世界维度
- `createNether(seed)` - 创建下界维度
- `createTheEnd(seed)` - 创建末地维度

**使用示例**:
```cpp
auto overworld = Dimension::createOverworld(seed);
auto biome = overworld->biomeProvider()->getBiome(x, y, z);
auto spawnPoint = overworld->spawnPoint();
```

### DimensionManager.hpp/cpp

**职责**: 维度管理器，管理所有维度实例的注册表。

**维度ID常量**:
- `OVERWORLD = 0` - 主世界
- `NETHER = 1` - 下界
- `THE_END = 2` - 末地

**主要方法**:
- `initialize(seed)` - 初始化维度管理器
- `shutdown()` - 关闭维度管理器
- `getDimension(id)` - 获取维度
- `getOverworld()` - 获取主世界
- `getNether()` - 获取下界
- `getTheEnd()` - 获取末地
- `forEachDimension(func)` - 遍历所有维度

**使用示例**:
```cpp
DimensionManager manager;
manager.initialize(seed);

// 访问维度
Dimension* overworld = manager.getDimension(DimensionManager::OVERWORLD);

// 遍历所有维度
manager.forEachDimension([](Dimension& dim) {
    dim.tick();
});
```

### DimensionRenderSettings.hpp

**职责**: 定义维度的渲染相关参数，包括云高度、天空、天花板、雾类型等设置。

**主要内容**:

#### FogType 枚举
定义雾类型，参考 MC 1.16.5 DimensionRenderInfo.FogType：
- `None` (0): 无雾
- `Normal` (1): 普通雾
- `End` (2): 末地雾

#### DimensionRenderSettings 结构体
定义维度渲染参数：

| 字段 | 类型 | 说明 |
|------|------|------|
| `cloudHeight` | `f32` | 云高度（NaN 表示无云） |
| `hasSky` | `bool` | 是否有天空 |
| `hasCeiling` | `bool` | 是否有天花板（下界为 true） |
| `fogType` | `FogType` | 雾类型 |
| `hasNaturalLight` | `bool` | 是否有自然光照 |
| `name` | `const char*` | 维度名称（调试用） |

**静态工厂方法**:
- `overworld()` - 主世界设置（云高度 192，有天空，普通雾）
- `nether()` - 下界设置（无云，有天花板，无自然光）
- `end()` - 末地设置（无云，末地雾）
- `getDefault()` - 默认设置（返回主世界）

**成员方法**:
- `hasClouds()` - 检查该维度是否有云（通过判断 cloudHeight 是否为 NaN）

## 文件关系图

```
                    ┌──────────────────────────────────┐
                    │     DimensionRenderSettings.hpp  │
                    │         (维度渲染设置)            │
                    └──────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    │               │               │
                    ▼               ▼               ▼
           ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐
           │ CloudRenderer│  │  其他渲染器  │  │  测试用例        │
           │ (客户端渲染)  │  │ (天空/雾等)  │  │ test_cloud_     │
           └─────────────┘  └─────────────┘  │ renderer.cpp    │
                                               └─────────────────┘
```

## 与相关模块的关系

### 与 DimensionSettings.hpp 的区别

| 模块 | 位置 | 用途 |
|------|------|------|
| **DimensionRenderSettings** | `world/dimension/` | 渲染参数（云高度、雾类型等） |
| **DimensionSettings** | `world/gen/settings/` | 生成参数（噪声设置、默认方块、海平面等） |

- `DimensionSettings` 用于世界生成阶段
- `DimensionRenderSettings` 用于客户端渲染阶段

## 模块概述

### 整体职责

定义各维度（主世界、下界、末地）的渲染相关参数，供客户端渲染器使用。

### 输入

- 无运行时输入，所有设置通过静态工厂方法预定义

### 输出

- `DimensionRenderSettings` 结构体实例，包含维度的渲染参数

### 依赖项

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（`f32`, `u8`, `bool` 等） |
| `<cmath>` | `std::isnan()` 用于检测 NaN |
| `<limits>` | `std::numeric_limits<f32>::quiet_NaN()` |

### 使用方法

```cpp
#include "common/world/dimension/DimensionRenderSettings.hpp"

// 获取主世界渲染设置
auto settings = DimensionRenderSettings::overworld();

// 检查是否有云
if (settings.hasClouds()) {
    // 在 settings.cloudHeight 高度渲染云
    renderClouds(settings.cloudHeight);
}

// 检查是否有天空
if (settings.hasSky) {
    renderSky();
}

// 根据雾类型设置渲染
switch (settings.fogType) {
    case FogType::None:
        disableFog();
        break;
    case FogType::Normal:
        enableNormalFog();
        break;
    case FogType::End:
        enableEndFog();
        break;
}

// 检查自然光照
if (settings.hasNaturalLight) {
    updateSkyLight();
}
```

### 容易踩的坑

1. **NaN 检查**: `cloudHeight` 使用 NaN 表示无云，必须使用 `std::isnan()` 检查，不能直接比较
   ```cpp
   // 错误！NaN 不等于自身
   if (settings.cloudHeight == settings.cloudHeight) { ... }

   // 正确
   if (!std::isnan(settings.cloudHeight)) { ... }
   // 或使用 hasClouds() 方法
   if (settings.hasClouds()) { ... }
   ```

2. **与 DimensionSettings 混淆**: 注意区分 `DimensionRenderSettings`（渲染）和 `DimensionSettings`（生成）

3. **指针类型**: `name` 字段是 `const char*`，指向静态字符串常量，不要尝试修改或释放

4. **预设值不可变**: 工厂方法返回的是临时对象，如果需要持久化请复制
   ```cpp
   // 每次调用都创建新对象
   auto settings1 = DimensionRenderSettings::overworld();
   auto settings2 = DimensionRenderSettings::overworld();
   // settings1 和 settings2 是独立的副本
   ```

## 涉及的测试用例

测试文件位置: `tests/client/renderer/test_cloud_renderer.cpp`

| 测试用例 | 说明 |
|----------|------|
| `DimensionRenderSettingsTest.OverworldSettings` | 测试主世界设置（云高度、天空、雾类型等） |
| `DimensionRenderSettingsTest.NetherSettings` | 测试下界设置（无云、有天花板、无自然光） |
| `DimensionRenderSettingsTest.EndSettings` | 测试末地设置（无云、末地雾） |
| `DimensionRenderSettingsTest.DefaultSettings` | 测试 `getDefault()` 返回主世界设置 |
| `DimensionRenderSettingsTest.HasCloudsMethod` | 测试 `hasClouds()` 方法正确性 |
| `DimensionRenderSettingsTest.FogTypeEnumValues` | 测试雾类型枚举值正确 |

## 传送系统

### 传送门触发时序

实体通过传送门传送的时序遵循 MC 1.16.5 规则：

| 实体类型 | 传送时间 | 说明 |
|----------|----------|------|
| 玩家 | 80 ticks (4秒) | `Player::getMaxInPortalTime()` 返回 80 |
| 其他实体 | 1 tick | `Entity::getMaxInPortalTime()` 默认返回 1 |

### 传送冷却

传送后有 300 ticks (15秒) 的冷却时间，期间无法再次传送：
- `Entity::getPortalCooldown()` 返回 300
- 冷却期间 `canTeleport()` 返回 false

### 坐标转换

使用 `Teleporter` 类进行坐标转换：

| 转换方向 | 缩放比例 |
|----------|----------|
| 主世界 → 下界 | 坐标 ÷ 8 |
| 下界 → 主世界 | 坐标 × 8 |
| 末地 → 主世界 | 固定出生点 (100, 49, 0) |

```cpp
// 使用 Teleporter 进行坐标转换
Vector3d targetPos = Teleporter::transformPosition(
    currentPos,
    DimensionType::fromId(currentDim),
    DimensionType::fromId(targetDim));

// 获取末地出生点
Vector3d endSpawn = Teleporter::getEndSpawnPosition(); // (100, 49, 0)
```

### 服务端维度切换

`ServerPlayer::changeDimension()` 负责处理玩家的维度切换：

```cpp
bool ServerPlayer::changeDimension(DimensionId targetDim) {
    // 1. 检查骑乘状态，下骑乘
    // 2. 计算目标坐标
    // 3. 重置传送门状态和触发冷却
    // 4. 调用 ServerDimensionManager::transferPlayerToDimension()
    // 5. 更新实体维度属性和位置
}
```

## 未来扩展

当前维度系统已实现核心框架，后续计划：

- 完善传送门搜索算法（寻找或创建目标维度的传送门）
- 生物群系提供者目录隔离（provider/overworld, provider/nether, provider/end）
- 专用区块生成器（NetherChunkGenerator, EndChunkGenerator）
- 服务端维度管理集成到 MinecraftServer
