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

#include "MossyCarpetBlock.hpp"

#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../Block.hpp"
#include "../../BlockRegistry.hpp"
#include "../MultifaceBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

/// 水平四方向（北/东/南/西），用于遍历各 WALL_HEIGHT 属性。
constexpr Direction HORIZONTAL_DIRECTIONS[4] = {
    Direction::North,
    Direction::East,
    Direction::South,
    Direction::West,
};

/// BASE 平铺底座形状：1 像素厚的地板（0..1, 0..1/16, 0..1）。
const CollisionShape& baseShape()
{
    static const CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f / 16.0f, 1.0f);
    return shape;
}

/// LOW 附着面形状（按方向）：16 宽 × 10/16 高 × 1/16 深的薄片。
/// 对齐 MC boxZ(16,0,10,0,1) 旋转水平方向后的结果。
const CollisionShape& lowShape(Direction direction)
{
    static const CollisionShape shapes[4] = {
        // North: 贴 z=0 面，深 1/16
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 10.0f / 16.0f, 1.0f / 16.0f),
        // East: 贴 x=1 面，深 1/16
        CollisionShape::box(15.0f / 16.0f, 0.0f, 0.0f, 1.0f, 10.0f / 16.0f, 1.0f),
        // South: 贴 z=1 面，深 1/16
        CollisionShape::box(0.0f, 0.0f, 15.0f / 16.0f, 1.0f, 10.0f / 16.0f, 1.0f),
        // West: 贴 x=0 面，深 1/16
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f / 16.0f, 10.0f / 16.0f, 1.0f),
    };
    switch (direction) {
        case Direction::North:
            return shapes[0];
        case Direction::East:
            return shapes[1];
        case Direction::South:
            return shapes[2];
        case Direction::West:
            return shapes[3];
        default:
            return shapes[0];
    }
}

/// TALL 附着面形状（按方向）：16 宽 × 满高 × 1/16 深的薄片。
/// 对齐 MC boxZ(16,0,1) 旋转水平方向后的结果。
const CollisionShape& tallShape(Direction direction)
{
    static const CollisionShape shapes[4] = {
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f / 16.0f),
        CollisionShape::box(15.0f / 16.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
        CollisionShape::box(0.0f, 0.0f, 15.0f / 16.0f, 1.0f, 1.0f, 1.0f),
        CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f / 16.0f, 1.0f, 1.0f),
    };
    switch (direction) {
        case Direction::North:
            return shapes[0];
        case Direction::East:
            return shapes[1];
        case Direction::South:
            return shapes[2];
        case Direction::West:
            return shapes[3];
        default:
            return shapes[0];
    }
}

} // namespace

MossyCarpetBlock::MossyCarpetBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充（详见其它方块注释）：
    // createBlockState 触发 _cacheProperties→propagatesSkylightDown→getOcclusionShape→getShape，
    // 构造期回调 getShape 需 m_shapes 已就绪，否则依赖空 shape 的脆弱巧合。
    _initShapes();

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::BOTTOM())
            .add(BlockStateProperties::WALL_HEIGHT_NORTH())
            .add(BlockStateProperties::WALL_HEIGHT_EAST())
            .add(BlockStateProperties::WALL_HEIGHT_SOUTH())
            .add(BlockStateProperties::WALL_HEIGHT_WEST())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::BOTTOM(), true)
            .with(BlockStateProperties::WALL_HEIGHT_NORTH(), WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_EAST(), WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), WallHeight::None)
            .with(BlockStateProperties::WALL_HEIGHT_WEST(), WallHeight::None));
}

const EnumProperty<MossyCarpetBlock::WallHeight>& MossyCarpetBlock::_propertyForDirection(Direction direction)
{
    switch (direction) {
        case Direction::North:
            return BlockStateProperties::WALL_HEIGHT_NORTH();
        case Direction::East:
            return BlockStateProperties::WALL_HEIGHT_EAST();
        case Direction::South:
            return BlockStateProperties::WALL_HEIGHT_SOUTH();
        case Direction::West:
            return BlockStateProperties::WALL_HEIGHT_WEST();
        default:
            return BlockStateProperties::WALL_HEIGHT_NORTH();
    }
}

