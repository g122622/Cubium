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

#include "common/world/block/blocks/decorative/WallSkullBlock.hpp"

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

namespace {

/// 构造墙壁头颅的形状表（按 North/South/West/East 顺序，占据靠墙一侧 8/16 深度）
void _buildWallShapes(CollisionShape out[4], f32 widthPixels)
{
    // 原版墙壁头颅形状（实测 forAllBoxes）：
    //
    //   北向: box(0.25, 0.25, 0.5 -> 0.75, 0.75, 1.0)
    //
    // 即宽度居中、**Y 也居中**（4..12 像素）、深度占据靠墙一侧的一半。墙上的头颅朝向
    // 与站立时一致，无需旋转 Y。
    //
    // 此前 Y 范围写成 8..16 像素（贴顶），头颅比原版高出一半的位置。
    const f32 half = widthPixels / 2.0f / 16.0f;
    const f32 cx = 0.5f;
    constexpr f32 yMin = 4.0f / 16.0f;
    constexpr f32 yMax = 12.0f / 16.0f;
    // 朝向与占据侧的对应（依据原版实测）：
    //   北向 -> Z 靠 1 侧；南向 -> Z 靠 0 侧；西向 -> X 靠 1 侧；东向 -> X 靠 0 侧
    // 头颅的「面朝」方向与它贴附的墙面相反，因此占据的是远离 FACING 的那一半。
    out[0] = CollisionShape::box(cx - half, yMin, 0.5f, cx + half, yMax, 1.0f); // North
    out[1] = CollisionShape::box(cx - half, yMin, 0.0f, cx + half, yMax, 0.5f); // South
    out[2] = CollisionShape::box(0.5f, yMin, cx - half, 1.0f, yMax, cx + half); // West
    out[3] = CollisionShape::box(0.0f, yMin, cx - half, 0.5f, yMax, cx + half); // East
}

/// 方向到形状表下标（水平方向）
i32 _facingIndex(Direction facing)
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
            return 0;
    }
}

} // namespace

// ============================================================================
// WallSkullBlock（贴墙）
// ============================================================================

WallSkullBlock::WallSkullBlock(SkullType type, const BlockProperties& properties)
    : AbstractSkullBlock(type, properties)
{
    // 墙挂头颅不继承站立头颅的乐器，音符盒在其上方演奏默认的竖琴。
    // 原版注册墙面变体用的是 `Blocks.wallVariant(Blocks.SKELETON_SKULL, true)`，
    // 而 wallVariant 从 `Properties.of()` 全新建起属性，第二个参数只控制
    // `overrideDescription`（覆盖描述），与乐器无关；战利品表是唯一被复制过来的属性。
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    _buildWallShapes(m_shapes, 8.0f);
}

const CollisionShape& WallSkullBlock::_shapeForFacing(Direction facing) const
{
    return m_shapes[_facingIndex(facing)];
}

const CollisionShape& WallSkullBlock::getShape(const BlockState& state) const
{
    return _shapeForFacing(state.get(BlockStateProperties::HORIZONTAL_FACING()));
}

BlockState WallSkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 遍历最近视线方向，取首个水平方向的反方向作为 FACING，
    // 且要求该方向的相邻方块不可替换（即有可附着的实心墙面）。
    IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();

    bool powered = world::redstone::RedstonePower::isPowered(world, pos);

    for (Direction direction : context.getNearestLookingDirections()) {
        if (!Directions::isHorizontal(direction)) {
            continue;
        }
        Direction facing = Directions::opposite(direction);
        const BlockPos attachPos = pos.offset(direction);
        const BlockState* attachState = world.getBlockState(attachPos);
        if (attachState != nullptr && !attachState->isAir() &&
            attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full)) {
            return defaultState()
                .with(BlockStateProperties::POWERED(), powered)
                .with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        }
    }

    return defaultState().with(BlockStateProperties::POWERED(), powered);
}

const BlockState& WallSkullBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::rotateDirection(facing, rotation));
}

const BlockState& WallSkullBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(),
        Directions::rotateDirection(facing, Directions::mirrorToRotation(mirror, facing)));
}

} // namespace blocks
} // namespace mc
