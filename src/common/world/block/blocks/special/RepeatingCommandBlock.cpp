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

#include "RepeatingCommandBlock.hpp"

#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/special/CommandBlock.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/redstone/CommandBlockEntity.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ========== RepeatingCommandBlock ==========

RepeatingCommandBlock::RepeatingCommandBlock(const BlockProperties& properties)
    : CommandBlock(properties)
{}

void RepeatingCommandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 对齐 vanilla：循环命令方块与脉冲/连锁共用 BlockStateBase.tick（CommandBlock::tick），
    // 由方块实体持有的模式决定行为，方块类自身不拆出独立实现。
    // 循环方块通过 createBlockEntity 创建 Auto 模式的实体，因此这里不会退化为脉冲语义。
    CommandBlock::tick(world, pos, state, random);
}

std::unique_ptr<BlockEntity> RepeatingCommandBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::CommandBlockEntity>(pos, blockentity::CommandBlockMode::Auto);
}

} // namespace blocks
} // namespace mc