bool MossyCarpetBlock::_hasFaces(const BlockState& state)
{
    if (state.get(BlockStateProperties::BOTTOM())) {
        return true;
    }
    for (Direction dir : HORIZONTAL_DIRECTIONS) {
        if (state.get(_propertyForDirection(dir)) != WallHeight::None) {
            return true;
        }
    }
    return false;
}

bool MossyCarpetBlock::_canSupportAtFace(IWorld& world, const BlockPos& pos, Direction direction)
{
    // 对齐 MC MossyCarpetBlock.canSupportAtFace：UP 方向不支持，其余委托 MultifaceBlock.canAttachTo。
    if (direction == Direction::Up) {
        return false;
    }
    return MultifaceBlock::canAttachTo(world, direction, pos.offset(direction));
}

BlockState MossyCarpetBlock::_getUpdatedState(
    BlockState state, IWorld& world, const BlockPos& pos, bool includeBase) const
{
    // 对齐 MC MossyCarpetBlock.getUpdatedState：
    // - includeBase 时把 BASE 视为 true 参与判定；
    // - 各水平方向：相邻方块可附着则（includeBase?LOW:保持原值），否则 NONE；
    // - 若该方向为 LOW 且上方同类型方块该方向非 NONE 且非底座，则升级为 TALL；
    // - 非底座时若下方同类型方块该方向为 NONE，则降回 NONE。
    bool base = includeBase || state.get(BlockStateProperties::BOTTOM());

    for (Direction dir : HORIZONTAL_DIRECTIONS) {
        const auto& prop = _propertyForDirection(dir);
        WallHeight height =
            _canSupportAtFace(world, pos, dir) ? (base ? WallHeight::Low : state.get(prop)) : WallHeight::None;

        if (height == WallHeight::Low) {
            const BlockState* above = world.getBlockState(pos.up());
            if (above != nullptr && above->is(this) && above->get(prop) != WallHeight::None &&
                !above->get(BlockStateProperties::BOTTOM())) {
                height = WallHeight::Tall;
            }

            if (!state.get(BlockStateProperties::BOTTOM())) {
                const BlockState* below = world.getBlockState(pos.down());
                if (below != nullptr && below->is(this) && below->get(prop) == WallHeight::None) {
                    height = WallHeight::None;
                }
            }
        }

        state = state.with(prop, height);
    }

    return state;
}

BlockState MossyCarpetBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return _getUpdatedState(defaultState(), context.getWorld(), context.placementPos(), true);
}

BlockState MossyCarpetBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        if (const BlockState* air = BlockRegistry::instance().airState()) {
            return *air;
        }
        return state;
    }
    BlockState updated = _getUpdatedState(state, world, currentPos, false);
    if (!_hasFaces(updated)) {
        if (const BlockState* air = BlockRegistry::instance().airState()) {
            return *air;
        }
        return updated;
    }
    return updated;
}

bool MossyCarpetBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 对齐 MC MossyCarpetBlock.canSurvive：
    // - 底座：下方非空气；
    // - 非底座：下方为同类型底座方块。
    const BlockState* below = world.getBlockState(pos.down());
    if (state.get(BlockStateProperties::BOTTOM())) {
        return below != nullptr && !below->isAir();
    }
    return below != nullptr && below->is(this) && below->get(BlockStateProperties::BOTTOM());
}

const BlockState& MossyCarpetBlock::rotate(const BlockState& state, Rotation rotation) const
{
    WallHeight north = state.get(BlockStateProperties::WALL_HEIGHT_NORTH());
    WallHeight east = state.get(BlockStateProperties::WALL_HEIGHT_EAST());
    WallHeight south = state.get(BlockStateProperties::WALL_HEIGHT_SOUTH());
    WallHeight west = state.get(BlockStateProperties::WALL_HEIGHT_WEST());

    switch (rotation) {
        case Rotation::Clockwise180:
            return state.with(BlockStateProperties::WALL_HEIGHT_NORTH(), south)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), west)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), north)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), east);
        case Rotation::CounterClockwise90:
            return state.with(BlockStateProperties::WALL_HEIGHT_NORTH(), east)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), south)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), west)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), north);
        case Rotation::Clockwise90:
            return state.with(BlockStateProperties::WALL_HEIGHT_NORTH(), west)
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), north)
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), east)
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), south);
        case Rotation::None:
        default:
            return state;
    }
}

