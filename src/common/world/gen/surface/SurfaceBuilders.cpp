#include "SurfaceBuilders.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../biome/Biome.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace mc {

namespace {

[[nodiscard]] const BlockState* getStateOrNull(Block* block) {
    return block != nullptr ? VanillaBlocks::getState(block) : nullptr;
}

} // namespace

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
                    // TODO: MC使用biome.getTemperature(pos)，需要实现基于位置的温度计算
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

    // 参考 MC SwampSurfaceBuilder
    // 使用Biome.INFO_NOISE在水面附近生成粘土
    // TODO: 需要实现Biome.INFO_NOISE采样
    // 暂时使用surfaceNoise参数作为近似

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* clayState = VanillaBlocks::getState(VanillaBlocks::CLAY);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    // 计算地表深度
    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    // 使用噪声决定粘土生成位置
    // MC使用INFO_NOISE采样，这里暂时简化
    const double clayNoise = surfaceNoise * 0.5;  // 简化的噪声

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 水面附近可能放置粘土
                if (y < seaLevel && clayNoise > 0.3) {
                    if (clayState) {
                        chunk.setBlock(x, y, z, clayState);
                    } else {
                        chunk.setBlock(x, y, z, topState);
                    }
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                chunk.setBlock(x, y, z, underState);
                --currentDepth;
            }
        }
    }
}

void SwampSurfaceBuilder::setSeed(u64 seed)
{
    m_seed = seed;
    // TODO: 初始化INFO_NOISE噪声生成器
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

    // 参考 MC FrozenOceanSurfaceBuilder
    // TODO: 实现完整的冰山生成逻辑
    // 目前使用简化的实现

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* iceState = VanillaBlocks::getState(VanillaBlocks::ICE);
    const BlockState* packedIceState = VanillaBlocks::getState(VanillaBlocks::PACKED_ICE);

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
                // 水面可能结冰
                if (y <= seaLevel && iceState) {
                    // 在水面上放置冰
                    chunk.setBlock(x, y, z, packedIceState ? packedIceState : iceState);
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                chunk.setBlock(x, y, z, underState);
                --currentDepth;
            }
        }
    }
}

void FrozenOceanSurfaceBuilder::setSeed(u64 seed)
{
    if (m_cachedSeed == seed) {
        return;
    }
    m_cachedSeed = seed;
    m_seed = seed;
    // TODO: 初始化噪声生成器 m_noiseA 和 m_noiseB
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

    const BlockState* terracottaState = getStateOrNull(VanillaBlocks::TERRACOTTA);
    const BlockState* redSandState = VanillaBlocks::getState(VanillaBlocks::RED_SAND);
    const BlockState* orangeTerracottaState = VanillaBlocks::getState(VanillaBlocks::ORANGE_TERRACOTTA);
    const BlockState* topState = config.topBlock;

    if (!topState || !defaultBlock) {
        return;
    }

    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;
    bool useOrangeLayer = false;
    const i32 worldX = chunk.x() * 16 + x;
    const i32 worldZ = chunk.z() * 16 + z;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            useOrangeLayer = false;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                currentDepth = depth + std::max(0, y - seaLevel);

                if (y >= seaLevel - 1) {
                    const BlockState* surfaceState = redSandState ? redSandState : topState;
                    chunk.setBlock(x, y, z, surfaceState);
                    useOrangeLayer = true;
                } else {
                    const BlockState* layerState = getTerracottaLayer(worldX, y, worldZ);
                    if (!layerState) {
                        layerState = terracottaState ? terracottaState : topState;
                    }
                    chunk.setBlock(x, y, z, layerState);
                    useOrangeLayer = false;
                }
            } else if (currentDepth > 0) {
                --currentDepth;

                const BlockState* layerState = nullptr;
                if (useOrangeLayer && orangeTerracottaState) {
                    layerState = orangeTerracottaState;
                }
                if (!layerState) {
                    layerState = getTerracottaLayer(worldX, y, worldZ);
                }
                if (!layerState) {
                    layerState = terracottaState ? terracottaState : topState;
                }
                chunk.setBlock(x, y, z, layerState);
            }
        }
    }
}

void BadlandsSurfaceBuilder::setSeed(u64 seed)
{
    if (m_cachedSeed == seed) {
        return;
    }
    m_cachedSeed = seed;
    m_seed = seed;
    initBands(seed);
}

void BadlandsSurfaceBuilder::initBands(u64 seed)
{
    // TODO: 使用PerlinNoiseGenerator生成陶瓦色带
    // 参考 MC BadlandsSurfaceBuilder 第27-42行
    (void)seed;
    // 暂时初始化为默认色带
    for (size_t i = 0; i < m_terracottaBands.size(); ++i) {
        m_terracottaBands[i] = VanillaBlocks::getState(VanillaBlocks::TERRACOTTA);
    }
}

const BlockState* BadlandsSurfaceBuilder::getTerracottaLayer(i32 worldX, i32 worldY, i32 worldZ)
{
    // 使用噪声生成陶瓦层
    // 参考 MC BadlandsSurfaceBuilder 第44-59行
    const f64 bandNoise = std::sin(
        static_cast<f64>(worldX) / 512.0 +
        static_cast<f64>(worldZ) / 512.0
    );
    const i32 offset = static_cast<i32>(std::round(bandNoise * 2.0));
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

    // 参考 MC NetherForestsSurfaceBuilder
    // 使用噪声决定表层方块类型
    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* netherrackState = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    const i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    // TODO: 使用噪声生成器决定表层类型
    const bool useNylium = surfaceNoise > 0.0;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 使用菌光体或下界岩
                if (useNylium) {
                    chunk.setBlock(x, y, z, topState);
                } else {
                    if (netherrackState) {
                        chunk.setBlock(x, y, z, netherrackState);
                    } else {
                        chunk.setBlock(x, y, z, topState);
                    }
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 下层使用下界岩
                if (netherrackState) {
                    chunk.setBlock(x, y, z, netherrackState);
                } else {
                    chunk.setBlock(x, y, z, underState);
                }
                --currentDepth;
            }
        }
    }
}

void NetherForestsSurfaceBuilder::setSeed(u64 seed)
{
    if (m_cachedSeed == seed) {
        return;
    }
    m_cachedSeed = seed;
    m_seed = seed;
    // TODO: 初始化噪声生成器 m_noise
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

} // namespace mc
