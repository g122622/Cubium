#include "SurfaceBuilders.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../biome/Biome.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/MathConstants.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace mc {

namespace {

[[nodiscard]] const BlockState* getStateOrNull(Block* block) {
    return block != nullptr ? VanillaBlocks::getState(block) : nullptr;
}

/**
 * @brief 全局 INFO_NOISE 噪声生成器
 *
 * 参考 MC Biome.INFO_NOISE，用于沼泽等地表构建器。
 * MC 中这是一个静态共享的 PerlinNoiseGenerator。
 */
class GlobalInfoNoise {
public:
    static PerlinNoiseGenerator& instance() {
        // 使用固定种子初始化（MC Biome.INFO_NOISE 使用派生种子）
        static PerlinNoiseGenerator s_noise(12345ULL, 0, 0);
        return s_noise;
    }
};

} // anonymous namespace

// ============================================================================
// SurfaceBuilderConfig 静态方法实现
// ============================================================================

SurfaceBuilderConfig SurfaceBuilderConfig::grass()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK),
        VanillaBlocks::getState(VanillaBlocks::DIRT),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::sand()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::SAND),
        VanillaBlocks::getState(VanillaBlocks::SAND),
        VanillaBlocks::getState(VanillaBlocks::SAND)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::stone()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::STONE),
        VanillaBlocks::getState(VanillaBlocks::STONE),
        VanillaBlocks::getState(VanillaBlocks::STONE)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::gravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::GRAVEL),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::redSand()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::RED_SAND),
        VanillaBlocks::getState(VanillaBlocks::RED_SAND),
        VanillaBlocks::getState(VanillaBlocks::RED_SAND)
    );
}

// ========== MC原版预设配置 ==========

SurfaceBuilderConfig SurfaceBuilderConfig::podzolDirtGravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::PODZOL),
        VanillaBlocks::getState(VanillaBlocks::DIRT),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::gravelOnly()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::GRAVEL),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::grassDirtGravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK),
        VanillaBlocks::getState(VanillaBlocks::DIRT),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::stoneStoneGravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::STONE),
        VanillaBlocks::getState(VanillaBlocks::STONE),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::coarseDirtDirtGravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::COARSE_DIRT),
        VanillaBlocks::getState(VanillaBlocks::DIRT),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::sandSandGravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::SAND),
        VanillaBlocks::getState(VanillaBlocks::SAND),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::grassDirtSand()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK),
        VanillaBlocks::getState(VanillaBlocks::DIRT),
        VanillaBlocks::getState(VanillaBlocks::SAND)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::redSandWhiteTerracottaGravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::RED_SAND),
        VanillaBlocks::getState(VanillaBlocks::WHITE_TERRACOTTA),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::myceliumDirtGravel()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::MYCELIUM),
        VanillaBlocks::getState(VanillaBlocks::DIRT),
        VanillaBlocks::getState(VanillaBlocks::GRAVEL)
    );
}

SurfaceBuilderConfig SurfaceBuilderConfig::netherrack()
{
    return SurfaceBuilderConfig(
        VanillaBlocks::getState(VanillaBlocks::NETHERRACK),
        VanillaBlocks::getState(VanillaBlocks::NETHERRACK),
        VanillaBlocks::getState(VanillaBlocks::NETHERRACK)
    );
}

// ============================================================================
// SurfaceBuilder 基类实现
// ============================================================================

