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

#include "GeodeFeature.hpp"

#include "common/util/Direction.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>

namespace mc::world::gen::feature::cave {

namespace {

/// MC Mth.invSqrt(x) = 1.0 / sqrt(x)。
inline f64 invSqrt(f64 x)
{
    return 1.0 / std::sqrt(x);
}

/// MC BuddingAmethystBlock.canClusterGrowAtState：空气或水源。
[[nodiscard]] bool canClusterGrowAtState(const BlockState* state)
{
    if (state == nullptr || state->isAir()) {
        return true;
    }
    const fluid::FluidState* fluid = state->getFluidState();
    return fluid != nullptr && fluid->isSource() && &fluid->getFluid() == fluid::Fluids::WATER();
}

} // namespace

// ============================================================================
// ConfiguredGeodeFeature
// ============================================================================

ConfiguredGeodeFeature::ConfiguredGeodeFeature(std::unique_ptr<GeodeConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredGeodeFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    if (m_config == nullptr) {
        return false;
    }
    return m_feature.place(region, generator, random, pos, *m_config);
}

// ============================================================================
// GeodeFeature
// ============================================================================

bool GeodeFeature::place(
    IWorld& world, IChunkGenerator& generator, math::Random& random, const BlockPos& origin, const GeodeConfig& config)
{
    const i32 minOffset = config.minGenOffset;
    const i32 maxOffset = config.maxGenOffset;

    // 分布点：每个点带一个 pointOffset 偏移，参与 distSqr 累加。
    const i32 k = config.distributionPoints->sample(random);
    // MC: WorldgenRandom(new LegacyRandomSource(seed))，Cubium 用 Xoroshiro 等价种子化。
    math::Random worldgenRandom(generator.seed());
    auto normalNoise = std::make_unique<noise::NormalNoise>(worldgenRandom, -4, std::vector<f64>{1.0});

    std::vector<std::pair<BlockPos, i32>> distributionList;
    std::vector<BlockPos> crackList;

    const f64 d0 = static_cast<f64>(k) / static_cast<f64>(config.outerWallDistance->getMaxValue());
    const GeodeLayerSettings& layers = config.layerSettings;
    const GeodeBlockSettings& blocks = config.blockSettings;
    const GeodeCrackSettings& crack = config.crackSettings;

    const f64 d1 = 1.0 / std::sqrt(layers.filling);
    const f64 d2 = 1.0 / std::sqrt(layers.innerLayer + d0);
    const f64 d3 = 1.0 / std::sqrt(layers.middleLayer + d0);
    const f64 d4 = 1.0 / std::sqrt(layers.outerLayer + d0);
    const f64 d5 = 1.0 / std::sqrt(crack.baseCrackSize + random.nextDouble() / 2.0 + (k > 3 ? d0 : 0.0));
    const bool generateCrack = random.nextFloat() < static_cast<f32>(crack.generateCrackChance);

    i32 invalidCount = 0;
    for (i32 i1 = 0; i1 < k; ++i1) {
        const i32 j1 = config.outerWallDistance->sample(random);
        const i32 k1 = config.outerWallDistance->sample(random);
        const i32 l1 = config.outerWallDistance->sample(random);
        const BlockPos blockpos1(origin.x + j1, origin.y + k1, origin.z + l1);
        const BlockState* blockstate = world.getBlockState(blockpos1);
        // MC: blockstate.isAir() || blockstate.is(invalidBlocks)
        const bool isInvalid = (blockstate == nullptr || blockstate->isAir()) ||
            (blocks.invalidBlocks != nullptr && blockstate != nullptr && blocks.invalidBlocks->contains(*blockstate));
        if (isInvalid) {
            if (++invalidCount > config.invalidBlocksThreshold) {
                return false;
            }
        }
        distributionList.emplace_back(blockpos1, config.pointOffset->sample(random));
    }

    if (generateCrack) {
        const i32 i2 = random.nextInt(4);
        const i32 j2 = k * 2 + 1;
        if (i2 == 0) {
            crackList.emplace_back(origin.x + j2, origin.y + 7, origin.z);
            crackList.emplace_back(origin.x + j2, origin.y + 5, origin.z);
            crackList.emplace_back(origin.x + j2, origin.y + 1, origin.z);
        } else if (i2 == 1) {
            crackList.emplace_back(origin.x, origin.y + 7, origin.z + j2);
            crackList.emplace_back(origin.x, origin.y + 5, origin.z + j2);
            crackList.emplace_back(origin.x, origin.y + 1, origin.z + j2);
        } else if (i2 == 2) {
            crackList.emplace_back(origin.x + j2, origin.y + 7, origin.z + j2);
            crackList.emplace_back(origin.x + j2, origin.y + 5, origin.z + j2);
            crackList.emplace_back(origin.x + j2, origin.y + 1, origin.z + j2);
        } else {
            crackList.emplace_back(origin.x, origin.y + 7, origin.z);
            crackList.emplace_back(origin.x, origin.y + 5, origin.z);
            crackList.emplace_back(origin.x, origin.y + 1, origin.z);
        }
    }

    std::vector<BlockPos> potentialPlacements;
    const BlockTag* const cannotReplace = blocks.cannotReplace;

    // MC: BlockPos.betweenClosed(offset(i,i,i), offset(j,j,j))
    for (i32 x = origin.x + minOffset; x <= origin.x + maxOffset; ++x) {
        for (i32 y = origin.y + minOffset; y <= origin.y + maxOffset; ++y) {
            for (i32 z = origin.z + minOffset; z <= origin.z + maxOffset; ++z) {
                const BlockPos blockpos3(x, y, z);
                const f64 d8 = normalNoise->getValue(static_cast<f64>(x), static_cast<f64>(y), static_cast<f64>(z)) *
                    config.noiseMultiplier;
                f64 d6 = 0.0;
                f64 d7 = 0.0;

                for (const auto& pair : distributionList) {
                    d6 += invSqrt(static_cast<f64>(blockpos3.distanceSq(pair.first)) + static_cast<f64>(pair.second)) +
                        d8;
                }
                for (const BlockPos& blockpos6 : crackList) {
                    d7 += invSqrt(static_cast<f64>(blockpos3.distanceSq(blockpos6)) +
                              static_cast<f64>(crack.crackPointOffset)) +
                        d8;
                }

                if (!(d6 < d4)) {
                    // d6 < d4 时跳过（外层壁之外）。
                } else if (generateCrack && d7 >= d5 && d6 < d1) {
                    safeSetBlock(world, blockpos3, nullptr, cannotReplace);
                    // MC 在此对 crack 邻居流体调 scheduleTick(blockpos2, fluid, 0) 使其立即流动填补裂缝。
                    // 项目 WorldGenRegion 不支持 worldgen 期间调度（tickManager() 抛 std::logic_error），
                    // 流体更新改由区块后处理流水线（ServerChunkManager::_postProcessChunk 扫描 isLiquid 方块
                    // 调 scheduleFluidTick）负责，与 LakeFeature/SpringFeature 的处理一致。
                } else if (d6 >= d1) {
                    const BlockState* state =
                        blocks.fillingProvider->getState(world, random, blockpos3.x, blockpos3.y, blockpos3.z);
                    safeSetBlock(world, blockpos3, state, cannotReplace);
                } else if (d6 >= d2) {
                    const bool alternate = random.nextFloat() < static_cast<f32>(config.useAlternateLayer0Chance);
                    const BlockState* state = alternate
                        ? blocks.alternateInnerLayerProvider->getState(
                              world, random, blockpos3.x, blockpos3.y, blockpos3.z)
                        : blocks.innerLayerProvider->getState(world, random, blockpos3.x, blockpos3.y, blockpos3.z);
                    safeSetBlock(world, blockpos3, state, cannotReplace);
                    if ((!config.placementsRequireLayer0Alternate || alternate) &&
                        random.nextFloat() < static_cast<f32>(config.usePotentialPlacementsChance)) {
                        potentialPlacements.push_back(blockpos3);
                    }
                } else if (d6 >= d3) {
                    const BlockState* state =
                        blocks.middleLayerProvider->getState(world, random, blockpos3.x, blockpos3.y, blockpos3.z);
                    safeSetBlock(world, blockpos3, state, cannotReplace);
                } else if (d6 >= d4) {
                    const BlockState* state =
                        blocks.outerLayerProvider->getState(world, random, blockpos3.x, blockpos3.y, blockpos3.z);
                    safeSetBlock(world, blockpos3, state, cannotReplace);
                }
            }
        }
    }

    // inner_placements：在 potential 候选点的六向寻找可附着面（canClusterGrowAtState）。
    const std::vector<const BlockState*>& innerPlacements = blocks.innerPlacements;
    if (!innerPlacements.empty()) {
        for (const BlockPos& blockpos4 : potentialPlacements) {
            const i32 idx = random.nextInt(static_cast<i32>(innerPlacements.size()));
            BlockState blockstate1(*innerPlacements[static_cast<size_t>(idx)]);
            for (Direction direction : Directions::all()) {
                if (blockstate1.hasProperty(BlockStateProperties::FACING())) {
                    blockstate1 = blockstate1.with(BlockStateProperties::FACING(), direction);
                }
                const BlockPos blockpos5 = blockpos4.offset(direction);
                const BlockState* blockstate2 = world.getBlockState(blockpos5);
                if (blockstate1.hasProperty(BlockStateProperties::WATERLOGGED())) {
                    const fluid::FluidState* fs = (blockstate2 != nullptr) ? blockstate2->getFluidState() : nullptr;
                    const bool waterlogged = (fs != nullptr && fs->isSource());
                    blockstate1 = blockstate1.with(BlockStateProperties::WATERLOGGED(), waterlogged);
                }
                if (canClusterGrowAtState(blockstate2)) {
                    safeSetBlock(world, blockpos5, &blockstate1, cannotReplace);
                    break;
                }
            }
        }
    }

    return true;
}

bool GeodeFeature::safeSetBlock(
    IWorld& world, const BlockPos& pos, const BlockState* state, const BlockTag* cannotReplace) const
{
    if (state == nullptr) {
        // MC safeSetBlock 传 AIR 时 state 非空；此处 nullptr 表示显式凿空气。
        const BlockState* existing = world.getBlockState(pos);
        if (cannotReplace != nullptr && existing != nullptr && cannotReplace->contains(*existing)) {
            return false;
        }
        return world.setBlockState(pos, VanillaBlocks::getState(VanillaBlocks::AIR));
    }
    const BlockState* existing = world.getBlockState(pos);
    if (cannotReplace != nullptr && existing != nullptr && cannotReplace->contains(*existing)) {
        return false;
    }
    return world.setBlockState(pos, state);
}

} // namespace mc::world::gen::feature::cave
