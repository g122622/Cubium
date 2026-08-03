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
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include <utility>

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
    // 判断是绯红菌岩还是诡异菌岩
    bool isCrimson = state.is(VanillaBlocks::CRIMSON_NYLIUM);
    bool isWarped = state.is(VanillaBlocks::WARPED_NYLIUM);

    if (!isCrimson && !isWarped) {
        return;
    }

    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);

    // 下界菌岩骨粉散布算法
    // 对应 MC NetherForestVegetationFeature：spreadWidth=3, spreadHeight=1
    // 每个特性执行 spreadWidth * spreadWidth = 9 次散布尝试
    // 每次尝试在 origin 附近随机偏移 [0, spreadWidth) 范围内选择位置
    // 偏移方式：nextInt(spreadWidth) - nextInt(spreadWidth)，产生三角分布

    if (isCrimson) {
        // 绯红菌岩：CRIMSON_FOREST_VEGETATION_BONEMEAL
        // 权重：绯红菌索 87, 绯红菌 11, 诡异菌 1（总计 99）
        _placeNetherVegetation(world, random, abovePos, NetherVegetationType::Crimson);
    } else {
        // 诡异菌岩：WARPED_FOREST_VEGETATION_BONEMEAL + NETHER_SPROUTS_BONEMEAL
        // 权重：诡异菌索 85, 诡异菌 13, 绯红菌索 1, 绯红菌 1（总计 100）
        _placeNetherVegetation(world, random, abovePos, NetherVegetationType::Warped);

        // NETHER_SPROUTS_BONEMEAL：在同样的散布范围内放置下界苗
        _placeNetherSprouts(world, random, abovePos);

        // TWISTING_VINES_BONEMEAL：1/8 概率在散布范围内放置缠怨藤
        if (random.nextInt(8) == 0) {
            _placeTwistingVines(world, random, abovePos);
        }
    }
}

void NyliumBlock::_placeNetherVegetation(
    IWorld& world, math::IRandom& random, const BlockPos& origin, NetherVegetationType type)
{
    // 对应 MC NetherForestVegetationFeature.place()
    // spreadWidth=3, spreadHeight=1 → 9 次散布尝试
    constexpr i32 SPREAD_WIDTH = 3;
    constexpr i32 SPREAD_HEIGHT = 1;

    for (i32 i = 0; i < SPREAD_WIDTH * SPREAD_WIDTH; ++i) {
        // 随机偏移：nextInt(spreadWidth) - nextInt(spreadWidth)
        i32 dx = random.nextInt(SPREAD_WIDTH) - random.nextInt(SPREAD_WIDTH);
        i32 dy = random.nextInt(SPREAD_HEIGHT) - random.nextInt(SPREAD_HEIGHT);
        i32 dz = random.nextInt(SPREAD_WIDTH) - random.nextInt(SPREAD_WIDTH);
        BlockPos currentPos(origin.x + dx, origin.y + dy, origin.z + dz);

        // 检查目标位置是否为空气（nullptr 视为空气，兼容测试世界）
        const BlockState* currentState = world.getBlockState(currentPos);
        if (currentState != nullptr && !currentState->isAir()) {
            continue;
        }

        // 检查目标位置下方是否为菌岩
        const BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr ||
            (!belowState->is(VanillaBlocks::CRIMSON_NYLIUM) && !belowState->is(VanillaBlocks::WARPED_NYLIUM))) {
            continue;
        }

        // 根据植被类型进行加权随机选择
        const Block* vegetationBlock = nullptr;
        if (type == NetherVegetationType::Crimson) {
            // 绯红菌岩植被权重：绯红菌索 87, 绯红菌 11, 诡异菌 1（总计 99）
            i32 choice = random.nextInt(99);
            if (choice < 87) {
                vegetationBlock = VanillaBlocks::CRIMSON_ROOTS;
            } else if (choice < 98) {
                vegetationBlock = VanillaBlocks::CRIMSON_FUNGUS;
            } else {
                vegetationBlock = VanillaBlocks::WARPED_FUNGUS;
            }
        } else {
            // 诡异菌岩植被权重：诡异菌索 85, 诡异菌 13, 绯红菌索 1, 绯红菌 1（总计 100）
            i32 choice = random.nextInt(100);
            if (choice < 85) {
                vegetationBlock = VanillaBlocks::WARPED_ROOTS;
            } else if (choice < 98) {
                vegetationBlock = VanillaBlocks::WARPED_FUNGUS;
            } else if (choice < 99) {
                vegetationBlock = VanillaBlocks::CRIMSON_ROOTS;
            } else {
                vegetationBlock = VanillaBlocks::CRIMSON_FUNGUS;
            }
        }

        if (vegetationBlock != nullptr) {
            world.setBlockState(currentPos, &vegetationBlock->defaultState(), 3);
        }
    }
}

