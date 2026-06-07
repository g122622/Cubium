# Color 模块

本目录包含客户端颜色解析系统，负责从生物群系获取各种颜色值（草、树叶、水体等）以及生物群系边界处的颜色平滑过渡。

## 目录结构

```
color/
├── ColorResolver.hpp       # 颜色解析器抽象接口
├── BiomeColors.hpp         # 颜色解析器实现（草、树叶、水）和常量
├── BiomeColors.cpp         # 实现文件
├── blend/                  # 生物群系颜色混合模块
│   ├── blend.hpp           # 统一头文件
│   ├── BiomeColorCache.hpp # 颜色缓存（按区块缓存计算结果）
│   ├── BiomeColorCache.cpp
│   ├── BiomeColorBlender.hpp # 颜色混合器（核心算法）
│   ├── BiomeColorBlender.cpp
│   ├── ChunkBiomeAccessor.hpp # 区块生物群系访问器（支持邻居区块查询）
│   ├── ChunkBiomeAccessor.cpp
│   └── README.md           # blend 模块文档
└── README.md               # 本文档
```

## 内部模块关系

```
ChunkMesher 调用
       │
       ▼
resolveTintColorBlended()
       │
       ▼
BiomeColorBlender ─────────┬─────────────┐
       │                    │             │
       │ 使用               │ 查询        │ 缓存
       ▼                    ▼             ▼
ChunkBiomeAccessor     BiomeRegistry   BiomeColorCache
       │                    │
       │ 读取               │ 返回
       ▼                    ▼
ChunkData + 邻居区块     Biome
                           │
                           ▼
                    ColorResolver (上层提供)
                           │
                   ┌───────┼───────┐
                   │       │       │
            GrassResolver FoliageResolver WaterResolver
```

**核心流程**：
1. `ColorResolver.getColor()` 根据生物群系返回颜色，返回 `0xFFFFFFFF` 表示需要从 colormap 获取
2. `BiomeColorBlender` 在 (2r+1)×(2r+1) 区域内采样并平均颜色，实现平滑过渡
3. `BiomeColorCache` 按区块缓存计算结果，避免重复计算

## 上下游外部依赖

### 上游依赖（本模块依赖的模块）

```
common/world/biome/Biome.hpp          # 生物群系定义
common/world/biome/BiomeEffects.hpp   # 视觉效果（颜色常量）
common/world/chunk/ChunkData.hpp      # 区块数据（用于 ChunkBiomeAccessor）
common/util/math/random/Random.hpp    # 随机数（噪声计算）
```

### 下游依赖（依赖本模块的模块）

```
client/renderer/trident/chunk/ChunkMesher.cpp  # 区块网格生成（主要使用者）
client/world/ClientWorld.cpp                    # 世界颜色查询
```

## 容易踩的坑

### 1. 颜色格式混乱

`BiomeEffects` 中存储的颜色是 RGB 格式（无 alpha），渲染时需添加 alpha 通道：`color | 0xFF000000`。

### 2. 沼泽颜色噪声

沼泽颜色使用噪声实现双色混合，MC 使用 `PerlinNoiseGenerator.getValue(x * 0.0225, z * 0.0225)`，阈值是 `-0.1`。本项目的 `BiomeColors::calculateSwampColor()` 已实现。

### 3. ColorResolver 返回值

`GrassColorResolver` 和 `FoliageColorResolver` 可能返回 `0xFFFFFFFF`，调用方检测此值后需从 colormap 计算（基于温度和湿度索引）。`WaterColorResolver` 直接返回颜色值，不需要 colormap。

### 4. 位置参数用途

`ColorResolver.getColor(biome, x, z)` 的位置参数主要用于沼泽的双色噪声混合，其他生物群系实现可忽略位置参数。

### 5. 混合半径与性能

MC 默认使用半径 2（5x5 混合区域），半径太大导致性能问题，太小导致颜色突变。用户可在设置中调整（0-7），半径 0 表示禁用混合。

### 6. 缓存一致性与区块卸载

区块卸载时必须调用 `BiomeColorCache::invalidateChunk()`，否则缓存会持有无效引用。混合半径变化后缓存结果也会不正确，`setBlendRadius()` 会自动清空缓存。

### 7. 邻居区块访问限制

`ChunkBiomeAccessor` 仅支持东、西、南、北四个邻居区块，不支持对角线方向。混合半径最大为 7，对于区块访问通常够用。

### 8. Colormap 设置

`BiomeColorBlender` 需要外部设置 `grassColorMap` 和 `foliageColorMap` 指针（由 ChunkMesher 管理），水颜色不需要 colormap。
