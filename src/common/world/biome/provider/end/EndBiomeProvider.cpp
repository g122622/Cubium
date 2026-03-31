#include "EndBiomeProvider.hpp"
#include "../../BiomeRegistry.hpp"
#include "../../layer/BiomeValues.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace biome {
namespace end {

// ============================================================================
// 末地生物群系 ID 常量
// ============================================================================

// MC 1.16.5 末地生物群系 ID
// 参考 BiomeValues.hpp
namespace EndBiomes {
    constexpr BiomeId TheEnd = 9;           // 末地主岛
    constexpr BiomeId SmallEndIslands = 40; // 小型末地岛屿
    constexpr BiomeId EndMidlands = 41;     // 末地中部
    constexpr BiomeId EndHighlands = 42;    // 末地高地
    constexpr BiomeId EndBarrens = 43;      // 末地荒地
}

// ============================================================================
// 构造函数
// ============================================================================

EndBiomeProvider::EndBiomeProvider(u64 seed)
    : BiomeProvider(seed)
{
    math::Random rng(seed);

    // 初始化岛屿噪声生成器
    // 参考 MC 1.16.5 EndBiomeProvider 构造函数
    m_islandNoise = std::make_unique<SimplexNoiseGenerator>(rng);
}

// ============================================================================
// 生物群系获取
// ============================================================================

BiomeId EndBiomeProvider::getBiome(i32 x, i32 y, i32 z) const {
    // 末地使用 2D 生物群系采样（忽略 Y 坐标）
    // 参考 MC 1.16.5 EndBiomeProvider.getNoiseBiomeAt
    MC_UNUSED(y);
    return selectBiome(x, z, getIslandNoise(x, z));
}

BiomeId EndBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const {
    // 末地使用 2D 采样，noiseY 被忽略
    MC_UNUSED(noiseY);

    // 噪声坐标转回世界坐标（乘以 4）
    const i32 worldX = noiseX << 2;
    const i32 worldZ = noiseZ << 2;

    return selectBiome(worldX, worldZ, getIslandNoise(worldX, worldZ));
}

f32 EndBiomeProvider::getDepth(i32 x, i32 z) const {
    // 末地地形深度（平坦地形）
    MC_UNUSED(x);
    MC_UNUSED(z);
    return 0.0f;
}

f32 EndBiomeProvider::getScale(i32 x, i32 z) const {
    // 末地地形比例（平坦地形）
    MC_UNUSED(x);
    MC_UNUSED(z);
    return 0.0f;
}

// ============================================================================
// 末地特有方法
// ============================================================================

bool EndBiomeProvider::isInMainIsland(i32 x, i32 z) const {
    // 检查是否在主岛范围内
    // 主岛是圆形，半径 256 方块
    // 参考 MC 1.16.5: 主岛中心在 (0, 0)
    // MC 使用 (x >> 2)^2 + (z >> 2)^2 <= 4096，即 x^2 + z^2 <= 65536
    const i64 distSq = static_cast<i64>(x) * x + static_cast<i64>(z) * z;
    return distSq <= static_cast<i64>(MAIN_ISLAND_RADIUS) * MAIN_ISLAND_RADIUS;
}

f32 EndBiomeProvider::getIslandNoise(i32 x, i32 z) const {
    // 岛屿噪声采样
    // 参考 MC 1.16.5 EndBiomeProvider.islandNoise
    // 使用较大的缩放因子，使外岛分布更分散
    constexpr f32 ISLAND_SCALE = 0.0078125f;  // 1/128

    const f32 nx = static_cast<f32>(x) * ISLAND_SCALE;
    const f32 nz = static_cast<f32>(z) * ISLAND_SCALE;

    return m_islandNoise->noise2D(nx, nz);
}

// ============================================================================
// 生物群系选择
// ============================================================================

BiomeId EndBiomeProvider::selectBiome(i32 x, i32 z, f32 noise) const {
    // 参考 MC 1.16.5 EndBiomeProvider.getBiome
    //
    // 末地生物群系选择逻辑：
    // 1. 如果在主岛范围内，返回 The End
    // 2. 外岛区域根据噪声值选择生物群系：
    //    - 噪声 < -0.5: 小型末地岛屿
    //    - 噪声 < 0: 末地荒地
    //    - 噪声 < 0.5: 末地中部
    //    - 噪声 >= 0.5: 末地高地

    // 主岛区域
    if (isInMainIsland(x, z)) {
        return EndBiomes::TheEnd;
    }

    // 外岛区域
    // 噪声值划分：
    // - 小型岛屿：噪声非常低，形成小型岛屿群
    // - 荒地：噪声较低，空旷区域
    // - 中部：噪声中等，过渡区域
    // - 高地：噪声较高，有末地城和紫颂树

    if (noise < -0.5f) {
        return EndBiomes::SmallEndIslands;
    } else if (noise < 0.0f) {
        return EndBiomes::EndBarrens;
    } else if (noise < 0.5f) {
        return EndBiomes::EndMidlands;
    } else {
        return EndBiomes::EndHighlands;
    }
}

// ============================================================================
// 生物群系容器填充
// ============================================================================

void EndBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) {
    // 区块坐标转换为世界坐标
    const i32 worldX = chunkX * 16;
    const i32 worldZ = chunkZ * 16;

    // 末地使用 2D 生物群系采样
    // 整个区块垂直方向使用相同生物群系
    const BiomeId biome = getBiome(worldX + 8, 0, worldZ + 8);

    // 填充整个容器
    for (i32 y = 0; y < 64; ++y) {  // 16 区块段 * 4 采样点
        for (i32 z = 0; z < BiomeContainer::BIOME_DEPTH; ++z) {
            for (i32 x = 0; x < BiomeContainer::BIOME_WIDTH; ++x) {
                container.setBiome(x, y, z, biome);
            }
        }
    }
}

} // namespace end
} // namespace biome
} // namespace mc