void NyliumBlock::_placeNetherSprouts(IWorld& world, math::IRandom& random, const BlockPos& origin)
{
    // 对应 MC NETHER_SPROUTS_BONEMEAL
    // 使用相同的 NetherForestVegetationFeature，但 stateProvider 固定为下界苗
    constexpr i32 SPREAD_WIDTH = 3;
    constexpr i32 SPREAD_HEIGHT = 1;

    for (i32 i = 0; i < SPREAD_WIDTH * SPREAD_WIDTH; ++i) {
        i32 dx = random.nextInt(SPREAD_WIDTH) - random.nextInt(SPREAD_WIDTH);
        i32 dy = random.nextInt(SPREAD_HEIGHT) - random.nextInt(SPREAD_HEIGHT);
        i32 dz = random.nextInt(SPREAD_WIDTH) - random.nextInt(SPREAD_WIDTH);
        BlockPos currentPos(origin.x + dx, origin.y + dy, origin.z + dz);

        const BlockState* currentState = world.getBlockState(currentPos);
        if (currentState != nullptr && !currentState->isAir()) {
            continue;
        }

        const BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr ||
            (!belowState->is(VanillaBlocks::CRIMSON_NYLIUM) && !belowState->is(VanillaBlocks::WARPED_NYLIUM))) {
            continue;
        }

        if (VanillaBlocks::NETHER_SPROUTS != nullptr) {
            world.setBlockState(currentPos, &VanillaBlocks::NETHER_SPROUTS->defaultState(), 3);
        }
    }
}

void NyliumBlock::_placeTwistingVines(IWorld& world, math::IRandom& random, const BlockPos& origin)
{
    // 对应 MC TWISTING_VINES_BONEMEAL (TwistingVinesFeature)
    // spreadWidth=3, spreadHeight=1, maxHeight=2
    // 在散布范围内寻找合适位置，向上放置 1-2 格高的缠怨藤柱
    constexpr i32 SPREAD_WIDTH = 3;
    constexpr i32 SPREAD_HEIGHT = 1;

    for (i32 i = 0; i < SPREAD_WIDTH * SPREAD_WIDTH; ++i) {
        i32 dx = random.nextInt(SPREAD_WIDTH) - random.nextInt(SPREAD_WIDTH);
        i32 dy = random.nextInt(SPREAD_HEIGHT) - random.nextInt(SPREAD_HEIGHT);
        i32 dz = random.nextInt(SPREAD_WIDTH) - random.nextInt(SPREAD_WIDTH);
        BlockPos currentPos(origin.x + dx, origin.y + dy, origin.z + dz);

        // 缠怨藤需要从地面开始向上生长，找到第一个空气位置
        // 对应 MC: 找到地面上方第一个空气方块
        // 地面方块需要是下界岩、诡异菌岩或诡异疣块
        const BlockState* groundState = world.getBlockState(currentPos);
        if (groundState != nullptr && !groundState->isAir()) {
            continue;
        }

        const BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr ||
            (!belowState->is(VanillaBlocks::NETHERRACK) && !belowState->is(VanillaBlocks::WARPED_NYLIUM) &&
                !belowState->is(VanillaBlocks::WARPED_WART_BLOCK))) {
            continue;
        }

        if (VanillaBlocks::TWISTING_VINES == nullptr || VanillaBlocks::TWISTING_VINES_PLANT == nullptr) {
            continue;
        }

        // 确定藤蔓高度：1~2，1/6 概率翻倍，1/5 概率强制为 1
        i32 vineHeight = random.nextInt(1, 2); // [1, 2]
        if (random.nextInt(6) == 0) {
            vineHeight *= 2;
        }
        if (random.nextInt(5) == 0) {
            vineHeight = 1;
        }

        // 放置藤蔓柱：底部是 TWISTING_VINES_PLANT，顶部是 TWISTING_VINES（头部）
        for (i32 h = 0; h < vineHeight; ++h) {
            BlockPos vinePos(currentPos.x, currentPos.y + h, currentPos.z);
            const BlockState* vinePosState = world.getBlockState(vinePos);
            if (vinePosState != nullptr && !vinePosState->isAir()) {
                break;
            }

            if (h < vineHeight - 1) {
                // 藤蔓身体
                world.setBlockState(vinePos, &VanillaBlocks::TWISTING_VINES_PLANT->defaultState(), 3);
            } else {
                // 藤蔓头部（带 AGE 属性）
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
