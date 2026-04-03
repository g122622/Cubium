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

[[nodiscard]] const BlockState* getBadlandsTerracottaLayer(i32 worldX, i32 worldY, i32 worldZ) {
    static const std::array<Block*, 64> kBands = {
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::YELLOW_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::BROWN_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::RED_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::WHITE_TERRACOTTA,
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::YELLOW_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::BROWN_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::RED_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::WHITE_TERRACOTTA,
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::YELLOW_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::BROWN_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::RED_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::WHITE_TERRACOTTA,
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::YELLOW_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::BROWN_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::RED_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::WHITE_TERRACOTTA,
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA,
        VanillaBlocks::TERRACOTTA,
        VanillaBlocks::ORANGE_TERRACOTTA
    };

    const f64 bandNoise = std::sin(
        static_cast<f64>(worldX) / 512.0 +
        static_cast<f64>(worldZ) / 512.0
    );
    const i32 offset = static_cast<i32>(std::round(bandNoise * 2.0));
    const i32 bandIndex = ((worldY + offset) % 64 + 64) % 64;

    const BlockState* state = getStateOrNull(kBands[static_cast<size_t>(bandIndex)]);
    if (state != nullptr) {
        return state;
    }

    return getStateOrNull(VanillaBlocks::TERRACOTTA);
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

// ============================================================================
// DefaultSurfaceBuilder 实现
// ============================================================================

void DefaultSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;  // 基类实现不直接使用生物群系
    (void)defaultFluid;

    // 获取方块状态
    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* underWaterState = config.underWaterBlock;

    if (!topState || !underState || !underWaterState || !defaultBlock) {
        return;
    }

    // 计算地表深度
    i32 depth = calculateDepth(surfaceNoise, random);

    // 从上到下遍历
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            // 空气，重置深度
            currentDepth = -1;
            continue;
        }

        // 检查是否是石头（默认方块）
        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 到达地表
                if (y >= seaLevel - 4) {
                    // 水面以上，放置表层
                    chunk.setBlock(x, y, z, topState);
                } else {
                    // 水面以下，放置水下表面
                    chunk.setBlock(x, y, z, underWaterState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 放置次层
                chunk.setBlock(x, y, z, underState);
                --currentDepth;
            }
        }
    }
}

i32 DefaultSurfaceBuilder::calculateDepth(f32 noise, math::Random& random) const
{
    // 参考 MC DefaultSurfaceBuilder
    // 地表深度 = noise / 3.0 + 3.0 + random
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
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* snowState = VanillaBlocks::getState(VanillaBlocks::SNOW);
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 高海拔处放置雪
                if (shouldPlaceSnow(y, biome)) {
                    if (snowState) {
                        chunk.setBlock(x, y, z, snowState);
                    } else if (stoneState) {
                        chunk.setBlock(x, y, z, stoneState);
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

bool MountainSurfaceBuilder::shouldPlaceSnow(i32 y, const Biome& biome) const
{
    // 温度低于 0.15 且高度高于 90 时放置雪
    return biome.temperature() < 0.15f && y > 90;
}

// ============================================================================
// DesertSurfaceBuilder 实现
// ============================================================================

void DesertSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;
    (void)seaLevel;

    const BlockState* sandState = config.topBlock;
    const BlockState* sandstoneState = VanillaBlocks::getState(VanillaBlocks::SANDSTONE);

    if (!sandState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                chunk.setBlock(x, y, z, sandState);
                currentDepth = depth;
            } else if (currentDepth > 0) {
                if (sandstoneState) {
                    chunk.setBlock(x, y, z, sandstoneState);
                } else {
                    chunk.setBlock(x, y, z, sandState);
                }
                --currentDepth;
            }
        }
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
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)biome;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* clayState = VanillaBlocks::getState(VanillaBlocks::CLAY);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 水面附近可能放置粘土
                if (y < seaLevel && shouldPlaceClay(surfaceNoise)) {
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

bool SwampSurfaceBuilder::shouldPlaceClay(f32 noise) const
{
    // 噪声值大于阈值时放置粘土
    return noise > 0.5;
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
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* iceState = VanillaBlocks::getState(VanillaBlocks::ICE);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
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
                if (y == seaLevel && iceState) {
                    chunk.setBlock(x, y, z, iceState);
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

// ============================================================================
// BadlandsSurfaceBuilder 实现
// ============================================================================

void BadlandsSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;
    (void)seaLevel;

    const BlockState* terracottaState = getStateOrNull(VanillaBlocks::TERRACOTTA);
    const BlockState* redSandState = VanillaBlocks::getState(VanillaBlocks::RED_SAND);
    const BlockState* orangeTerracottaState = VanillaBlocks::getState(VanillaBlocks::ORANGE_TERRACOTTA);
    const BlockState* topState = config.topBlock;

    if (!topState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
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
                    const BlockState* layerState = getBadlandsTerracottaLayer(worldX, y, worldZ);
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
                    layerState = getBadlandsTerracottaLayer(worldX, y, worldZ);
                }
                if (!layerState) {
                    layerState = terracottaState ? terracottaState : topState;
                }
                chunk.setBlock(x, y, z, layerState);
            }
        }
    }
}

