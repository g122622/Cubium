/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
 * LIABILITY, ARISING FROM, AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CandleCakeBlock.hpp"

#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"

namespace mc {
namespace blocks {

CandleCakeBlock::CandleCakeBlock(BlockProperties properties, Block* candleBlock)
    : AbstractCandleBlock(std::move(properties))
    , m_candleBlock(candleBlock)
{
    // 创建状态容器：仅 LIT（蜡烛蛋糕没有 CANDLES 和 WATERLOGGED）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LIT())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：未点燃
    setDefaultState(defaultState().with(BlockStateProperties::LIT(), false));

    // 蜡烛蛋糕形状：蛋糕主体 + 蜡烛柱体
    // 参考 MC Java: Shapes.or(Block.column(2.0, 8.0, 14.0), Block.column(14.0, 0.0, 8.0))
    // 即：蜡烛部分 y=8-14, 半径1px(2/16)；蛋糕部分 y=0-8, 半径7px(14/16)
    // 使用 fromPixelBox，坐标系为像素 (0-16)
    m_shape = CollisionShape::fromPixelBox(1.0f, 0.0f, 1.0f, 15.0f, 8.0f, 15.0f);
}

BlockState CandleCakeBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    // 蜡烛蛋糕默认未点燃
    return defaultState();
}

bool CandleCakeBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 下方方块必须是固体
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }
    return belowState->isSolid();
}

BlockState CandleCakeBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 下方支撑变化时检查有效性
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

const CollisionShape& CandleCakeBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

u8 CandleCakeBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 点燃时亮度为3
    if (isLit(state)) {
        return 3;
    }
    return 0;
}

std::vector<Vector3f> CandleCakeBlock::getParticleOffsets(const BlockState& state) const
{
    MC_UNUSED(state);
    // 蜡烛蛋糕只有一根蜡烛，偏移位置固定在中心偏上
    return {{0.5f, 0.5f, 0.5f}};
}

i32 CandleCakeBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 蜡烛蛋糕始终输出满信号（类似完整蛋糕）
    return 14;
}

} // namespace blocks
} // namespace mc