void SurfaceBuilder::buildDefaultSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const BlockState* top,
    const BlockState* middle,
    const BlockState* bottom)
{
    const i32 k = x & 15;  // 区块内X坐标
    const i32 l = z & 15;  // 区块内Z坐标

    // 计算地表深度
    // 参考 MC DefaultSurfaceBuilder 第25行
    const i32 j = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);

    const BlockState* blockstate = top;       // 表层方块
    const BlockState* blockstate1 = middle;   // 次层方块
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            // 空气，重置深度
            currentDepth = -1;
            continue;
        }

        // 检查是否是默认方块（石头）
        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 到达地表，确定表层方块

                // MC第36-39行：深度<=0时的处理
                if (j <= 0) {
                    blockstate = nullptr;  // AIR
                    blockstate1 = defaultBlock;
                } else if (y >= seaLevel - 4 && y <= seaLevel + 1) {
                    // MC第39-41行：海平面附近使用配置的表层
                    blockstate = top;
                    blockstate1 = middle;
                }

                // MC第44-52行：水下填充逻辑
                if (y < seaLevel && (blockstate == nullptr || blockstate->isAir())) {
                    // 根据生物群系温度决定放冰还是水
                    if (biome.temperature() < 0.15f) {
                        blockstate = VanillaBlocks::getState(VanillaBlocks::ICE);
                    } else {
                        blockstate = defaultFluid;
                    }
                }

                currentDepth = j;

                // MC第55-63行：放置表层方块
                if (y >= seaLevel - 1) {
                    // 水面或以上
                    if (blockstate != nullptr) {
                        chunk.setBlock(x, y, z, blockstate);
                    }
                } else if (y < seaLevel - 7 - j) {
                    // MC第57-60行：深层水下底板
                    blockstate = nullptr;  // AIR
                    blockstate1 = defaultBlock;
                    if (bottom != nullptr) {
                        chunk.setBlock(x, y, z, bottom);
                    }
                } else {
                    // 次层
                    if (blockstate1 != nullptr) {
                        chunk.setBlock(x, y, z, blockstate1);
                    }
                }
            } else if (currentDepth > 0) {
                // 继续填充次层
                --currentDepth;
                if (blockstate1 != nullptr) {
                    chunk.setBlock(x, y, z, blockstate1);
                }

                // MC第67-70行：砂岩替换逻辑
                if (currentDepth == 0 && blockstate1 != nullptr && j > 1) {
                    const Block* block = &blockstate1->owner();
                    if (block == VanillaBlocks::SAND || block == VanillaBlocks::RED_SAND) {
                        currentDepth = random.nextInt(4) + std::max(0, y - 63);
                        if (block == VanillaBlocks::RED_SAND) {
                            blockstate1 = VanillaBlocks::getState(VanillaBlocks::RED_SANDSTONE);
                        } else {
                            blockstate1 = VanillaBlocks::getState(VanillaBlocks::SANDSTONE);
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// DefaultSurfaceBuilder 实现
// ============================================================================

void DefaultSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)worldSeed;  // DefaultSurfaceBuilder不需要种子

    buildDefaultSurface(
        random, chunk, biome, x, z, startHeight, surfaceNoise,
        defaultBlock, defaultFluid, seaLevel,
        config.topBlock, config.underBlock, config.underWaterBlock);
}

i32 DefaultSurfaceBuilder::calculateDepth(f64 noise, math::Random& random) const
{
    // 参考 MC DefaultSurfaceBuilder 第25行
    i32 depth = static_cast<i32>(noise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    return std::max(1, depth);
}

// ============================================================================
// MountainSurfaceBuilder 实现
// ============================================================================

void MountainSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)config;  // MountainSurfaceBuilder根据噪声选择配置
    (void)worldSeed;

    // 参考 MC MountainSurfaceBuilder
    // 根据噪声值委托给DefaultSurfaceBuilder使用不同配置
    if (surfaceNoise > 1.0) {
        const SurfaceBuilderConfig stoneConfig = SurfaceBuilderConfig::stoneStoneGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            stoneConfig.topBlock, stoneConfig.underBlock, stoneConfig.underWaterBlock);
    } else {
        const SurfaceBuilderConfig grassConfig = SurfaceBuilderConfig::grassDirtGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            grassConfig.topBlock, grassConfig.underBlock, grassConfig.underWaterBlock);
    }
}

// ============================================================================
// GravellyMountainSurfaceBuilder 实现
// ============================================================================

void GravellyMountainSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)config;
    (void)worldSeed;

    // 参考 MC GravellyMountainSurfaceBuilder
    // 根据噪声值选择不同配置
    if (surfaceNoise > 2.0 || surfaceNoise < -1.0) {
        const SurfaceBuilderConfig gravelConfig = SurfaceBuilderConfig::gravelOnly();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            gravelConfig.topBlock, gravelConfig.underBlock, gravelConfig.underWaterBlock);
    } else if (surfaceNoise > 1.0) {
        const SurfaceBuilderConfig stoneConfig = SurfaceBuilderConfig::stoneStoneGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            stoneConfig.topBlock, stoneConfig.underBlock, stoneConfig.underWaterBlock);
    } else {
        const SurfaceBuilderConfig grassConfig = SurfaceBuilderConfig::grassDirtGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            grassConfig.topBlock, grassConfig.underBlock, grassConfig.underWaterBlock);
    }
}

// ============================================================================
// ShatteredSavannaSurfaceBuilder 实现
// ============================================================================

void ShatteredSavannaSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)config;
    (void)worldSeed;

    // 参考 MC ShatteredSavannaSurfaceBuilder
    // 根据噪声值委托给DefaultSurfaceBuilder使用不同配置
    if (surfaceNoise > 1.75) {
        const SurfaceBuilderConfig stoneConfig = SurfaceBuilderConfig::stoneStoneGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            stoneConfig.topBlock, stoneConfig.underBlock, stoneConfig.underWaterBlock);
    } else if (surfaceNoise > -0.5) {
        const SurfaceBuilderConfig coarseConfig = SurfaceBuilderConfig::coarseDirtDirtGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            coarseConfig.topBlock, coarseConfig.underBlock, coarseConfig.underWaterBlock);
    } else {
        const SurfaceBuilderConfig grassConfig = SurfaceBuilderConfig::grassDirtGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            grassConfig.topBlock, grassConfig.underBlock, grassConfig.underWaterBlock);
    }
}

// ============================================================================
// GiantTreeTaigaSurfaceBuilder 实现
// ============================================================================

void GiantTreeTaigaSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)config;
    (void)worldSeed;

    // 参考 MC GiantTreeTaigaSurfaceBuilder
    // 根据噪声值委托给DefaultSurfaceBuilder使用不同配置
    if (surfaceNoise > 1.75) {
        const SurfaceBuilderConfig coarseConfig = SurfaceBuilderConfig::coarseDirtDirtGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            coarseConfig.topBlock, coarseConfig.underBlock, coarseConfig.underWaterBlock);
    } else if (surfaceNoise > -0.95) {
        const SurfaceBuilderConfig podzolConfig = SurfaceBuilderConfig::podzolDirtGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            podzolConfig.topBlock, podzolConfig.underBlock, podzolConfig.underWaterBlock);
    } else {
        const SurfaceBuilderConfig grassConfig = SurfaceBuilderConfig::grassDirtGravel();
        buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
            defaultBlock, defaultFluid, seaLevel,
            grassConfig.topBlock, grassConfig.underBlock, grassConfig.underWaterBlock);
    }
}

// ============================================================================
// SwampSurfaceBuilder 实现
// ============================================================================

void SwampSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)worldSeed;

    // 参考 MC SwampSurfaceBuilder 第15-34行
    // 使用Biome.INFO_NOISE在水面附近生成粘土
    const i32 localX = x & 15;
    const i32 localZ = z & 15;

    // 使用全局 INFO_NOISE 采样
    // MC 第16行：Biome.INFO_NOISE.noiseAt((double)x * 0.25D, (double)z * 0.25D, false)
    const f64 infoNoiseValue = static_cast<f64>(
        GlobalInfoNoise::instance().noiseAt(
            static_cast<f32>(x) * 0.25f,
            static_cast<f32>(z) * 0.25f,
            false
        )
    );

    if (infoNoiseValue > 0.0) {
        // 在水面附近查找空气方块下的方块
        for (i32 y = startHeight; y >= 0; --y) {
            const BlockState* state = chunk.getBlock(localX, y, localZ);
            if (!state || !state->isAir()) {
                // 找到非空气方块，检查是否在海平面(62)且不是水
                if (y == 62 && state->blockId() != defaultFluid->blockId()) {
                    // 替换为水
                    chunk.setBlock(localX, y, localZ, defaultFluid);
                }
                break;
            }
        }
    }

    // 委托给 DefaultSurfaceBuilder 完成常规地表生成
    buildDefaultSurface(
        random, chunk, biome, x, z, startHeight, surfaceNoise,
        defaultBlock, defaultFluid, seaLevel,
        config.topBlock, config.underBlock, config.underWaterBlock);
}

void SwampSurfaceBuilder::setSeed(u64 seed)
{
    m_seed = seed;
    // INFO_NOISE 是全局共享的，不需要每个实例初始化
}

// ============================================================================
// FrozenOceanSurfaceBuilder 实现
// ============================================================================

void FrozenOceanSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)worldSeed;

    // 参考 MC FrozenOceanSurfaceBuilder 第30-123行
    const i32 localX = x & 15;
    const i32 localZ = z & 15;

    // 获取温度（参考第34行）
    const f32 temperature = biome.temperature();

    // 计算冰山参数
    f64 icebergHeight = 0.0;
    f64 icebergBase = 0.0;

    // 使用噪声生成器计算冰山尺寸
    // MC 第35行：min(abs(noise), field_205199_h.noiseAt(...) * 15.0)
    if (m_icebergHeightNoise && m_icebergDensityNoise) {
        const f64 noiseValue = std::abs(surfaceNoise);
        const f64 heightNoise = static_cast<f64>(
            m_icebergHeightNoise->noiseAt(
                static_cast<f32>(x) * 0.1f,
                static_cast<f32>(z) * 0.1f,
                false
            )
        ) * 15.0;

        const f64 d2 = std::min(noiseValue, heightNoise);

        if (d2 > 1.8) {
            // MC 第37-43行：计算冰山密度
            const f64 densityNoise = std::abs(
                static_cast<f64>(m_icebergDensityNoise->noiseAt(
                    static_cast<f32>(x) * 0.09765625f,  // 1/1024 * 100
                    static_cast<f32>(z) * 0.09765625f,
                    false
                ))
            );

            icebergHeight = d2 * d2 * 1.2;
            f64 maxHeight = std::ceil(densityNoise * 40.0) + 14.0;

            if (icebergHeight > maxHeight) {
                icebergHeight = maxHeight;
            }

            // 温度影响冰山高度
            if (temperature > 0.1f) {
                icebergHeight -= 2.0;
            }

            if (icebergHeight > 2.0) {
                icebergBase = static_cast<f64>(seaLevel) - icebergHeight - 7.0;
                icebergHeight += static_cast<f64>(seaLevel);
            } else {
                icebergHeight = 0.0;
            }
        }
    }

    // 地表生成参数
    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* packedIceState = VanillaBlocks::getState(VanillaBlocks::PACKED_ICE);
    const BlockState* snowBlockState = VanillaBlocks::getState(VanillaBlocks::SNOW_BLOCK);
    const BlockState* iceState = VanillaBlocks::getState(VanillaBlocks::ICE);
    const BlockState* gravelState = VanillaBlocks::getState(VanillaBlocks::GRAVEL);

    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;
    i32 packedIceCount = 0;
    const i32 maxPackedIce = 2 + random.nextInt(4);
    const i32 snowThreshold = seaLevel + 18 + random.nextInt(10);

    const i32 startY = std::max(startHeight, static_cast<i32>(icebergHeight) + 1);

    for (i32 y = startY; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(localX, y, localZ);

        // 冰山生成（MC 第72-76行）
        if ((!currentState || currentState->isAir()) && y < static_cast<i32>(icebergHeight)) {
            if (random.nextDouble() > 0.01) {
                chunk.setBlock(localX, y, localZ, packedIceState);
            }
        } else if (currentState && currentState->isLiquid() &&
                   y > static_cast<i32>(icebergBase) && y < seaLevel && icebergBase != 0.0) {
            if (random.nextDouble() > 0.15) {
                chunk.setBlock(localX, y, localZ, packedIceState);
            }
        }

        // 常规地表处理
        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            packedIceCount = 0;
        } else if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 到达地表
                if (depth <= 0) {
                    topState = nullptr;  // AIR
                    underState = defaultBlock;
                } else if (y >= seaLevel - 4 && y <= seaLevel + 1) {
                    topState = config.topBlock;
                    underState = config.underBlock;
                }

                // 水下填充
                if (y < seaLevel && (topState == nullptr || topState->isAir())) {
                    if (temperature < 0.15f) {
                        topState = iceState;
                    } else {
                        topState = defaultFluid;
                    }
                }

                currentDepth = depth;

                if (y >= seaLevel - 1) {
                    chunk.setBlock(localX, y, localZ, topState);
                } else if (y < seaLevel - 7 - depth) {
                    topState = nullptr;  // AIR
                    underState = defaultBlock;
                    chunk.setBlock(localX, y, localZ, gravelState);
                } else {
                    chunk.setBlock(localX, y, localZ, underState);
                }
            } else if (currentDepth > 0) {
                --currentDepth;
                chunk.setBlock(localX, y, localZ, underState);

                // 砂岩替换
                if (currentDepth == 0 && underState != nullptr && depth > 1) {
                    const Block* block = &underState->owner();
                    if (block == VanillaBlocks::SAND || block == VanillaBlocks::RED_SAND) {
                        currentDepth = random.nextInt(4) + std::max(0, y - 63);
                        if (block == VanillaBlocks::RED_SAND) {
                            underState = VanillaBlocks::getState(VanillaBlocks::RED_SANDSTONE);
                        } else {
                            underState = VanillaBlocks::getState(VanillaBlocks::SANDSTONE);
                        }
                    }
                }
            }
        } else if (packedIceState && currentState->blockId() == packedIceState->blockId()) {
            // 浮冰转换为雪块（MC 第82-84行）
            if (packedIceCount <= maxPackedIce && y > snowThreshold) {
                chunk.setBlock(localX, y, localZ, snowBlockState);
                ++packedIceCount;
            }
        }
    }
}

