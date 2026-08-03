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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "MossBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

MossBlock::MossBlock(const BlockProperties& properties)
    : Block(properties)
{}

bool MossBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);
    return true;
}

bool MossBlock::canUseBonemeal(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    return true;
}

void MossBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    auto& mossReplaceable = BlockTags::MOSS_REPLACEABLE();

    // 在以骨粉位置为中心、上方1格下方5格的3x7x3范围内传播苔藓
    // MC源码：扫描范围 pos + (-1, -1, -1) 到 pos + (1, 5, 1)
    for (i32 dy = -1; dy <= 5; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                BlockPos targetPos(pos.x + dx, pos.y + dy, pos.z + dz);
                const BlockState* targetState = world.getBlockState(targetPos);
                if (targetState == nullptr) {
                    continue;
                }

                // 检查目标方块是否在MOSS_REPLACEABLE标签中
                if (!mossReplaceable.contains(*targetState)) {
                    continue;
                }

                // 检查目标上方是否有空气（苔藓块上方需要可用的空间）
                BlockPos abovePos(targetPos.x, targetPos.y + 1, targetPos.z);
                const BlockState* aboveState = world.getBlockState(abovePos);
                if (aboveState == nullptr || !aboveState->isAir()) {
                    continue;
                }

                // 替换为苔藓块
                const BlockState& mossState = VanillaBlocks::MOSS_BLOCK->defaultState();
                world.setBlockState(targetPos, &mossState, 3);

                // 在苔藓块上方的空气位置放置植被
                _placeMossVegetation(world, random, abovePos);
            }
        }
    }
}

void MossBlock::_placeMossVegetation(IWorld& world, math::IRandom& random, const BlockPos& pos)
{
    // 50%概率放置苔藓地毯
    if (random.nextInt(4) == 0) {
        const BlockState& carpetState = VanillaBlocks::MOSS_CARPET->defaultState();
        world.setBlockState(pos, &carpetState, 3);
        return;
    }

    // 杜鹃花丛概率较低
    if (random.nextInt(10) == 0) {
        // 2/3概率普通杜鹃，1/3概率开花杜鹃
        if (random.nextInt(3) < 2) {
            const BlockState& azaleaState = VanillaBlocks::AZALEA->defaultState();
            world.setBlockState(pos, &azaleaState, 3);
        } else {
            const BlockState& floweringState = VanillaBlocks::FLOWERING_AZALEA->defaultState();
            world.setBlockState(pos, &floweringState, 3);
        }
    }
}

} // namespace blocks
} // namespace mc
