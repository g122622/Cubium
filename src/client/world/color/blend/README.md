# Blend 模块 - 生物群系颜色混合

本目录实现生物群系边界处的颜色平滑过渡，参考 MC 1.16.5 的 `ClientWorld.getBlockColorRaw()` 实现。

## 目录结构

```
blend/
├── blend.hpp               # 统一头文件
├── BiomeColorCache.hpp     # 颜色缓存声明
├── BiomeColorCache.cpp     # 颜色缓存实现
├── BiomeColorBlender.hpp   # 颜色混合器声明
├── BiomeColorBlender.cpp   # 颜色混合器实现
├── ChunkBiomeAccessor.hpp  # 区块生物群系访问器声明
├── ChunkBiomeAccessor.cpp  # 区块生物群系访问器实现
└── README.md               # 本文档
```

## 架构设计

### 类图

```
┌─────────────────────────────────────────────────────────────────────┐
│                      BiomeColorBlender                               │
│  ─────────────────────────────────────────────────────────────────  │
│  - m_blendRadius: i32                                                │
│  - m_cacheEnabled: bool                                              │
│  - m_cache: BiomeColorCache                                          │
│  - m_colorBuffer: vector<u32>                                        │
│  ─────────────────────────────────────────────────────────────────  │
│  + setBlendRadius(radius: i32)                                       │
│  + getBlendedColor(accessor, x, y, z, resolver, resolverId): u32    │
│  + getBlendedColorCached(...): u32                                   │
│  + invalidateChunk(chunkX, chunkZ)                                   │
│  + clearCache()                                                      │
│  + averageColors(colors, count): u32 [static]                        │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                │ uses
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       BiomeColorCache                                │
│  ─────────────────────────────────────────────────────────────────  │
│  - m_entries: map<u64, BiomeColorCacheEntry>                         │
│  - m_mutex: mutex                                                    │
│  - m_cacheHits: size_t                                               │
│  - m_cacheMisses: size_t                                             │
│  ─────────────────────────────────────────────────────────────────  │
│  + getOrCompute<ComputeFunc>(chunkX, chunkZ, localX, localZ,        │
│                               resolverId, compute): u32              │
│  + invalidateChunk(chunkX, chunkZ)                                   │
│  + invalidatePosition(x, z)                                          │
│  + clear()                                                           │
│  + getStats(): Stats                                                 │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                     IBiomeAccessor (interface)                       │
│  ─────────────────────────────────────────────────────────────────  │
│  + getBiome(x, y, z): const Biome*                                   │
│  + isChunkLoaded(x, z): bool                                         │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                │ implements
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     ChunkBiomeAccessor                               │
│  ─────────────────────────────────────────────────────────────────  │
│  - m_chunk: const ChunkData&                                         │
│  - m_neighbors: array<const ChunkData*, 4>                          │
│  - m_chunkX, m_chunkZ: ChunkCoord                                    │
│  ─────────────────────────────────────────────────────────────────  │
│  + getBiome(x, y, z): const Biome*                                   │
│  + getBiomeLocal(localX, y, localZ): const Biome*                   │
│  + isChunkLoaded(x, z): bool                                         │
└─────────────────────────────────────────────────────────────────────┘
```

### 工作流程

```
                    getBlendedColorCached()
                            │
                            ▼
                ┌─── 检查缓存 (m_cache) ───┐
                │                           │
          命中  │                     未命中 │
                ▼                           ▼
           返回缓存值               getBlendedColor()
                                            │
                            ┌───────────────┴───────────────┐
                            │                               │
                     blendRadius == 0               blendRadius > 0
                            │                               │
                            ▼                               ▼
                     getColorDirect()              getColorBlended()
                            │                               │
                            │                      采样周围生物群系
                            │                      对 RGB 分量求平均
                            │                               │
                            └───────────────┬───────────────┘
                                            │
                                            ▼
                                    存入缓存并返回
```

## 核心组件

### BiomeColorBlender

**职责**：实现生物群系颜色混合算法。

**关键参数**：

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `blendRadius` | i32 | 2 | 混合半径，采样区域为 (2r+1)×(2r+1) |
| `cacheEnabled` | bool | true | 是否启用缓存 |

**混合算法**：

```cpp
// 对于半径 r，采样 (2r+1) × (2r+1) 区域
int count = 0;
int r = 0, g = 0, b = 0;

for (int dx = -radius; dx <= radius; dx++) {
    for (int dz = -radius; dz <= radius; dz++) {
        Biome* biome = getBiome(x + dx, y, z + dz);
        if (biome) {
            u32 color = resolver.getColor(*biome, x + dx, z + dz);
            r += (color >> 16) & 0xFF;
            g += (color >> 8) & 0xFF;
            b += color & 0xFF;
            count++;
        }
    }
}

// 平均颜色
return ((r / count) << 16) | ((g / count) << 8) | (b / count);
```

### BiomeColorCache

**职责**：缓存计算结果，避免重复计算。

**缓存结构**：

- 按 `(chunkX, chunkZ)` 组织缓存条目
- 每个条目存储 16×16 网格的颜色
- 支持三种颜色解析器（草、树叶、水）独立缓存

