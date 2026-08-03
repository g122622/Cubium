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

#include "NetherPortalBlock.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

NetherPortalBlock::NetherPortalBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_AXIS())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::X));

    // 创建形状
    m_xAxisShape = CollisionShape::box(0.0f, 0.0f, 0.375f, 1.0f, 1.0f, 0.625f);
    m_zAxisShape = CollisionShape::box(0.375f, 0.0f, 0.0f, 0.625f, 1.0f, 1.0f);
}

Axis NetherPortalBlock::getAxis(const BlockState& state) const
{
    return state.get(BlockStateProperties::HORIZONTAL_AXIS());
}

BlockState NetherPortalBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.horizontalDirection();
    Axis axis = Directions::getAxis(facing);
    if (axis == Axis::Y) axis = Axis::X; // 水平轴
    return defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), axis);
}

bool NetherPortalBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 当邻居方块更新时，会调用 PortalSize 验证传送门是否仍然有效。
    // isValidPosition 用于检查当前位置是否可以作为传送门的一部分。

    // 获取传送门轴向
    Axis axis = getAxis(state);

    // 根据轴向确定检查方向
    // X 轴传送门：检查东西方向（宽度方向）和上下方向
    // Z 轴传送门：检查南北方向（宽度方向）和上下方向
    Direction widthDir = (axis == Axis::X) ? Direction::East : Direction::South;
    Direction depthDir = (axis == Axis::X) ? Direction::South : Direction::East;

    // 检查六个方向：上、下、宽度方向两侧、深度方向两侧
    // 传送门方块需要与传送门方块或框架方块相邻才能存在

    // 检查上下方向
    const BlockState* upState = world.getBlockState(pos.up());
    const BlockState* downState = world.getBlockState(pos.down());
    if ((upState != nullptr && _isConnectedToPortal(*upState)) ||
        (downState != nullptr && _isConnectedToPortal(*downState))) {
        return true;
    }

    // 检查宽度方向（X轴传送门检查东西，Z轴传送门检查南北）
    const BlockState* widthPosState = world.getBlockState(pos.offset(widthDir));
    const BlockState* widthNegState = world.getBlockState(pos.offset(Directions::opposite(widthDir)));
    if ((widthPosState != nullptr && _isConnectedToPortal(*widthPosState)) ||
        (widthNegState != nullptr && _isConnectedToPortal(*widthNegState))) {
        return true;
    }

    // 检查深度方向（X轴传送门检查南北，Z轴传送门检查东西）
    // 深度方向应该是框架方块
    const BlockState* depthPosState = world.getBlockState(pos.offset(depthDir));
    const BlockState* depthNegState = world.getBlockState(pos.offset(Directions::opposite(depthDir)));
    if ((depthPosState != nullptr && _isConnectedToPortal(*depthPosState)) ||
        (depthNegState != nullptr && _isConnectedToPortal(*depthNegState))) {
        return true;
    }

    return false;
}

bool NetherPortalBlock::_isConnectedToPortal(const BlockState& state) const
{
    if (state.isAir()) {
        return false;
    }

    // 检查是否是传送门方块
    if (state.is(this)) {
        return true;
    }

    // 检查是否是框架方块（黑曜石）
    if (VanillaBlocks::OBSIDIAN != nullptr && state.is(VanillaBlocks::OBSIDIAN)) {
        return true;
    }

    return false;
}

BlockState NetherPortalBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查传送门是否仍然有效
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!isValidPosition(state, blockReader, currentPos)) {
        if (auto* airState = BlockRegistry::instance().airState()) {
            return *airState;
        }
    }

    return state;
}

void NetherPortalBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 实体进入传送门后开始传送计时
    // 玩家需要站立在传送门中约 4 秒（80 ticks）才能传送
    // 其他实体约 1 tick

    MC_UNUSED(state);
    MC_UNUSED(world);

    // 检查实体是否是乘客或被骑乘
    if (entity.isRiding() || entity.hasPassengers()) {
        return;
    }

    // Boss 不能使用传送门（末影龙、凋灵等）
    if (!entity.isNonBoss()) {
        return;
    }

    // 检查传送冷却
    if (!entity.canTeleport()) {
        return;
    }

    // 设置实体在传送门中
    entity.setInPortal(true);
    // 记录传送门位置
    entity.setPortalPos(pos);

    // 注意：传送逻辑由 Entity::tickPortal() 处理
    // 玩家的 getMaxInPortalTime() 返回 80 ticks
    // 其他实体的 getMaxInPortalTime() 返回 1 tick
}

const CollisionShape& NetherPortalBlock::getShape(const BlockState& state) const
{
    Axis axis = getAxis(state);
    return (axis == Axis::X) ? m_xAxisShape : m_zAxisShape;
}

const CollisionShape& NetherPortalBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