void FrozenOceanSurfaceBuilder::setSeed(u64 seed)
{
    if (m_cachedSeed == seed && m_icebergHeightNoise && m_icebergDensityNoise) {
        return;
    }

    m_cachedSeed = seed;
    m_seed = seed;

    // 创建随机数生成器用于噪声初始化
    // MC 使用 SharedSeedRandom，我们使用标准 Random
    math::Random rng(seed);

    // MC 第128-129行：创建两个 PerlinNoiseGenerator
    // field_205199_h: rangeClosed(-3, 0) = 4 octaves
    // field_205200_i: ImmutableList.of(0) = 1 octave
    m_icebergHeightNoise = std::make_unique<PerlinNoiseGenerator>(rng, -3, 0);
    m_icebergDensityNoise = std::make_unique<PerlinNoiseGenerator>(rng, 0, 0);
}

// ============================================================================
// BadlandsSurfaceBuilder 实现
// ============================================================================

void BadlandsSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;
    (void)worldSeed;

    // 参考 MC BadlandsSurfaceBuilder 第35-111行
    const i32 localX = x & 15;
    const i32 localZ = z & 15;

    const BlockState* whiteTerracottaState = getStateOrNull(VanillaBlocks::WHITE_TERRACOTTA);
    const BlockState* orangeTerracottaState = getStateOrNull(VanillaBlocks::ORANGE_TERRACOTTA);
    const BlockState* terracottaState = getStateOrNull(VanillaBlocks::TERRACOTTA);
    const BlockState* topState = config.topBlock;

    if (!topState || !defaultBlock) {
        return;
    }

    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;
    bool useOrangeLayer = false;
    i32 terracottaCount = 0;

    // MC 第44行：cos(noise / 3.0 * PI) > 0 判断是否使用陶瓦色带
    const bool useTerracottaBands = std::cos(surfaceNoise / 3.0 * math::PI) > 0.0;

    for (i32 y = startHeight; y >= 0; --y) {
        if (terracottaCount >= 15) {
            break;
        }

        const BlockState* currentState = chunk.getBlock(localX, y, localZ);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            useOrangeLayer = false;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                useOrangeLayer = false;

                if (depth <= 0) {
                    topState = nullptr;  // AIR
                } else if (y >= seaLevel - 4 && y <= seaLevel + 1) {
                    topState = whiteTerracottaState ? whiteTerracottaState : config.underBlock;
                }

                currentDepth = depth + std::max(0, y - seaLevel);

                if (y >= seaLevel - 1) {
                    if (y > seaLevel + 3 + depth) {
                        // 高于地表，使用陶瓦层
                        const BlockState* layerState = nullptr;
                        if (y >= 64 && y <= 127) {
                            if (useTerracottaBands) {
                                layerState = terracottaState;
                            } else {
                                layerState = getTerracottaLayer(x, y, z);
                            }
                        } else {
                            layerState = orangeTerracottaState;
                        }
                        if (layerState) {
                            chunk.setBlock(localX, y, localZ, layerState);
                        }
                    } else {
                        // 地表使用红沙
                        const BlockState* redSandState = getStateOrNull(VanillaBlocks::RED_SAND);
                        chunk.setBlock(localX, y, localZ, redSandState ? redSandState : topState);
                        useOrangeLayer = true;
                    }
                } else {
                    // 水下方块
                    const BlockState* underState = config.underBlock;
                    chunk.setBlock(localX, y, localZ, underState);

                    // 如果是陶瓦颜色，替换为橙色陶瓦（MC 第93-95行）
                    if (underState && isTerracottaColor(underState)) {
                        chunk.setBlock(localX, y, localZ, orangeTerracottaState);
                    }
                }
            } else if (currentDepth > 0) {
                --currentDepth;

                if (useOrangeLayer) {
                    if (orangeTerracottaState) {
                        chunk.setBlock(localX, y, localZ, orangeTerracottaState);
                    }
                } else {
                    const BlockState* layerState = getTerracottaLayer(x, y, z);
                    if (layerState) {
                        chunk.setBlock(localX, y, localZ, layerState);
                    }
                }
            }

            ++terracottaCount;
        }
    }
}