**缓存失效**：

- 区块卸载时：清理该区块及其邻居区块的边缘缓存
- 方块变化时：清理特定位置

**线程安全**：使用 `std::mutex` 保护内部状态。

### ChunkBiomeAccessor

**职责**：提供区块和邻居区块的生物群系访问接口。

**邻居顺序**：

| 索引 | 方向 | 相对位置 |
|------|------|----------|
| 0 | 西 (-X) | chunkX - 1 |
| 1 | 东 (+X) | chunkX + 1 |
| 2 | 北 (-Z) | chunkZ - 1 |
| 3 | 南 (+Z) | chunkZ + 1 |

**坐标转换**：

```cpp
// 世界坐标 → 区块坐标
ChunkCoord chunkX = worldX >> 4;
ChunkCoord chunkZ = worldZ >> 4;

// 世界坐标 → 区块内坐标
i32 localX = worldX & 15;
i32 localZ = worldZ & 15;
```

## 性能考虑

### 缓存命中率

- 每个区块有 16×16 = 256 个位置
- 每个位置有 3 种颜色（草、树叶、水）
- 理论上每个区块最多缓存 256 × 3 = 768 个颜色值
- 实际命中率取决于混合半径和地形复杂度

### 内存占用

- 每个 `BiomeColorCacheEntry`：约 3KB
- 默认缓存 256 个区块：约 768KB
- 最大混合半径 (7)：每次计算最多 225 次生物群系查询

### 性能优化建议

1. **合理设置混合半径**：默认 2 已足够，更大值会显著增加计算量
2. **启用缓存**：对于静态地形，缓存命中率接近 100%
3. **及时清理缓存**：区块卸载时调用 `invalidateChunk()`

## 与 ChunkMesher 集成

```cpp
// ChunkMesher.hpp
class ChunkMesher {
    static BiomeColorBlender s_biomeColorBlender;

public:
    static void setBiomeBlendRadius(i32 radius);
    static i32 biomeBlendRadius();
    static void invalidateBiomeColorCache(ChunkCoord x, ChunkCoord z);
};

// ChunkMesher.cpp
u32 ChunkMesher::resolveTintColorBlended(
    const ChunkBiomeAccessor& accessor,
    i32 worldX, i32 worldY, i32 worldZ,
    const BlockState* block, i32 tintIndex
) {
    // 水体颜色
    if (block->is(VanillaBlocks::WATER)) {
        return s_biomeColorBlender.getBlendedColorCached(
            accessor, worldX, worldY, worldZ,
            BiomeColors::waterColorResolver(),
            BiomeColorBlender::ResolverId::Water
        );
    }

    // 草颜色
    return s_biomeColorBlender.getBlendedColorCached(
        accessor, worldX, worldY, worldZ,
        BiomeColors::grassColorResolver(),
        BiomeColorBlender::ResolverId::Grass
    );
}
```

## 容易踩的坑

### 1. 邻居区块缺失

**问题**：混合采样跨越区块边界时，邻居区块可能未加载。

**解决方案**：
- `ChunkBiomeAccessor::getBiome()` 返回 `nullptr` 时跳过
- `averageColors()` 处理空数组情况

### 2. 缓存一致性与混合半径

**问题**：混合半径变化后，缓存结果不正确。

**解决方案**：
- `setBiomeBlendRadius()` 自动清空缓存

### 3. 对角线区块访问

**问题**：当前实现不支持访问对角线方向的区块。

**解决方案**：
- 混合半径最大为 7，但区块访问仅支持东、西、南、北四个邻居
- 对于半径 ≤ 7 的混合，通常不需要访问更远的区块
- 如果需要，可以扩展 `ChunkBiomeAccessor` 支持更多邻居

## 测试

```cpp
// 测试颜色平均
TEST(BiomeColorBlender, AverageColors) {
    u32 colors[] = {0xFF0000, 0x00FF00, 0x0000FF};
    u32 avg = BiomeColorBlender::averageColors(colors, 3);
    // 平均值约为 0x555555
    EXPECT_EQ(avg, 0x555555);
}

// 测试混合半径
TEST(BiomeColorBlender, BlendRadius) {
    BiomeColorBlender blender;
    blender.setBlendRadius(3);
    EXPECT_EQ(blender.blendRadius(), 3);
}

// 测试缓存
TEST(BiomeColorCache, CacheHit) {
    BiomeColorCache cache;
    i32 callCount = 0;

    auto compute = [&]() { callCount++; return 0xFF0000; };

    // 第一次调用，未命中
    cache.getOrCompute(0, 0, 0, 0, 0, compute);
    EXPECT_EQ(callCount, 1);

    // 第二次调用，命中
    cache.getOrCompute(0, 0, 0, 0, 0, compute);
    EXPECT_EQ(callCount, 1);  // compute 未被调用
}
```

## 参考资料

- MC 1.16.5 `ClientWorld.getBlockColorRaw()` - 生物群系混合算法
- MC 1.16.5 `ColorCache` - 颜色缓存实现
- MC 1.16.5 `CubeCoordinateIterator` - 坐标迭代器
