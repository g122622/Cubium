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

#include "common/world/block/blocks/end/EndPortalFrameBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/block/state/pattern/BlockInWorld.hpp"
#include "common/world/block/state/pattern/BlockPattern.hpp"
#include "common/world/block/state/pattern/BlockPatternBuilder.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

EndPortalFrameBlock::EndPortalFrameBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::EYE())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([this](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::EYE(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // vanilla：
    //   SHAPE_EMPTY = Block.column(16.0, 0.0, 13.0)                 → 整格宽、13 像素高的底座
    //   SHAPE_FULL  = Shapes.or(SHAPE_EMPTY, Block.column(8, 13, 16)) → 底座 + 中央 8×8×3 的凸起
    // 有眼状态不是「整个方块变成 16 像素高的立方体」，而是底座之上多出一个窄凸起——
    // 此前写成整格立方体，导致有眼框架被算作完全实心、天空光透传判定翻转。
    m_frameShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.8125f, 1.0f);
    m_frameWithEyeShape =
        CollisionShape::combine(m_frameShape, CollisionShape::box(0.25f, 0.8125f, 0.25f, 0.75f, 1.0f, 0.75f));
}

bool EndPortalFrameBlock::hasEye(const BlockState& state) const noexcept
{
    return state.get(BlockStateProperties::EYE());
}

i32 EndPortalFrameBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // vanilla EndPortalFrameBlock#getAnalogOutputSignal：有眼 15，无眼 0。
    return hasEye(state) ? 15 : 0;
}

Direction EndPortalFrameBlock::getFacing(const BlockState& state) const noexcept
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

BlockState EndPortalFrameBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 对齐 vanilla EndPortalFrameBlock.getStateForPlacement：
    //   defaultBlockState().setValue(FACING, ctx.getHorizontalDirection().getOpposite()).setValue(HAS_EYE, false)
    // 即框架 FACING 朝向玩家面朝方向的反方向。
    Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing));
}

const BlockState& EndPortalFrameBlock::rotate(const BlockState& state, Rotation rotation) const noexcept
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& EndPortalFrameBlock::mirror(const BlockState& state, Mirror mirror) const noexcept
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& EndPortalFrameBlock::getShape(const BlockState& state) const noexcept
{
    return hasEye(state) ? m_frameWithEyeShape : m_frameShape;
}

std::unique_ptr<blockpattern::BlockPattern> EndPortalFrameBlock::getOrCreatePortalShape()
{
    // 对应 MC Java: EndPortalFrameBlock.getOrCreatePortalShape()
    //
    // 模式布局（aisle 从上到下，每个字符串为一行）：
    //   "?vvv?"
    //   ">???<"
    //   ">???<"
    //   ">???<"
    //   "?^^^?"
    //
    // 字符含义：
    //   '?' - 任意方块（传送门内部和外围角落）
    //   '^' - 含眼且朝向 SOUTH 的末地传送门框架
    //   '>' - 含眼且朝向 WEST  的末地传送门框架
    //   'v' - 含眼且朝向 NORTH 的末地传送门框架
    //   '<' - 含眼且朝向 EAST  的末地传送门框架
    //
    // 匹配后 frontTopLeft() 返回左上角框架位置，传送门内部 3×3 区域从该位置开始。
    using namespace blockpattern;

    const Block* frameBlock = VanillaBlocks::END_PORTAL_FRAME;
    // 注意：模式字符串 ">???<" 中的 "??<" 会被 C++ 编译器识别为 trigraph，
    // 因此用相邻字符串字面量拼接（">???" "<"）来断开 trigraph 序列。
    // 拼接在预处理 phase 3 完成，而 trigraph 替换在 phase 1，故拼接后的
    // ">???<" 不会触发 trigraph 警告。
    return BlockPatternBuilder::start()
        .aisle({"?vvv?",
            ">???"
            "<",
            ">???"
            "<",
            ">???"
            "<",
            "?^^^?"})
        .where('?', BlockInWorld::hasState([](const BlockState&) {
            return true;
        }))
        .where('^', BlockInWorld::hasState([frameBlock](const BlockState& s) {
            return s.is(frameBlock) && s.get(BlockStateProperties::EYE()) &&
                s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::South;
        }))
        .where('>', BlockInWorld::hasState([frameBlock](const BlockState& s) {
            return s.is(frameBlock) && s.get(BlockStateProperties::EYE()) &&
                s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::West;
        }))
        .where('v', BlockInWorld::hasState([frameBlock](const BlockState& s) {
            return s.is(frameBlock) && s.get(BlockStateProperties::EYE()) &&
                s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::North;
        }))
        .where('<', BlockInWorld::hasState([frameBlock](const BlockState& s) {
            return s.is(frameBlock) && s.get(BlockStateProperties::EYE()) &&
                s.get(BlockStateProperties::HORIZONTAL_FACING()) == Direction::East;
        }))
        .build();
}

bool EndPortalFrameBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 原版显式返回 false：形状不完整不等于可以穿过。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

} // namespace blocks
} // namespace mc
