# Blend 模块 - 生物群系颜色混合

本目录实现生物群系边界处的颜色平滑过渡，参考 MC 1.16.5 的 `ClientWorld.getBlockColorRaw()` 实现。

## 目录结构

```
blend/
├── blend.hpp               # 统一头文件
├── BiomeColorCache.hpp     # 颜色缓存（按区块缓存计算结果）
├── BiomeColorCache.cpp     # 缓存实现
├── BiomeColorBlender.hpp   # 颜色混合器（核心算法）
├── BiomeColorBlender.cpp   # 混合器实现
├── ChunkBiomeAccessor.hpp  # 区块生物群系访问器（支持邻居区块查询）
├── ChunkBiomeAccessor.cpp  # 访问器实现
└── README.md               # 本文档
```

## 内部模块关系

```
ChunkMesher 调用
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
```

**核心流程**：
1. `ChunkMesher` 创建 `ChunkBiomeAccessor` 封装当前区块和邻居区块
2. `BiomeColorBlender.getBlendedColorCached()` 首先检查缓存
3. 缓存未命中时，在 (2r+1)×(2r+1) 区域内采样生物群系颜色
4. 对 RGB 分量分别求平均，实现平滑过渡

## 上下游外部依赖

### 上游依赖（本模块依赖的模块）

```
common/world/biome/Biome.hpp           # 生物群系定义
common/world/biome/BiomeRegistry.hpp   # 生物群系注册表
common/world/chunk/ChunkData.hpp       # 区块数据（生物群系存储）
common/world/WorldConstants.hpp        # 区块常量（CHUNK_SHIFT, CHUNK_MASK）
common/core/Constants.hpp              # MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT
client/world/color/ColorResolver.hpp   # 颜色解析器接口
client/world/color/BiomeColors.hpp     # 颜色解析器实现（草、树叶、水）
```

### 下游依赖（依赖本模块的模块）

```
client/renderer/trident/chunk/ChunkMesher.hpp   # 区块网格生成（主要使用者）
client/renderer/trident/chunk/ChunkMesher.cpp   # 通过 resolveTintColorBlended 获取混合颜色
```

## 容易踩的坑

### 1. 邻居区块缺失

混合采样跨越区块边界时，邻居区块可能未加载。`ChunkBiomeAccessor::getBiome()` 会返回 `nullptr`，混合算法会自动跳过该位置。

### 2. 缓存一致性与混合半径

混合半径变化后，缓存结果不正确。`setBlendRadius()` 会自动清空缓存，但如果外部缓存了旧值需要手动清理。

### 3. 对角线区块访问

当前实现仅支持东、西、南、北四个邻居区块，不支持访问对角线方向的区块。混合半径最大为 7，但区块访问仅限四个正交方向，对于半径 ≤ 7 的混合通常够用。

### 4. 缓存失效时机

区块卸载时必须调用 `invalidateChunk()`，否则缓存会持有无效引用。同时，邻居区块的边缘缓存也需要失效（当前实现已处理）。

### 5. ColorResolver 返回值

`ColorResolver.getColor()` 可能返回 `0xFFFFFFFF` 表示需要从 colormap 获取颜色。`BiomeColorBlender` 内部会处理这种情况。

### 6. Colormap 设置

`BiomeColorBlender` 需要外部设置 `grassColorMap` 和 `foliageColorMap` 指针（由 ChunkMesher 管理），水颜色不需要 colormap。
