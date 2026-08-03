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

#include "SugarCaneBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

SugarCaneBlock::SugarCaneBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_15())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_15(), 0));

    // 甘蔗形状：稍小的方块
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

i32 SugarCaneBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_15());
}

const BlockState& SugarCaneBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_15(), std::min(age, 15));
}

BlockState SugarCaneBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool SugarCaneBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 甘蔗可以放置在甘蔗上
    if (belowState->is(this)) {
        return true;
    }

    const bool validGround = (VanillaBlocks::GRASS_BLOCK != nullptr && belowState->is(VanillaBlocks::GRASS_BLOCK)) ||
        (VanillaBlocks::DIRT != nullptr && belowState->is(VanillaBlocks::DIRT)) ||
        (VanillaBlocks::SAND != nullptr && belowState->is(VanillaBlocks::SAND)) ||
        (VanillaBlocks::RED_SAND != nullptr && belowState->is(VanillaBlocks::RED_SAND));

    return validGround && _isNearWater(world, pos);
}

bool SugarCaneBlock::_isNearWater(IBlockReader& world, const BlockPos& pos) const
{
    // 检查根部同高度四个方向是否有水
    const i32 waterY = pos.y - 1;
    for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
        BlockPos adjPos(pos.x + Directions::xOffset(dir), waterY, pos.z + Directions::zOffset(dir));
        const BlockState* adjState = world.getBlockState(adjPos);

        if (adjState != nullptr && adjState->getMaterial() == Material::WATER) {
            return true;
        }
    }
    return false;
}

BlockState SugarCaneBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查下方支撑
    if (facing == Direction::Down) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

void SugarCaneBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return;
    }

    // 检查高度限制（最高3格）
    i32 height = 1;
    for (i32 i = 1; i < 3; ++i) {
        BlockPos checkPos(pos.x, pos.y - i, pos.z);
        const BlockState* checkState = world.getBlockState(checkPos);
        if (checkState == nullptr || !checkState->is(this)) {
            break;
        }
        height++;
    }

    if (height >= 3) {
        return; // 已达到最高高度
    }

    // 随机生长
    if (random.nextInt(16) == 0) {
        i32 age = getAge(state);
        if (age >= 15) {
            // 生长新的甘蔗
            world.setBlockState(abovePos, &defaultState(), 2);
            world.setBlockState(pos, &withAge(0), 2);
        } else {
            // 增加年龄
            world.setBlockState(pos, &withAge(age + 1), 2);
        }
    }
}

const CollisionShape& SugarCaneBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& SugarCaneBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

PlantType SugarCaneBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Beach;
}

const BlockState& SugarCaneBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

} // namespace blocks
} // namespace mc