const BlockState& MossyCarpetBlock::mirror(const BlockState& state, Mirror mirror) const
{
    switch (mirror) {
        case Mirror::LeftRight:
            return state
                .with(BlockStateProperties::WALL_HEIGHT_NORTH(), state.get(BlockStateProperties::WALL_HEIGHT_SOUTH()))
                .with(BlockStateProperties::WALL_HEIGHT_SOUTH(), state.get(BlockStateProperties::WALL_HEIGHT_NORTH()));
        case Mirror::FrontBack:
            return state
                .with(BlockStateProperties::WALL_HEIGHT_EAST(), state.get(BlockStateProperties::WALL_HEIGHT_WEST()))
                .with(BlockStateProperties::WALL_HEIGHT_WEST(), state.get(BlockStateProperties::WALL_HEIGHT_EAST()));
        case Mirror::None:
        default:
            return state;
    }
}

const CollisionShape& MossyCarpetBlock::getShape(const BlockState& state) const
{
    const size_t index = _shapeIndex(state.get(BlockStateProperties::BOTTOM()),
        state.get(BlockStateProperties::WALL_HEIGHT_NORTH()),
        state.get(BlockStateProperties::WALL_HEIGHT_EAST()),
        state.get(BlockStateProperties::WALL_HEIGHT_SOUTH()),
        state.get(BlockStateProperties::WALL_HEIGHT_WEST()));
    MC_ASSERT_DEBUG(index < m_shapes.size());
    return m_shapes[index];
}

size_t MossyCarpetBlock::_shapeIndex(bool bottom, WallHeight north, WallHeight east, WallHeight south, WallHeight west)
{
    // bottom 占 2 种，每个方向高度占 3 种（None=0/Low=1/Tall=2）。
    size_t idx = bottom ? 1 : 0;
    idx += static_cast<size_t>(north) * 2;
    idx += static_cast<size_t>(east) * 6;
    idx += static_cast<size_t>(south) * 18;
    idx += static_cast<size_t>(west) * 54;
    return idx;
}

void MossyCarpetBlock::_initShapes()
{
    // 预计算全部 2×3^4 = 162 种形状：底座 + 各方向 LOW/TALL 薄片叠加。
    m_shapes.resize(162);

    static constexpr WallHeight HEIGHTS[3] = {WallHeight::None, WallHeight::Low, WallHeight::Tall};

    for (i32 bottom = 0; bottom < 2; ++bottom) {
        for (i32 north = 0; north < 3; ++north) {
            for (i32 east = 0; east < 3; ++east) {
                for (i32 south = 0; south < 3; ++south) {
                    for (i32 west = 0; west < 3; ++west) {
                        const size_t idx =
                            _shapeIndex(bottom != 0, HEIGHTS[north], HEIGHTS[east], HEIGHTS[south], HEIGHTS[west]);
                        CollisionShape shape = CollisionShape::empty();
                        if (bottom != 0) {
                            shape = baseShape();
                        }
                        const WallHeight heights[4] = {HEIGHTS[north], HEIGHTS[east], HEIGHTS[south], HEIGHTS[west]};
                        for (i32 d = 0; d < 4; ++d) {
                            if (heights[d] == WallHeight::Low) {
                                shape = CollisionShape::combine(shape, lowShape(HORIZONTAL_DIRECTIONS[d]));
                            } else if (heights[d] == WallHeight::Tall) {
                                shape = CollisionShape::combine(shape, tallShape(HORIZONTAL_DIRECTIONS[d]));
                            }
                        }
                        m_shapes[idx] = shape;
                    }
                }
            }
        }
    }
}

} // namespace blocks
} // namespace mc
