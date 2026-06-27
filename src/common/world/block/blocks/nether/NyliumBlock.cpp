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

#include "NyliumBlock.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"

namespace mc::blocks {

// ============================================================================
// NyliumBlock 实现
// ============================================================================

NyliumBlock::NyliumBlock(BlockProperties properties)
    : Block(std::move(properties))
{}

void NyliumBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{

    MC_UNUSED(random); // 退化不需要随机数

    // 如果位置不够暗，退化为下界岩
    if (!_isDarkEnough(world, pos, state)) {
        world.setBlockState(pos, &VanillaBlocks::NETHERRACK->defaultState());
    }
}

bool NyliumBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);
    // 菌岩上方需要有空气才能使用骨粉
    // 参考: net.minecraft.world.level.block.NyliumBlock.isValidBonemealTarget
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    return aboveState != nullptr && aboveState->isAir();
}

bool NyliumBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 菌岩骨粉总是有效（只要 canGrow 返回 true）
    return true;
}

void NyliumBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);
    // 在菌岩上方生成下界植物
    // 参考: net.minecraft.world.level.block.NyliumBlock.performBonemeal
    //
    // MC 原版根据菌岩类型放置不同的 ConfiguredFeature:
    // - 绯红菌岩: CRIMSON_FOREST_VEGETATION_BONEMEAL
    // - 诡异菌岩: WARPED_FOREST_VEGETATION_BONEMEAL + NETHER_SPROUTS_BONEMEAL + 1/8概率TWISTING_VINES_BONEMEAL
    //
    // TODO: 完整实现需要 NetherFeatures/PlacedFeature 系统，当前为简化版本。

    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr || !aboveState->isAir()) {
        return;
    }

    // 判断是绯红菌岩还是诡异菌岩
    bool isCrimson = state.is(VanillaBlocks::CRIMSON_NYLIUM);
    bool isWarped = state.is(VanillaBlocks::WARPED_NYLIUM);

    if (isCrimson) {
        // 绯红菌岩：生成绯红菌或绯红菌索
        // TODO: 应使用 CRIMSON_FOREST_VEGETATION_BONEMEAL 特性放置，当前简化实现
        if (VanillaBlocks::CRIMSON_FUNGUS != nullptr && random.nextInt(4) == 0) {
            world.setBlockState(abovePos, &VanillaBlocks::CRIMSON_FUNGUS->defaultState(), 3);
        } else if (VanillaBlocks::CRIMSON_ROOTS != nullptr) {
            world.setBlockState(abovePos, &VanillaBlocks::CRIMSON_ROOTS->defaultState(), 3);
        }
    } else if (isWarped) {
        // 诡异菌岩：生成诡异菌、诡异菌索和下界苗
        // TODO: 应使用 WARPED_FOREST_VEGETATION_BONEMEAL + NETHER_SPROUTS_BONEMEAL 特性放置，当前简化实现
        if (VanillaBlocks::WARPED_FUNGUS != nullptr && random.nextInt(4) == 0) {
            world.setBlockState(abovePos, &VanillaBlocks::WARPED_FUNGUS->defaultState(), 3);
        } else if (VanillaBlocks::WARPED_ROOTS != nullptr) {
            world.setBlockState(abovePos, &VanillaBlocks::WARPED_ROOTS->defaultState(), 3);
        }
        // 下界苗（Nether Sprouts）: 简化处理，与诡异菌索类似
        // 1/8 概率生成缠怨藤
        if (VanillaBlocks::TWISTING_VINES != nullptr && random.nextInt(8) == 0) {
            const BlockPos vinePos(pos.x, pos.y + 2, pos.z);
            const BlockState* vineState = world.getBlockState(vinePos);
            if (vineState != nullptr && vineState->isAir()) {
                world.setBlockState(vinePos, &VanillaBlocks::TWISTING_VINES->defaultState(), 3);
            }
        }
    }
}

bool NyliumBlock::_isDarkEnough(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    static const BlockState* s_airState = &VanillaBlocks::AIR->defaultState();
    const BlockState& resolvedAboveState = aboveState != nullptr ? *aboveState : *s_airState;
    const i32 lightBlockInto = LightEngineUtils::getLightBlockInto(
        world, state, pos, resolvedAboveState, abovePos, Direction::Up, resolvedAboveState.getOpacity());
    return lightBlockInto < game::MAX_LIGHT_LEVEL;
}

} // namespace mc::blocks