bool BadlandsSurfaceBuilder::isTerracottaColor(const BlockState* state) const
{
    if (!state) return false;
    const Block* block = &state->owner();
    return block == VanillaBlocks::WHITE_TERRACOTTA ||
           block == VanillaBlocks::ORANGE_TERRACOTTA ||
           block == VanillaBlocks::MAGENTA_TERRACOTTA ||
           block == VanillaBlocks::LIGHT_BLUE_TERRACOTTA ||
           block == VanillaBlocks::YELLOW_TERRACOTTA ||
           block == VanillaBlocks::LIME_TERRACOTTA ||
           block == VanillaBlocks::PINK_TERRACOTTA ||
           block == VanillaBlocks::GRAY_TERRACOTTA ||
           block == VanillaBlocks::LIGHT_GRAY_TERRACOTTA ||
           block == VanillaBlocks::CYAN_TERRACOTTA ||
           block == VanillaBlocks::PURPLE_TERRACOTTA ||
           block == VanillaBlocks::BLUE_TERRACOTTA ||
           block == VanillaBlocks::BROWN_TERRACOTTA ||
           block == VanillaBlocks::GREEN_TERRACOTTA ||
           block == VanillaBlocks::RED_TERRACOTTA ||
           block == VanillaBlocks::BLACK_TERRACOTTA;
}

void BadlandsSurfaceBuilder::setSeed(u64 seed)
{
    if (m_cachedSeed == seed && m_bandOffsetNoise) {
        return;
    }

    m_cachedSeed = seed;
    m_seed = seed;

    // 创建随机数生成器
    math::Random rng(seed);

    // MC 第119-122行：创建噪声生成器
    // field_215435_c: rangeClosed(-3, 0) = 4 octaves
    // field_215437_d: ImmutableList.of(0) = 1 octave
    m_surfaceNoiseA = std::make_unique<PerlinNoiseGenerator>(rng, -3, 0);
    m_surfaceNoiseB = std::make_unique<PerlinNoiseGenerator>(rng, 0, 0);

    // MC 第127-192行：初始化陶瓦色带
    initBands(seed, rng);
}

void BadlandsSurfaceBuilder::initBands(u64 seed, math::IRandom& rng)
{
    // MC BadlandsSurfaceBuilder.func_215430_b (第127-192行)
    // 初始化陶瓦色带数组

    // 默认填充橙色陶瓦
    const BlockState* orangeTerracotta = getStateOrNull(VanillaBlocks::ORANGE_TERRACOTTA);
    const BlockState* yellowTerracotta = getStateOrNull(VanillaBlocks::YELLOW_TERRACOTTA);
    const BlockState* brownTerracotta = getStateOrNull(VanillaBlocks::BROWN_TERRACOTTA);
    const BlockState* redTerracotta = getStateOrNull(VanillaBlocks::RED_TERRACOTTA);
    const BlockState* whiteTerracotta = getStateOrNull(VanillaBlocks::WHITE_TERRACOTTA);
    const BlockState* lightGrayTerracotta = getStateOrNull(VanillaBlocks::LIGHT_GRAY_TERRACOTTA);
    const BlockState* terracotta = getStateOrNull(VanillaBlocks::TERRACOTTA);

    // 默认填充陶瓦
    for (size_t i = 0; i < m_terracottaBands.size(); ++i) {
        m_terracottaBands[i] = terracotta;
    }

    // MC 第131行：创建色带偏移噪声
    m_bandOffsetNoise = std::make_unique<PerlinNoiseGenerator>(rng, 0, 0);

    // MC 第133-138行：添加橙色条带
    for (i32 i = 0; i < 64; i += rng.nextInt(5) + 1) {
        if (i < 64) {
            m_terracottaBands[i] = orangeTerracotta;
        }
    }

    // MC 第140-149行：添加黄色条带
    i32 yellowBands = rng.nextInt(4) + 2;
    for (i32 i = 0; i < yellowBands; ++i) {
        i32 bandLength = rng.nextInt(3) + 1;
        i32 startPos = rng.nextInt(64);
        for (i32 j = 0; startPos + j < 64 && j < bandLength; ++j) {
            m_terracottaBands[startPos + j] = yellowTerracotta;
        }
    }

    // MC 第151-160行：添加棕色条带
    i32 brownBands = rng.nextInt(4) + 2;
    for (i32 i = 0; i < brownBands; ++i) {
        i32 bandLength = rng.nextInt(3) + 2;
        i32 startPos = rng.nextInt(64);
        for (i32 j = 0; startPos + j < 64 && j < bandLength; ++j) {
            m_terracottaBands[startPos + j] = brownTerracotta;
        }
    }

    // MC 第162-171行：添加红色条带
    i32 redBands = rng.nextInt(4) + 2;
    for (i32 i = 0; i < redBands; ++i) {
        i32 bandLength = rng.nextInt(3) + 1;
        i32 startPos = rng.nextInt(64);
        for (i32 j = 0; startPos + j < 64 && j < bandLength; ++j) {
            m_terracottaBands[startPos + j] = redTerracotta;
        }
    }

    // MC 第173-191行：添加白色和浅灰色条带
    i32 whiteBands = rng.nextInt(3) + 3;
    i32 currentPos = 0;
    for (i32 i = 0; i < whiteBands; ++i) {
        currentPos += rng.nextInt(16) + 4;
        if (currentPos < 64) {
            m_terracottaBands[currentPos] = whiteTerracotta;

            // 可能在上方添加浅灰色
            if (currentPos > 1 && rng.nextBoolean()) {
                m_terracottaBands[currentPos - 1] = lightGrayTerracotta;
            }

            // 可能在下方添加浅灰色
            if (currentPos < 63 && rng.nextBoolean()) {
                m_terracottaBands[currentPos + 1] = lightGrayTerracotta;
            }
        }
    }

    (void)seed;  // 已通过 rng 使用
}

