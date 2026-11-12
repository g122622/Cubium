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

#include "common/world/block/blocks/decorative/SkullBlock.hpp"

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

namespace {

/// 站立头颅碰撞形状：中心 8x8 像素、高 8 像素
const CollisionShape& _skullShape()
{
    static const CollisionShape shape = CollisionShape::fromPixelBox(4.0f, 0.0f, 4.0f, 12.0f, 8.0f, 12.0f);
    return shape;
}

/// 猪灵站立头颅形状：原版实测 box(0.1875, 0, 0.1875 -> 0.8125, 0.5, 0.8125)。
/// X 与 Z 都是 10 像素宽（3..13），高 8 像素。此前 Z 只有 8 像素（4..12）。
const CollisionShape& _piglinSkullShape()
{
    static const CollisionShape shape = CollisionShape::fromPixelBox(3.0f, 0.0f, 3.0f, 13.0f, 8.0f, 13.0f);
    return shape;
}

} // namespace

// ============================================================================
// SkullBlock（站立）
// ============================================================================

SkullBlock::SkullBlock(SkullType type, const BlockProperties& properties)
    : AbstractSkullBlock(type, properties)
    , m_shape(_skullShape())
    , m_piglinShape(_piglinSkullShape())
{
    // 站立头颅的乐器：音符盒放在其上方时演奏对应的生物头颅音色。
    // 原版 `Blocks.SKELETON_SKULL` 等注册时带 `.instrument(NoteBlockInstrument.SKELETON/ZOMBIE/...)`，
    // 玩家头带 `.instrument(NoteBlockInstrument.CUSTOM)`；这些都属于 worksAboveNoteBlock 类。
    // 墙面变体经 `Blocks.wallVariant(parent, true)` 继承同一乐器，见 WallSkullBlock 构造函数。
    setNonVirtualInstrument(_instrumentForSkullType(type));

    // 站立头颅额外持有 ROTATION_0_15 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::ROTATION_0_15())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(
        defaultState().with(BlockStateProperties::POWERED(), false).with(BlockStateProperties::ROTATION_0_15(), 0));
}

const CollisionShape& SkullBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return (m_type == SkullType::Piglin) ? m_piglinShape : m_shape;
}

BlockState SkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 旋转值：根据玩家 yaw 换算为 0-15（每 22.5 度为一段）
    i32 rotation = static_cast<i32>(std::floor((180.0f + context.getPlayerYaw()) * 16.0f / 360.0f + 0.5f)) & 15;

    bool powered = world::redstone::RedstonePower::isPowered(context.getWorld(), context.placementPos());
    return defaultState()
        .with(BlockStateProperties::POWERED(), powered)
        .with(BlockStateProperties::ROTATION_0_15(), rotation);
}

const BlockState& SkullBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& SkullBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

} // namespace blocks
} // namespace mc
