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

#include "TrappedChestBlock.hpp"

#include "common/world/IWorld.hpp"
#include "common/world/blockentity/storage/TrappedChestEntity.hpp"

namespace mc {
namespace blocks {

TrappedChestBlock::TrappedChestBlock(const BlockProperties& properties)
    : ChestBlock(properties)
{}

i32 TrappedChestBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::TrappedChest) {
        return 0;
    }

    auto* chest = static_cast<blockentity::TrappedChestEntity*>(blockEntity);

    // TODO: 需要转换为World来获取完整的红石信号计算（支持双箱）
    return chest->getOpenCount();
}

i32 TrappedChestBlock::getStrongPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 仅从顶面输出强充能
    if (side != Direction::Up) {
        return 0;
    }
    return getWeakPower(state, world, pos, side);
}

} // namespace blocks
} // namespace mc
