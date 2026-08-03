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

#include "CocoaBlock.hpp"
#include "../../../../core/Constants.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../BlockTags.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/HorizontalBlock.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

// 将方向转换为索引 (North=0, South=1, West=2, East=3)
[[nodiscard]] i32 directionToIndex(Direction facing)
{
    switch (facing) {
        case Direction::North:
            return 0;
        case Direction::South:
            return 1;
        case Direction::West:
            return 2;
        case Direction::East:
            return 3;
        default:
            return 0; // 不应该发生
    }
}

} // namespace

CocoaBlock::CocoaBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    // 创建状态容器，添加 AGE 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).add(AGE()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(FACING(), Direction::North).with(AGE(), 0));

    // 初始化形状
    _initShapes();
}

i32 CocoaBlock::getAge(const BlockState& state) const
{
    return state.get(AGE());
}

const BlockState& CocoaBlock::withAge(const BlockState& state, i32 age) const
{
    return state.with(AGE(), std::clamp(age, 0, getMaxAge()));
}

bool CocoaBlock::isMaxAge(const BlockState& state) const
{
    return getAge(state) >= getMaxAge();
}

BlockState CocoaBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 遍历玩家朝向的各个方向，找到第一个可以附着的方向
    for (Direction direction : context.getNearestLookingDirections()) {
        if (Directions::isHorizontal(direction)) {
            BlockPos attachPos = context.placementPos().offset(direction);
            const BlockState* attachState = context.getWorld().getBlockState(attachPos);

            if (attachState != nullptr && BlockTags::JUNGLE_LOGS().contains(*attachState)) {
                // 返回面向丛林原木的状态（FACING 指向丛林原木）
                return defaultState().with(FACING(), direction);
            }
        }
    }

    // 没有找到有效的附着位置
    return defaultState();
}

bool CocoaBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    Direction facing = state.get(FACING());
    return _canAttachTo(world, pos, facing);
}

BlockState CocoaBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 如果更新方向是可可豆附着方向，检查是否仍然有效
    Direction attachDir = state.get(FACING());
    if (facing == attachDir) {
        // IWorld 继承自 IBlockReader，可以安全转换
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!_canAttachTo(blockReader, currentPos, attachDir)) {
            // 附着方块被移除，变成空气
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

void CocoaBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 只有未成熟的才生长
    i32 age = getAge(state);
    if (age >= getMaxAge()) {
        return;
    }

    // 检查光照：需要上方光照等级 >= CROP_GROWTH_LIGHT_THRESHOLD
    const BlockPos abovePos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(abovePos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(abovePos));
    if (std::max(blockLight, skyLight) < game::CROP_GROWTH_LIGHT_THRESHOLD) {
        return;
    }

    // 1/5 概率生长
    if (random.nextInt(5) == 0) {
        const BlockState& newState = withAge(state, age + 1);
        world.setBlockState(pos, &newState, 2);
    }
}

bool CocoaBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);

    // 未成熟时可以生长
    return !isMaxAge(state);
}

bool CocoaBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);

    // 骨粉总是有效（如果未成熟）
    return true;
}

void CocoaBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);

    // 骨粉使可可豆增加一个生长阶段
    i32 age = getAge(state);
    if (age < getMaxAge()) {
        const BlockState& newState = withAge(state, age + 1);
        world.setBlockState(pos, &newState, 2);
    }
}

const CollisionShape& CocoaBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(FACING());
    i32 age = getAge(state);

    i32 dirIndex = directionToIndex(facing);
    return m_shapesByDirectionAndAge[dirIndex][age];
}

bool CocoaBlock::allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 可可豆不阻挡实体移动
    return true;
}

