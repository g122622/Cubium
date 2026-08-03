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

#include "NetherWartBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../IWorld.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

NetherWartBlock::NetherWartBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_3())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_3(), 0));

    // 创建各年龄的形状
    m_shapesByAge[0] = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.25f, 0.75f);
    m_shapesByAge[1] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.8125f, 0.375f, 0.8125f);
    m_shapesByAge[2] = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
    m_shapesByAge[3] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.625f, 0.9375f);
}

i32 NetherWartBlock::getAge(const BlockState& state) const noexcept
{
    return state.get(BlockStateProperties::AGE_0_3());
}

BlockState NetherWartBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_3(), std::min(age, 3));
}

BlockState NetherWartBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool NetherWartBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方是否为灵魂沙
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 检查是否为灵魂沙
    return belowState->is(VanillaBlocks::SOUL_SAND) || belowState->is(VanillaBlocks::SOUL_SOIL);
}

void NetherWartBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    i32 age = getAge(state);

    if (age < getMaxAge()) {
        // 随机生长
        if (random.nextInt(10) == 0) {
            BlockState newState = withAge(age + 1);
            world.setBlockState(pos, &newState, 2);
        }
    }
}

const CollisionShape& NetherWartBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    return m_shapesByAge[std::min(age, 3)];
}

const CollisionShape& NetherWartBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

PlantType NetherWartBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Nether;
}

const BlockState& NetherWartBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

} // namespace blocks
} // namespace mc
