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

#include "common/world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/dirt/SnowyDirtBlock.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/capability/IBlockAnimateContext.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/lighting/LightEngineUtils.hpp"
#include <algorithm>
#include <optional>
#include <utility>

namespace mc {
namespace blocks {

// ============================================================================
// SpreadableSnowyDirtBlock 实现
// ============================================================================

SpreadableSnowyDirtBlock::SpreadableSnowyDirtBlock(BlockProperties properties)
    : SnowyDirtBlock(std::move(properties))
{
    // SNOWY 属性、默认状态、放置/邻居更新同步由基类 SnowyDirtBlock 负责，
    // 此处仅保留蔓延/退化所需的状态。
}

void SpreadableSnowyDirtBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查是否满足蔓延条件
    if (!isSnowyConditions(world, pos, state)) {
        // 不满足条件，退化成泥土
        world.setBlockState(pos, &VanillaBlocks::DIRT->defaultState());
    } else {
        // 满足条件，尝试向周围蔓延
        // 需要 pos.up() 的光照 >= 9
        const u8 skyLight = world.getSkyLight(pos.x, pos.y + 1, pos.z);
        const u8 blockLight = world.getBlockLight(pos.x, pos.y + 1, pos.z);
        const u8 lightLevel = std::max(skyLight, blockLight);

        if (lightLevel >= game::GRASS_SPREAD_LIGHT_THRESHOLD) {
            const BlockState* defaultState = &getDefaultState();

            // 尝试向4个随机位置的泥土蔓延
            for (i32 i = 0; i < 4; ++i) {
                const i32 dx = random.nextInt(3) - 1; // -1, 0, 1
                const i32 dy = random.nextInt(5) - 3; // -3, -2, -1, 0, 1
                const i32 dz = random.nextInt(3) - 1; // -1, 0, 1

                const BlockPos targetPos(pos.x + dx, pos.y + dy, pos.z + dz);

                // 检查目标位置是否为泥土
                const BlockState* targetState = world.getBlockState(targetPos);
                if (targetState == nullptr || targetState->blockId() != VanillaBlocks::DIRT->blockId()) {
                    continue;
                }

                // 检查目标位置是否满足蔓延条件
                if (isSnowyAndNotUnderwater(world, targetPos, *defaultState)) {
                    // 检查目标位置上方是否有雪
                    // 蔓延时只检查 SNOW（雪层），不检查 SNOW_BLOCK（雪块）
                    const BlockPos abovePos(targetPos.x, targetPos.y + 1, targetPos.z);
                    const BlockState* aboveState = world.getBlockState(abovePos);
                    const bool hasSnow = aboveState != nullptr && aboveState->is(VanillaBlocks::SNOW);

                    // 设置新方块状态，包含 SNOWY 属性
                    const BlockState* newState = &defaultState->with(SNOWY(), hasSnow);
                    world.setBlockState(targetPos, newState);
                }
            }
        }
    }
}

bool SpreadableSnowyDirtBlock::isSnowyConditions(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    const BlockPos abovePos = pos.up();
    const BlockState* aboveState = world.getBlockState(abovePos);

    // 检查是否为雪层且层数为1
    // 只有1层雪时满足条件
    if (aboveState != nullptr && aboveState->is(VanillaBlocks::SNOW)) {
        // 检查 LAYERS 属性是否为 1
        // 使用 getOptional 安全获取，因为 SNOWY 状态会在这里被检查
        const std::optional<i32> layers = aboveState->getOptional(SnowBlock::LAYERS());
        if (layers.has_value() && layers.value() == 1) {
            return true;
        }
        // 多层雪会进入下面的光照检查逻辑
    }

    // 检查上方是否有完整水源
    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->getLevel() == fluid::SOURCE_LEVEL) {
            return false; // 上方有完整水源，不满足条件
        }
    }

    // 结合上方方块的不透明度与面遮挡形状，判断是否达到满阻挡
    static const BlockState* s_airState = &VanillaBlocks::AIR->defaultState();
    const BlockState& resolvedAboveState = aboveState != nullptr ? *aboveState : *s_airState;
    const i32 lightBlockInto = LightEngineUtils::getLightBlockInto(
        world, state, pos, resolvedAboveState, abovePos, Direction::Up, resolvedAboveState.getOpacity());
    return lightBlockInto < game::MAX_LIGHT_LEVEL;
}

bool SpreadableSnowyDirtBlock::isSnowyAndNotUnderwater(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    if (!isSnowyConditions(world, pos, state)) {
        return false;
    }

    // 检查上方是否有水
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr) {
        return true;
    }

    const fluid::FluidState* fluidState = aboveState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        return false; // 上方有流体
    }

    return true;
}

} // namespace blocks
} // namespace mc