void CocoaBlock::_initShapes()
{
    // 形状基于像素坐标 (0-16)

    // 朝东 - 从丛林原木向东延伸
    // AGE 0: 11,7,6 到 15,12,10 (4x5x4)
    // AGE 1: 9,5,5 到 15,12,11 (6x7x6)
    // AGE 2: 7,3,4 到 15,12,12 (8x9x8)
    m_shapesByDirectionAndAge[3][0] =
        CollisionShape::box(11.0f / 16.0f, 7.0f / 16.0f, 6.0f / 16.0f, 15.0f / 16.0f, 12.0f / 16.0f, 10.0f / 16.0f);
    m_shapesByDirectionAndAge[3][1] =
        CollisionShape::box(9.0f / 16.0f, 5.0f / 16.0f, 5.0f / 16.0f, 15.0f / 16.0f, 12.0f / 16.0f, 11.0f / 16.0f);
    m_shapesByDirectionAndAge[3][2] =
        CollisionShape::box(7.0f / 16.0f, 3.0f / 16.0f, 4.0f / 16.0f, 15.0f / 16.0f, 12.0f / 16.0f, 12.0f / 16.0f);

    // 朝西 - 从丛林原木向西延伸
    // AGE 0: 1,7,6 到 5,12,10
    // AGE 1: 1,5,5 到 7,12,11
    // AGE 2: 1,3,4 到 9,12,12
    m_shapesByDirectionAndAge[2][0] =
        CollisionShape::box(1.0f / 16.0f, 7.0f / 16.0f, 6.0f / 16.0f, 5.0f / 16.0f, 12.0f / 16.0f, 10.0f / 16.0f);
    m_shapesByDirectionAndAge[2][1] =
        CollisionShape::box(1.0f / 16.0f, 5.0f / 16.0f, 5.0f / 16.0f, 7.0f / 16.0f, 12.0f / 16.0f, 11.0f / 16.0f);
    m_shapesByDirectionAndAge[2][2] =
        CollisionShape::box(1.0f / 16.0f, 3.0f / 16.0f, 4.0f / 16.0f, 9.0f / 16.0f, 12.0f / 16.0f, 12.0f / 16.0f);

    // 朝北 - 从丛林原木向北延伸
    // AGE 0: 6,7,1 到 10,12,5
    // AGE 1: 5,5,1 到 11,12,7
    // AGE 2: 4,3,1 到 12,12,9
    m_shapesByDirectionAndAge[0][0] =
        CollisionShape::box(6.0f / 16.0f, 7.0f / 16.0f, 1.0f / 16.0f, 10.0f / 16.0f, 12.0f / 16.0f, 5.0f / 16.0f);
    m_shapesByDirectionAndAge[0][1] =
        CollisionShape::box(5.0f / 16.0f, 5.0f / 16.0f, 1.0f / 16.0f, 11.0f / 16.0f, 12.0f / 16.0f, 7.0f / 16.0f);
    m_shapesByDirectionAndAge[0][2] =
        CollisionShape::box(4.0f / 16.0f, 3.0f / 16.0f, 1.0f / 16.0f, 12.0f / 16.0f, 12.0f / 16.0f, 9.0f / 16.0f);

    // 朝南 - 从丛林原木向南延伸
    // AGE 0: 6,7,11 到 10,12,15
    // AGE 1: 5,5,9 到 11,12,15
    // AGE 2: 4,3,7 到 12,12,15
    m_shapesByDirectionAndAge[1][0] =
        CollisionShape::box(6.0f / 16.0f, 7.0f / 16.0f, 11.0f / 16.0f, 10.0f / 16.0f, 12.0f / 16.0f, 15.0f / 16.0f);
    m_shapesByDirectionAndAge[1][1] =
        CollisionShape::box(5.0f / 16.0f, 5.0f / 16.0f, 9.0f / 16.0f, 11.0f / 16.0f, 12.0f / 16.0f, 15.0f / 16.0f);
    m_shapesByDirectionAndAge[1][2] =
        CollisionShape::box(4.0f / 16.0f, 3.0f / 16.0f, 7.0f / 16.0f, 12.0f / 16.0f, 12.0f / 16.0f, 15.0f / 16.0f);
}

bool CocoaBlock::_canAttachTo(IBlockReader& world, const BlockPos& pos, Direction facing) const
{
    // 检查 FACING 方向的方块是否为丛林原木
    BlockPos attachPos = pos.offset(facing);
    const BlockState* attachState = world.getBlockState(attachPos);

    if (attachState == nullptr) {
        return false;
    }

    return BlockTags::JUNGLE_LOGS().contains(*attachState);
}

} // namespace blocks
} // namespace mc
