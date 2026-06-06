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
#include "common/world/IWorld.hpp"

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

    // 检查上方是否有支撑方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr) {
        return false;
    }

    // TODO: 检查是否为苍白橡木原木、苍白橡木树叶或苍白苔藓
    // 目前返回 true
    return true;
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

bool PaleHangingMossBlock::useShapeForLightOcclusion(const BlockState& state) const
{
    MC_UNUSED(state);
    return true;
}

void PaleHangingMossBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

} // namespace blocks
} // namespace mc
