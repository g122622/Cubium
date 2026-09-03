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

#include "NetherrackBlock.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc::blocks {

NetherrackBlock::NetherrackBlock(BlockProperties properties)
    : Block(std::move(properties))
{}

bool NetherrackBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    // 条件1：上方方块传播天空光（对齐 MC: above().propagatesSkylightDown()）
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr || !aboveState->propagatesSkylightDown()) {
        return false;
    }

    // 条件2：周围 3×3×3 范围内存在菌岩（对齐 MC: BlockPos.betweenClosed(pos+(-1,-1,-1), pos+(1,1,1))）
    bool foundCrimson = false;
    bool foundWarped = false;
    pos.forEachInCube(1, 1, 1, [&](const BlockPos& checkPos) -> bool {
        const BlockState* checkState = world.getBlockState(checkPos);
        if (checkState != nullptr) {
            if (checkState->is(VanillaBlocks::CRIMSON_NYLIUM)) {
                foundCrimson = true;
            } else if (checkState->is(VanillaBlocks::WARPED_NYLIUM)) {
                foundWarped = true;
            }
        }
        // 两种都找到时提前终止
        return !(foundCrimson && foundWarped);
    });

    return foundCrimson || foundWarped;
}

bool NetherrackBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 下界岩骨粉总是有效（只要 canGrow 返回 true）
    return true;
}

void NetherrackBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 遍历周围 3×3×3 范围，统计绯红菌岩和诡异菌岩的存在情况
    // 对齐 MC NetherrackBlock.performBonemeal
    bool foundCrimson = false;
    bool foundWarped = false;
    pos.forEachInCube(1, 1, 1, [&](const BlockPos& checkPos) -> bool {
        const BlockState* checkState = world.getBlockState(checkPos);
        if (checkState != nullptr) {
            if (checkState->is(VanillaBlocks::CRIMSON_NYLIUM)) {
                foundCrimson = true;
            } else if (checkState->is(VanillaBlocks::WARPED_NYLIUM)) {
                foundWarped = true;
            }
        }
        return !(foundCrimson && foundWarped);
    });

    // 根据周围菌岩类型转化下界岩
    // 对齐 MC: flag1 && flag → 随机选一种；仅 flag1 → 诡异；仅 flag → 绯红
    const BlockState* newState = nullptr;
    if (foundCrimson && foundWarped) {
        // 两种菌岩都有 → 随机选一种
        newState = random.nextBoolean() ? &VanillaBlocks::WARPED_NYLIUM->defaultState()
                                        : &VanillaBlocks::CRIMSON_NYLIUM->defaultState();
    } else if (foundWarped) {
        newState = &VanillaBlocks::WARPED_NYLIUM->defaultState();
    } else if (foundCrimson) {
        newState = &VanillaBlocks::CRIMSON_NYLIUM->defaultState();
    }

    if (newState != nullptr) {
        world.setBlockState(pos, newState, 3);
    }
}

} // namespace mc::blocks
