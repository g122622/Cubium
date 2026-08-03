/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "LakeFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <array>
#include <utility>

namespace mc::world::gen::feature::lake {

// 16x8x16 布尔数组尺寸（对应 MC 1.21.11 LakeFeature: new boolean[2048]）
constexpr i32 LAKE_DIM_X = 16;
constexpr i32 LAKE_DIM_Y = 8;
constexpr i32 LAKE_DIM_Z = 16;
constexpr i32 LAKE_ARRAY_SIZE = LAKE_DIM_X * LAKE_DIM_Y * LAKE_DIM_Z; // 2048

/// 流体/空气分界：Y>=4 为洞穴空气，Y<4 为流体
constexpr i32 FLUID_SURFACE_Y = 4;

/// (x * 16 + z) * 8 + y —— 与 MC 索引顺序一致
[[nodiscard]] inline i32 lakeIndex(i32 x, i32 z, i32 y)
{
    return (x * LAKE_DIM_Z + z) * LAKE_DIM_Y + y;
}

LakeFeatureConfig::LakeFeatureConfig(const LakeFeatureConfig& other)
    : fluidProvider(other.fluidProvider ? other.fluidProvider->clone() : nullptr)
    , barrierProvider(other.barrierProvider ? other.barrierProvider->clone() : nullptr)
{}

LakeFeatureConfig& LakeFeatureConfig::operator=(const LakeFeatureConfig& other)
{
    if (this != &other) {
        fluidProvider = other.fluidProvider ? other.fluidProvider->clone() : nullptr;
        barrierProvider = other.barrierProvider ? other.barrierProvider->clone() : nullptr;
    }
    return *this;
}

const BlockState* LakeFeatureConfig::getFluidState(const IWorld& world, math::IRandom& rng, i32 x, i32 y, i32 z) const
{
    if (fluidProvider != nullptr) {
        return fluidProvider->getState(world, rng, x, y, z);
    }
    return nullptr;
}

const BlockState* LakeFeatureConfig::getBarrierState(const IWorld& world, math::IRandom& rng, i32 x, i32 y, i32 z) const
{
    if (barrierProvider != nullptr) {
        return barrierProvider->getState(world, rng, x, y, z);
    }
    return nullptr;
}

LakeFeature::LakeFeature(LakeFeatureConfig config)
    : m_config(std::move(config))
{}

bool LakeFeature::place(WorldGenRegion& world, math::Random& rng, i32 x, i32 y, i32 z)
{
    // MC: blockpos.getY() <= worldgenlevel.getMinY() + 4 → return false
    if (y <= world::MIN_BUILD_HEIGHT + 4) {
        return false;
    }

    // 中心下移 4 格
    y -= 4;

    // 1. 4~7 个随机椭球叠加到布尔数组
    std::array<bool, LAKE_ARRAY_SIZE> lakeMap{};
    lakeMap.fill(false);
    const i32 ellipsoidCount = 4 + rng.nextInt(4);
    for (i32 e = 0; e < ellipsoidCount; ++e) {
        const f64 rx = rng.nextDouble() * 6.0 + 3.0;
        const f64 ry = rng.nextDouble() * 4.0 + 2.0;
        const f64 rz = rng.nextDouble() * 6.0 + 3.0;
        const f64 cx = rng.nextDouble() * (16.0 - rx - 2.0) + 1.0 + rx / 2.0;
        const f64 cy = rng.nextDouble() * (8.0 - ry - 4.0) + 2.0 + ry / 2.0;
        const f64 cz = rng.nextDouble() * (16.0 - rz - 2.0) + 1.0 + rz / 2.0;

        // MC: for l=1..14, i1=1..14, j1=1..6（内部区域，边缘不参与）
        for (i32 bx = 1; bx < 15; ++bx) {
            const f64 dx = (static_cast<f64>(bx) - cx) / (rx / 2.0);
            const f64 dxx = dx * dx;
            if (dxx >= 1.0) {
                continue;
            }
            for (i32 bz = 1; bz < 15; ++bz) {
                const f64 dz = (static_cast<f64>(bz) - cz) / (rz / 2.0);
                if (dxx + dz * dz >= 1.0) {
                    continue;
                }
                for (i32 by = 1; by < 7; ++by) {
                    const f64 dy = (static_cast<f64>(by) - cy) / (ry / 2.0);
                    if (dxx + dy * dy + dz * dz < 1.0) {
                        lakeMap[lakeIndex(bx, bz, by)] = true;
                    }
                }
            }
        }
    }

    // 本次放置使用的流体/边界状态（按提供者采样，坐标取湖中心）
    const BlockState* const fluidState = m_config.getFluidState(world, rng, x, y, z);
    if (fluidState == nullptr) {
        return false;
    }

    // 2. 边界格校验：自身不在湖内但有相邻格在湖内
    for (i32 bx = 0; bx < LAKE_DIM_X; ++bx) {
        for (i32 bz = 0; bz < LAKE_DIM_Z; ++bz) {
            for (i32 by = 0; by < LAKE_DIM_Y; ++by) {
                const bool isInside = lakeMap[lakeIndex(bx, bz, by)];
                const bool isBoundary = !isInside &&
                    ((bx < 15 && lakeMap[lakeIndex(bx + 1, bz, by)]) ||
                        (bx > 0 && lakeMap[lakeIndex(bx - 1, bz, by)]) ||
                        (bz < 15 && lakeMap[lakeIndex(bx, bz + 1, by)]) ||
                        (bz > 0 && lakeMap[lakeIndex(bx, bz - 1, by)]) ||
                        (by < 7 && lakeMap[lakeIndex(bx, bz, by + 1)]) ||
                        (by > 0 && lakeMap[lakeIndex(bx, bz, by - 1)]));
                if (!isBoundary) {
                    continue;
                }
                const BlockState* state = world.getBlockState(x + bx, y + by, z + bz);
                if (state == nullptr) {
                    continue;
                }
                // MC: l2 >= 4 && blockstate3.liquid() → return false
                if (by >= FLUID_SURFACE_Y && state->isLiquid()) {
                    return false;
                }
                // MC: l2 < 4 && !blockstate3.isSolid() && getBlockState != blockstate1 → return false
                if (by < FLUID_SURFACE_Y && !state->isSolid() && state != fluidState) {
                    return false;
                }
            }
        }
    }

    // 3. 雕刻湖内：Y>=4 填 CAVE_AIR，Y<4 填流体
    const BlockState* const caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    for (i32 bx = 0; bx < LAKE_DIM_X; ++bx) {
        for (i32 bz = 0; bz < LAKE_DIM_Z; ++bz) {
            for (i32 by = 0; by < LAKE_DIM_Y; ++by) {
                if (!lakeMap[lakeIndex(bx, bz, by)]) {
                    continue;
                }
                const BlockState* state = world.getBlockState(x + bx, y + by, z + bz);
                if (state == nullptr || !canReplaceBlock(*state)) {
                    continue;
                }
                const bool aboveFluid = by >= FLUID_SURFACE_Y;
                world.setBlockState(x + bx, y + by, z + bz, aboveFluid ? caveAir : fluidState);
                // MC 另对 cave_air 调 scheduleTick + markAboveForPostProcessing；项目流体更新由
                // 区块后处理流水线扫描流体方块完成，无需显式调度。
            }
        }
    }

    // 4. 放置边界方块（barrier 提供者非空时）
    const BlockState* const barrierState = m_config.getBarrierState(world, rng, x, y, z);
    if (barrierState != nullptr && !barrierState->isAir()) {
        for (i32 bx = 0; bx < LAKE_DIM_X; ++bx) {
            for (i32 bz = 0; bz < LAKE_DIM_Z; ++bz) {
                for (i32 by = 0; by < LAKE_DIM_Y; ++by) {
                    const bool isInside = lakeMap[lakeIndex(bx, bz, by)];
                    const bool isBoundary = !isInside &&
                        ((bx < 15 && lakeMap[lakeIndex(bx + 1, bz, by)]) ||
                            (bx > 0 && lakeMap[lakeIndex(bx - 1, bz, by)]) ||
                            (bz < 15 && lakeMap[lakeIndex(bx, bz + 1, by)]) ||
                            (bz > 0 && lakeMap[lakeIndex(bx, bz - 1, by)]) ||
                            (by < 7 && lakeMap[lakeIndex(bx, bz, by + 1)]) ||
                            (by > 0 && lakeMap[lakeIndex(bx, bz, by - 1)]));
                    if (!isBoundary) {
                        continue;
                    }
                    // MC: flag2 && (l3 < 4 || nextInt(2) != 0)
                    if (!(by < FLUID_SURFACE_Y || rng.nextInt(2) != 0)) {
                        continue;
                    }
                    const BlockState* state = world.getBlockState(x + bx, y + by, z + bz);
                    if (state == nullptr || !state->isSolid()) {
                        continue;
                    }
                    // MC: !blockstate.is(BlockTags.LAVA_POOL_STONE_CANNOT_REPLACE)
                    if (BlockTags::LAVA_POOL_STONE_CANNOT_REPLACE().contains(*state)) {
                        continue;
                    }
                    world.setBlockState(x + bx, y + by, z + bz, barrierState);
                }
            }
        }
    }

    // 5. 水湖表面冻结：fluid 是水时，Y=4 表面按生物群系结冰
    if (fluidState->getFluidState() != nullptr) {
        // MC: blockstate1.getFluidState().is(FluidTags.WATER)
        const bool isWater = fluidState->is(VanillaBlocks::WATER);
        if (isWater) {
            const BlockState* const iceState = VanillaBlocks::getState(VanillaBlocks::ICE);
            for (i32 bx = 0; bx < LAKE_DIM_X; ++bx) {
                for (i32 bz = 0; bz < LAKE_DIM_Z; ++bz) {
                    const i32 surfaceY = y + FLUID_SURFACE_Y;
                    const BlockState* state = world.getBlockState(x + bx, surfaceY, z + bz);
                    if (state == nullptr || !canReplaceBlock(*state)) {
                        continue;
                    }
                    const BiomeId biomeId = world.getBiome(x + bx, surfaceY, z + bz);
                    const Biome& biome = BiomeRegistry::instance().get(biomeId);
                    // checkNeighbors=false（湖面冻结无需检查邻居水域暴露）
                    if (biome.shouldFreeze(world, x + bx, surfaceY, z + bz, world::SEA_LEVEL, false)) {
                        if (iceState != nullptr) {
                            world.setBlockState(x + bx, surfaceY, z + bz, iceState);
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool LakeFeature::canReplaceBlock(const BlockState& state)
{
    // MC: !p_190952_.is(BlockTags.FEATURES_CANNOT_REPLACE)
    return !BlockTags::FEATURES_CANNOT_REPLACE().contains(state);
}

} // namespace mc::world::gen::feature::lake

namespace mc {

ConfiguredLakeFeature::ConfiguredLakeFeature(
    world::gen::feature::lake::LakeFeatureConfig config, const char* featureName)
    : m_feature(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredLakeFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)chunk;
    (void)generator;
    // 数据驱动下概率/高度采样由 placement 链（Count/HeightRange 等）完成，
    // 本方法在已确定的 pos 处直接放置湖泊。
    return m_feature.place(region, random, pos.x, pos.y, pos.z);
}

} // namespace mc
