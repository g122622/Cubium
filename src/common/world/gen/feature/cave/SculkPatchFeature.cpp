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

#include "SculkPatchFeature.hpp"

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/blocks/sculk/SculkBehaviour.hpp"
#include "common/world/block/blocks/sculk/SculkSpreader.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace cave {

// ============================================================================
// ConfiguredSculkPatchFeature
// ============================================================================

ConfiguredSculkPatchFeature::ConfiguredSculkPatchFeature(
    std::unique_ptr<SculkPatchConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredSculkPatchFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, generator, random, pos, *m_config);
}

// ============================================================================
// SculkPatchFeature
// ============================================================================

bool SculkPatchFeature::canSpreadFrom(IWorld& world, const BlockPos& pos)
{
    // MC SculkPatchFeature.canSpreadFrom:
    //   blockstate.getBlock() instanceof SculkBehaviour → true；
    //   否则 (!isAir && (!is(WATER) || !fluid.isSource())) ? false : 六邻任一 isCollisionShapeFullBlock。
    // 项目 nullptr 视为空气。
    const BlockState* state = world.getBlockState(pos);
    if (state != nullptr && dynamic_cast<const blocks::SculkBehaviour*>(&state->getBlock()) != nullptr) {
        return true;
    }

    const bool isAir = (state == nullptr) || state->isAir();
    bool isWaterSource = false;
    if (state != nullptr && state->is(VanillaBlocks::WATER)) {
        const fluid::FluidState* fluid = state->getFluidState();
        isWaterSource = fluid != nullptr && fluid->isSource();
    }
    if (!isAir && !isWaterSource) {
        return false;
    }

    for (Direction dir : Directions::all()) {
        const BlockPos neighbor = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighbor);
        if (neighborState == nullptr) {
            continue;
        }
        // MC isCollisionShapeFullBlock(level, pos)：碰撞形状为完整方块。
        if (neighborState->getBlock().getCollisionShape(*neighborState).isFullBlock()) {
            return true;
        }
    }
    return false;
}

bool SculkPatchFeature::place(IWorld& world,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin,
    const SculkPatchConfig& config)
{
    if (!canSpreadFrom(world, origin)) {
        return false;
    }

    blocks::SculkSpreader spreader = blocks::SculkSpreader::createWorldGenSpreader();
    const i32 rounds = config.spreadRounds + config.growthRounds;

    for (i32 j = 0; j < rounds; ++j) {
        for (i32 k = 0; k < config.chargeCount; ++k) {
            spreader.addCursors(origin, config.amountPerCharge);
        }

        // MC: 前 spreadRounds 轮 flag=true（shouldUpdateBlocks，触发 vein→sculk 转化），
        //     其后 growthRounds 轮 flag=false（仅生长 sensor/shrieker，不改方块）。
        const bool flag = j < config.spreadRounds;
        for (i32 l = 0; l < config.spreadAttempts; ++l) {
            spreader.updateCursors(world, origin, random, flag);
        }

        spreader.clear();
    }

    // MC: catalystChance 概率在中心放 SCULK_CATALYST（下方碰撞完整方块时）。
    const BlockPos below = origin.down();
    if (random.nextFloat() <= config.catalystChance) {
        const BlockState* belowState = world.getBlockState(below);
        if (belowState != nullptr && belowState->getBlock().getCollisionShape(*belowState).isFullBlock()) {
            world.setBlockState(origin, &VanillaBlocks::SCULK_CATALYST->defaultState(), 3);
        }
    }

    // MC: extraRareGrowths.sample() 次，在 origin 的 5×5（xz 各 -2..2）随机点放 CAN_SUMMON 尖叫体。
    const i32 extraGrowths = config.extraRareGrowths != nullptr ? config.extraRareGrowths->sample(random) : 0;
    for (i32 j1 = 0; j1 < extraGrowths; ++j1) {
        const i32 dx = random.nextInt(5) - 2;
        const i32 dz = random.nextInt(5) - 2;
        const BlockPos growthPos(origin.x + dx, origin.y, origin.z + dz);
        const BlockState* growthState = world.getBlockState(growthPos);
        const BlockState* belowGrowth = world.getBlockState(growthPos.down());
        // MC: growthPos 为空气 且 下方 UP 面 sturdy。
        if ((growthState == nullptr || growthState->isAir()) && belowGrowth != nullptr &&
            belowGrowth->isFaceSturdy(world, growthPos.down(), Direction::Up, SupportType::Full)) {
            const BlockState* shrieker =
                &VanillaBlocks::SCULK_SHRIEKER->defaultState().with(BlockStateProperties::CAN_SUMMON(), true);
            world.setBlockState(growthPos, shrieker, 3);
        }
    }

    return true;
}

} // namespace cave
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
