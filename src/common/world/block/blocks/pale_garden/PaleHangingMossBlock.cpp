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

#include "PaleHangingMossBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

PaleHangingMossBlock::PaleHangingMossBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::TIP())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::TIP(), false));

    // 创建形状
    // 非末端：box(2, 0, 2, 14, 16, 14)
    m_bodyShape = CollisionShape::box(2.0f, 0.0f, 2.0f, 14.0f, 16.0f, 14.0f);
    // 末端：box(2, 0, 2, 14, 10, 14)
    m_tipShape = CollisionShape::box(2.0f, 0.0f, 2.0f, 14.0f, 10.0f, 14.0f);
}

bool PaleHangingMossBlock::isTip(const BlockState& state) const noexcept
{
    return state.get(BlockStateProperties::TIP());
}

BlockState PaleHangingMossBlock::withTip(bool tip) const
{
    return defaultState().with(BlockStateProperties::TIP(), tip);
}

bool PaleHangingMossBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 上方方块必须提供向下的实心面，或者上方方块本身是苍白垂苔（允许垂苔链）
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr) {
        return false;
    }

    // 上方方块有向下的实心面（如苍白橡木原木、石头等任何固体方块）
    if (aboveState->isSolidSide(world, abovePos, Direction::Down)) {
        return true;
    }

    // 上方方块本身是苍白垂苔，允许垂苔链式悬挂
    if (dynamic_cast<const PaleHangingMossBlock*>(&aboveState->getBlock()) != nullptr) {
        return true;
    }

    return false;
}

BlockState PaleHangingMossBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 当上方支撑方块变化时，如果不再满足存活条件则调度tick以销毁方块
    if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        world.tickManager().scheduleBlockTick(currentPos, *this, 1, world::tick::TickPriority::Normal);
    }

    // 更新TIP属性：如果下方不是苍白垂苔，则为末端
    BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    bool isTip =
        (belowState == nullptr || dynamic_cast<const PaleHangingMossBlock*>(&belowState->getBlock()) == nullptr);

    return state.with(BlockStateProperties::TIP(), isTip);
}

void PaleHangingMossBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 检查是否仍然满足存活条件
    if (!isValidPosition(state, static_cast<IBlockReader&>(world), pos)) {
        // 不满足存活条件，替换为空气并掉落物品
        const BlockState* airState = VanillaBlocks::AIR ? &VanillaBlocks::AIR->defaultState() : nullptr;
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 3);
        }
    }
}

const CollisionShape& PaleHangingMossBlock::getShape(const BlockState& state) const
{
    if (isTip(state)) {
        return m_tipShape;
    }
    return m_bodyShape;
}

const CollisionShape& PaleHangingMossBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

void PaleHangingMossBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

} // namespace blocks
} // namespace mc
