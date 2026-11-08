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
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/base/types/PlantType.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_3(), 0));

    // 原版各生长阶段都是整格宽，只是高度不同：5/16、8/16、11/16、14/16。
    // 此前写成逐渐收缩的锥形，方向完全错了。
    m_shapesByAge[0] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.3125f, 1.0f);
    m_shapesByAge[1] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    m_shapesByAge[2] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.6875f, 1.0f);
    m_shapesByAge[3] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.875f, 1.0f);
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
            world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);
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
    // 原版下界疣**没有**碰撞形状。实测（1.21.11）：
    //
    //   shape     = AABB[0.0, 0.0, 0.0] -> [1.0, 0.3125, 1.0]   非空
    //   collision = EMPTY
    //   support   = EMPTY
    //
    // 即 `getShape` 与 `getCollisionShape` 在原版是**不同**的：前者给出可见轮廓，
    // 后者为空。上一条注释曾据此推断「碰撞等于 getShape」，与本能力的实测结果矛盾
    // （`nether_wart|DOWN|CENTER` 原版 false / Cubium true）。
    MC_UNUSED(state);
    static const CollisionShape emptyShape = CollisionShape::empty();
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

bool NetherWartBlock::isRandomlyTicking(const BlockState& state) const noexcept
{
    // 下界疣 age < 3 才生长。
    return getAge(state) < 3;
}

} // namespace blocks
} // namespace mc
