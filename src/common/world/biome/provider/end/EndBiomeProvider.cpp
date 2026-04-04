#include "EndBiomeProvider.hpp"
#include "../../BiomeRegistry.hpp"
#include "../../layer/BiomeValues.hpp"
#include "../../../../util/math/random/Random.hpp"

#include <algorithm>
#include <cmath>

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

    // 参考原版：SharedSeedRandom(seed).skip(17292) 后初始化 SimplexNoiseGenerator。
    rng.skip(17292);
    m_islandNoise = std::make_unique<SimplexNoiseGenerator>(rng);
}

// ============================================================================
// 生物群系获取
// ============================================================================

BiomeId EndBiomeProvider::getBiome(i32 x, i32 y, i32 z) const {
    MC_UNUSED(y);
    return getNoiseBiome(x >> 2, 0, z >> 2);
}

BiomeId EndBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const {
    MC_UNUSED(noiseY);
    return selectBiome(noiseX, noiseZ);
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
    // 对外部世界坐标保持语义：等价于原版 (noiseX >> 2)^2 + (noiseZ >> 2)^2 <= 4096。
    const i64 i = static_cast<i64>(x >> 4);
    const i64 j = static_cast<i64>(z >> 4);
    return i * i + j * j <= MAIN_ISLAND_RADIUS_SQ;
}

f32 EndBiomeProvider::getIslandHeight(i32 x, i32 z) const {
    return computeIslandHeight(*m_islandNoise, x, z);
}

// ============================================================================
// 生物群系选择
// ============================================================================

BiomeId EndBiomeProvider::selectBiome(i32 noiseX, i32 noiseZ) const {
    // 参考原版 EndBiomeProvider#getNoiseBiome：
    // i = noiseX >> 2, j = noiseZ >> 2
    // if i*i + j*j <= 4096 -> THE_END
    // f = func_235317_a_(generator, i*2+1, j*2+1)
    // f > 40 -> END_HIGHLANDS
    // f >= 0 -> END_MIDLANDS
    // f < -20 -> SMALL_END_ISLANDS
    // else -> END_BARRENS

    const i64 i = static_cast<i64>(noiseX >> 2);
    const i64 j = static_cast<i64>(noiseZ >> 2);
    if (i * i + j * j <= MAIN_ISLAND_RADIUS_SQ) {
        return EndBiomes::TheEnd;
    }

    const f32 height = getIslandHeight((noiseX >> 2) * 2 + 1, (noiseZ >> 2) * 2 + 1);

    if (height > 40.0f) {
        return EndBiomes::EndHighlands;
    }

    if (height >= 0.0f) {
        return EndBiomes::EndMidlands;
    }

    return height < -20.0f ? EndBiomes::SmallEndIslands : EndBiomes::EndBarrens;
}

f32 EndBiomeProvider::computeIslandHeight(
    const SimplexNoiseGenerator& noise,
    i32 x,
    i32 z)
{
    const i32 i = x / 2;
    const i32 j = z / 2;
    const i32 k = x % 2;
    const i32 l = z % 2;

    f32 height = 100.0f - std::sqrt(static_cast<f32>(x * x + z * z)) * 8.0f;
    height = std::clamp(height, -100.0f, 80.0f);

    for (i32 i1 = -12; i1 <= 12; ++i1) {
        for (i32 j1 = -12; j1 <= 12; ++j1) {
            const i64 k1 = static_cast<i64>(i + i1);
            const i64 l1 = static_cast<i64>(j + j1);

            if (k1 * k1 + l1 * l1 <= MAIN_ISLAND_RADIUS_SQ) {
                continue;
            }

            if (noise.getValue(static_cast<f64>(k1), static_cast<f64>(l1)) >= -0.9) {
                continue;
            }

            const f32 f1 =
                std::fmod(std::abs(static_cast<f32>(k1)) * 3439.0f + std::abs(static_cast<f32>(l1)) * 147.0f, 13.0f) +
                9.0f;
            const f32 f2 = static_cast<f32>(k - i1 * 2);
            const f32 f3 = static_cast<f32>(l - j1 * 2);
            f32 f4 = 100.0f - std::sqrt(f2 * f2 + f3 * f3) * f1;
            f4 = std::clamp(f4, -100.0f, 80.0f);
            height = std::max(height, f4);
        }
    }

    return height;
}

// ============================================================================
// 生物群系容器填充
// ============================================================================

void EndBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) {
    const i32 startNoiseX = chunkX << 2;
    const i32 startNoiseZ = chunkZ << 2;

    for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
        for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
            const i32 noiseX = startNoiseX + bx;
            const i32 noiseZ = startNoiseZ + bz;
            const BiomeId biome = getNoiseBiome(noiseX, 0, noiseZ);

            for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
                container.setBiome(bx, by, bz, biome);
            }
        }
    }
}

} // namespace end
} // namespace biome
} // namespace mc