const BlockState* BadlandsSurfaceBuilder::getTerracottaLayer(i32 worldX, i32 worldY, i32 worldZ)
{
    // MC BadlandsSurfaceBuilder.func_215431_a (第194-197行)
    if (!m_bandOffsetNoise) {
        return nullptr;
    }

    // 使用噪声计算 Y 轴偏移
    const f64 noiseValue = static_cast<f64>(
        m_bandOffsetNoise->noiseAt(
            static_cast<f32>(worldX) / 512.0f,
            static_cast<f32>(worldZ) / 512.0f,
            false
        )
    ) * 2.0;

    const i32 offset = static_cast<i32>(std::round(noiseValue));
    const i32 bandIndex = ((worldY + offset) % 64 + 64) % 64;

    if (bandIndex >= 0 && bandIndex < static_cast<i32>(m_terracottaBands.size())) {
        return m_terracottaBands[static_cast<size_t>(bandIndex)];
    }
    return nullptr;
}

// ============================================================================
// ErodedBadlandsSurfaceBuilder 实现
// ============================================================================

void ErodedBadlandsSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)config;

    // 参考 MC ErodedBadlandsSurfaceBuilder
    // 侵蚀恶地使用红沙和白陶瓦
    const SurfaceBuilderConfig erodedConfig = SurfaceBuilderConfig::redSandWhiteTerracottaGravel();
    buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
        defaultBlock, defaultFluid, seaLevel,
        erodedConfig.topBlock, erodedConfig.underBlock, erodedConfig.underWaterBlock);
}

// ============================================================================
// WoodedBadlandsSurfaceBuilder 实现
// ============================================================================

void WoodedBadlandsSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)config;

    // 参考 MC WoodedBadlandsSurfaceBuilder
    // 疏林恶地地表有草和泥土
    const BlockState* grassState = VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK);
    const BlockState* dirtState = VanillaBlocks::getState(VanillaBlocks::DIRT);

    buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
        defaultBlock, defaultFluid, seaLevel,
        grassState, dirtState, config.underWaterBlock);
}

// ============================================================================
// NetherSurfaceBuilder 实现
// ============================================================================

void NetherSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;
    (void)biome;
    (void)worldSeed;

    // 参考 MC NetherSurfaceBuilder
    const SurfaceBuilderConfig netherConfig = SurfaceBuilderConfig::netherrack();
    buildDefaultSurface(random, chunk, biome, x, z, startHeight, surfaceNoise,
        defaultBlock, defaultFluid, seaLevel,
        netherConfig.topBlock, netherConfig.underBlock, netherConfig.underWaterBlock);
}

// ============================================================================
// NetherForestsSurfaceBuilder 实现
// ============================================================================

void NetherForestsSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;
    (void)worldSeed;

    // 参考 MC NetherForestsSurfaceBuilder 第23-73行
    const i32 localX = x & 15;
    const i32 localZ = z & 15;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* netherrackState = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    // MC 第27-28行：使用噪声决定表层类型
    bool useUnderAsTop = false;
    bool useUnderWater = false;

    if (m_noise) {
        const f64 noiseValue = static_cast<f64>(
            m_noise->getValue(
                static_cast<f32>(x) * 0.1f,
                static_cast<f32>(seaLevel),
                static_cast<f32>(z) * 0.1f,
                0.0f, 0.0f, false
            )
        );
        useUnderAsTop = noiseValue > 0.15 + static_cast<f64>(random.nextDouble()) * 0.35;

        const f64 noiseValue2 = static_cast<f64>(
            m_noise->getValue(
                static_cast<f32>(x) * 0.1f,
                109.0f,
                static_cast<f32>(z) * 0.1f,
                0.0f, 0.0f, false
            )
        );
        useUnderWater = noiseValue2 > 0.25 + static_cast<f64>(random.nextDouble()) * 0.9;
    }

    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    // 下界高度从127开始向下
    for (i32 y = 127; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(localX, y, localZ);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                bool forceUnder = false;

                if (depth <= 0) {
                    forceUnder = true;
                    underState = config.underBlock;
                }

                // 根据噪声选择表层
                const BlockState* surfaceTop = topState;
                if (useUnderAsTop) {
                    surfaceTop = config.underBlock;
                } else if (useUnderWater) {
                    surfaceTop = config.underWaterBlock;
                }

                currentDepth = depth;

                if (y >= seaLevel - 1) {
                    if (forceUnder && y < seaLevel) {
                        // 水下使用默认流体
                        chunk.setBlock(localX, y, localZ, defaultFluid);
                    } else {
                        chunk.setBlock(localX, y, localZ, surfaceTop);
                    }
                } else {
                    chunk.setBlock(localX, y, localZ, underState);
                }
            } else if (currentDepth > 0) {
                --currentDepth;
                // 下层使用下界岩
                if (netherrackState) {
                    chunk.setBlock(localX, y, localZ, netherrackState);
                } else {
                    chunk.setBlock(localX, y, localZ, underState);
                }
            }
        }
    }
}

void NetherForestsSurfaceBuilder::setSeed(u64 seed)
{
    if (m_cachedSeed == seed && m_noise) {
        return;
    }

    m_cachedSeed = seed;
    m_seed = seed;

    // MC 第75-77行：创建 OctavesNoiseGenerator
    // ImmutableList.of(0) = 1 octave
    math::Random rng(seed);
    m_noise = std::make_unique<OctavesNoiseGenerator>(rng, 0, 0);
}

// ============================================================================
// SoulSandValleySurfaceBuilder 实现
// ============================================================================

void SoulSandValleySurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;
    (void)biome;
    (void)worldSeed;

    // 参考 MC SoulSandValleySurfaceBuilder
    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* soulSandState = VanillaBlocks::getState(VanillaBlocks::SOUL_SAND);
    const BlockState* soulSoilState = VanillaBlocks::getState(VanillaBlocks::SOUL_SOIL);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 灵魂沙峡谷表层使用灵魂沙
                if (soulSandState) {
                    chunk.setBlock(x, y, z, soulSandState);
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 次层使用灵魂土
                if (soulSoilState) {
                    chunk.setBlock(x, y, z, soulSoilState);
                } else if (soulSandState) {
                    chunk.setBlock(x, y, z, soulSandState);
                } else {
                    chunk.setBlock(x, y, z, underState);
                }
                --currentDepth;
            }
        }
    }
}

// ============================================================================
// BasaltDeltasSurfaceBuilder 实现
// ============================================================================

void BasaltDeltasSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f64 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    u64 worldSeed,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;
    (void)biome;
    (void)worldSeed;

    // 参考 MC BasaltDeltasSurfaceBuilder
    const BlockState* basaltState = VanillaBlocks::getState(VanillaBlocks::BASALT);
    const BlockState* blackstoneState = VanillaBlocks::getState(VanillaBlocks::BLACKSTONE);

    if (!basaltState || !defaultBlock) {
        return;
    }

    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 玄武岩三角洲表层使用玄武岩
                chunk.setBlock(x, y, z, basaltState);
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 次层使用黑石
                if (blackstoneState) {
                    chunk.setBlock(x, y, z, blackstoneState);
                } else {
                    chunk.setBlock(x, y, z, basaltState);
                }
                --currentDepth;
            }
        }
    }
}

// ============================================================================
// NoopSurfaceBuilder 实现
// ============================================================================

// NoopSurfaceBuilder 的 buildSurface 在头文件中定义为空操作

} // namespace mc
