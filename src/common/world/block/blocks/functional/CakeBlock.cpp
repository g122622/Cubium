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

#include "CakeBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ========== CakeBlock 实现 ==========

CakeBlock::CakeBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::BITES_0_6())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::BITES_0_6(), 0));

    // 预计算各片数的形状
    // 蛋糕高度8像素，每吃一片从一侧减少2像素
    constexpr f32 P = 1.0f / 16.0f;
    constexpr f32 heights[] = {15.0f, 13.0f, 11.0f, 9.0f, 7.0f, 5.0f, 3.0f};

    for (int i = 0; i < 7; ++i) {
        // 从右侧开始吃，每片减少2像素宽度
        f32 startX = 1.0f * P;
        f32 endX = heights[i] * P;
        m_shapesByBites[i] = CollisionShape::box(startX, 0.0f, 1.0f * P, endX, 8.0f * P, 15.0f * P);
    }
}

BlockState CakeBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool CakeBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 蛋糕需要放在固体方块上方
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    return belowState->isSolid();
}

BlockState CakeBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 下方方块被移除时，蛋糕掉落
    if (facing == Direction::Down) {
        BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !belowState->isSolid()) {
            // 返回空气状态，避免再次读取当前位置方块
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const CollisionShape& CakeBlock::getShape(const BlockState& state) const
{
    int bites = getBites(state);
    MC_ASSERT(bites >= 0 && bites <= 6);
    return m_shapesByBites[bites];
}

int CakeBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = (7 - 已吃片数) * 2
    return (7 - getBites(state)) * 2;
}

bool CakeBlock::eatSlice(IWorld& world, const BlockPos& pos, BlockState& state)
{
    int bites = getBites(state);

    if (bites < 6) {
        // 还有剩余片数，增加已吃片数
        BlockState newState = state.with(BlockStateProperties::BITES_0_6(), bites + 1);
        world.setBlockState(pos, &newState, 3);
        return true;
    } else {
        // 最后一片，移除方块
        // 参考 MC 1.16.5: CakeBlock.eatSlice
        world.setBlockState(pos, &VanillaBlocks::AIR->defaultState(), 3);
        return true;
    }
}

} // namespace blocks
} // namespace mc