// ============================================================================
// BeachSurfaceBuilder 实现
// ============================================================================

void BeachSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)biome;
    (void)defaultFluid;

    const BlockState* sandState = config.topBlock;
    const BlockState* sandstoneState = VanillaBlocks::getState(VanillaBlocks::SANDSTONE);
    const BlockState* topState = config.underBlock;

    if (!sandState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 海平面附近使用沙子
                if (y <= seaLevel + 2) {
                    chunk.setBlock(x, y, z, sandState);
                } else if (topState) {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                if (y <= seaLevel && sandstoneState) {
                    chunk.setBlock(x, y, z, sandstoneState);
                } else {
                    chunk.setBlock(x, y, z, sandState);
                }
                --currentDepth;
            }
        }
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
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)biome;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* podzolState = VanillaBlocks::getState(VanillaBlocks::PODZOL);
    const BlockState* coarseDirtState = VanillaBlocks::getState(VanillaBlocks::COARSE_DIRT);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 巨型针叶林使用灰化土作为表层
                if (podzolState) {
                    chunk.setBlock(x, y, z, podzolState);
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 次层使用砂土
                if (coarseDirtState) {
                    chunk.setBlock(x, y, z, coarseDirtState);
                } else {
                    chunk.setBlock(x, y, z, underState);
                }
                --currentDepth;
            }
        }
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
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)biome;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    const BlockState* coarseDirtState = VanillaBlocks::getState(VanillaBlocks::COARSE_DIRT);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    // 破碎热带草原有更高的石头生成概率
    bool placeStone = random.nextFloat() < 0.3f;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            placeStone = random.nextFloat() < 0.3f;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 随机放置石头或草方块
                if (placeStone && stoneState) {
                    chunk.setBlock(x, y, z, stoneState);
                } else {
                    chunk.setBlock(x, y, z, topState);
                }
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 次层使用砂土或石头
                if (coarseDirtState) {
                    chunk.setBlock(x, y, z, coarseDirtState);
                } else {
                    chunk.setBlock(x, y, z, underState);
                }
                --currentDepth;
            }
        }
    }
}

// ============================================================================
// BambooJungleSurfaceBuilder 实现
// ============================================================================

void BambooJungleSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)biome;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* podzolState = VanillaBlocks::getState(VanillaBlocks::PODZOL);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                // 竹林有概率使用灰化土
                if (random.nextFloat() < 0.2f && podzolState) {
                    chunk.setBlock(x, y, z, podzolState);
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

// ============================================================================
// NetherForestsSurfaceBuilder 实现
// ============================================================================

void NetherForestsSurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;
    (void)biome;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* netherrackState = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
    i32 currentDepth = -1;

    for (i32 y = startHeight; y >= 0; --y) {
        const BlockState* currentState = chunk.getBlock(x, y, z);

        if (!currentState || currentState->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (currentState->blockId() == static_cast<u32>(defaultBlock->blockId())) {
            if (currentDepth == -1) {
                chunk.setBlock(x, y, z, topState);
                currentDepth = depth;
            } else if (currentDepth > 0) {
                // 下界森林下层使用下界岩
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

// ============================================================================
// SoulSandValleySurfaceBuilder 实现
// ============================================================================

void SoulSandValleySurfaceBuilder::buildSurface(
    math::Random& random,
    ChunkPrimer& chunk,
    const Biome& biome,
    i32 x, i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    const BlockState* defaultBlock,
    const BlockState* defaultFluid,
    i32 seaLevel,
    const SurfaceBuilderConfig& config)
{
    (void)defaultFluid;
    (void)seaLevel;
    (void)biome;

    const BlockState* topState = config.topBlock;
    const BlockState* underState = config.underBlock;
    const BlockState* soulSandState = VanillaBlocks::getState(VanillaBlocks::SOUL_SAND);
    const BlockState* soulSoilState = VanillaBlocks::getState(VanillaBlocks::SOUL_SOIL);

    if (!topState || !underState || !defaultBlock) {
        return;
    }

    i32 depth = static_cast<i32>(surfaceNoise / 3.0 + 3.0 + random.nextDouble() * 0.25);
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

} // namespace mc
