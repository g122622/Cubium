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

#include "StairsBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

// ========== 静态形状定义 ==========
// 参考 MC 1.16.5 StairsBlock.java

// 下半台阶形状 (0-8像素高)
static const CollisionShape SLAB_BOTTOM = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);

// 上半台阶形状 (8-16像素高)
static const CollisionShape SLAB_TOP = CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);

// 角落形状 (8x8x8 的立方体)
// 命名规则: [N/S][W/E][D/U]_CORNER
// N=North, S=South, W=West, E=East, D=Down(下半), U=Up(上半)
// 坐标系: X轴向东, Y轴向上, Z轴向南

// 西北下半角 (0,0,0)-(0.5,0.5,0.5)
static const CollisionShape NWD_CORNER = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f);

// 西南下半角 (0,0,0.5)-(0.5,0.5,1.0)
static const CollisionShape SWD_CORNER = CollisionShape::box(0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f);

// 西北上半角 (0,0.5,0)-(0.5,1.0,0.5)
static const CollisionShape NWU_CORNER = CollisionShape::box(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f);

// 西南上半角 (0,0.5,0.5)-(0.5,1.0,1.0)
static const CollisionShape SWU_CORNER = CollisionShape::box(0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f);

// 东北下半角 (0.5,0,0)-(1.0,0.5,0.5)
static const CollisionShape NED_CORNER = CollisionShape::box(0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f);

// 东南下半角 (0.5,0,0.5)-(1.0,0.5,1.0)
static const CollisionShape SED_CORNER = CollisionShape::box(0.5f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f);

// 东北上半角 (0.5,0.5,0)-(1.0,1.0,0.5)
static const CollisionShape NEU_CORNER = CollisionShape::box(0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);

// 东南上半角 (0.5,0.5,0.5)-(1.0,1.0,1.0)
static const CollisionShape SEU_CORNER = CollisionShape::box(0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f);

/**
 * @brief 根据位域组合形状
 *
 * 位域规则:
 * - bit 0 (1): 包含西北角
 * - bit 1 (2): 包含东北角
 * - bit 2 (4): 包含西南角
 * - bit 3 (8): 包含东南角
 *
 * 参考: MC 1.16.5 StairsBlock.combineShapes()
 *
 * @param bitfield 位域值 (0-15)
 * @param slabShape 基础台阶形状
 * @param nwCorner 西北角形状
 * @param neCorner 东北角形状
 * @param swCorner 西南角形状
 * @param seCorner 东南角形状
 * @return 组合后的形状
 */
static CollisionShape combineShapes(int bitfield,
    const CollisionShape& slabShape,
    const CollisionShape& nwCorner,
    const CollisionShape& neCorner,
    const CollisionShape& swCorner,
    const CollisionShape& seCorner)
{
    CollisionShape result = slabShape;

    if ((bitfield & 1) != 0) {
        result = CollisionShape::combine(result, nwCorner, CollisionShape::CombineOp::OR);
    }
    if ((bitfield & 2) != 0) {
        result = CollisionShape::combine(result, neCorner, CollisionShape::CombineOp::OR);
    }
    if ((bitfield & 4) != 0) {
        result = CollisionShape::combine(result, swCorner, CollisionShape::CombineOp::OR);
    }
    if ((bitfield & 8) != 0) {
        result = CollisionShape::combine(result, seCorner, CollisionShape::CombineOp::OR);
    }

    return result;
}

/**
 * @brief 创建形状数组
 *
 * 为下半或上半楼梯创建16种形状。
 * 每种形状对应不同的角落组合。
 *
 * @param slabShape 台阶形状 (SLAB_BOTTOM 或 SLAB_TOP)
 * @param nwCorner 西北角形状
 * @param neCorner 东北角形状
 * @param swCorner 西南角形状
 * @param seCorner 东南角形状
 * @return 包含16种形状的数组
 */
static std::array<CollisionShape, 16> makeShapes(const CollisionShape& slabShape,
    const CollisionShape& nwCorner,
    const CollisionShape& neCorner,
    const CollisionShape& swCorner,
    const CollisionShape& seCorner)
{
    std::array<CollisionShape, 16> shapes;
    for (int i = 0; i < 16; ++i) {
        shapes[i] = combineShapes(i, slabShape, nwCorner, neCorner, swCorner, seCorner);
    }
    return shapes;
}

// ========== 静态形状数组 ==========
// 下半楼梯形状: 使用下半台阶 + 上半角
static const std::array<CollisionShape, 16> SLAB_BOTTOM_SHAPES =
    makeShapes(SLAB_BOTTOM, NWU_CORNER, NEU_CORNER, SWU_CORNER, SEU_CORNER);

// 上半楼梯形状: 使用上半台阶 + 下半角
static const std::array<CollisionShape, 16> SLAB_TOP_SHAPES =
    makeShapes(SLAB_TOP, NWD_CORNER, NED_CORNER, SWD_CORNER, SED_CORNER);

/**
 * @brief 形状索引映射数组
 *
 * 将 (shape.ordinal() * 4 + facing.getHorizontalIndex()) 映射到形状数组索引。
 * 顺序: STRAIGHT(4) + INNER_LEFT(4) + INNER_RIGHT(4) + OUTER_LEFT(4) + OUTER_RIGHT(4)
 * 每组按 NORTH(0), SOUTH(1), WEST(2), EAST(3) 排列
 *
 * 参考: MC 1.16.5 StairsBlock.field_196522_K
 */
static constexpr std::array<int, 20> SHAPE_INDEX_MAP = {
    12, 5, 3, 10, 14, 13, 7, 11, 13, 7, 11, 14, 8, 4, 1, 2, 4, 1, 2, 8};

// ========== 构造函数 ==========

StairsBlock::StairsBlock(const BlockState& baseState, const BlockProperties& properties)
    : Block(properties)
    , m_baseState(&baseState)
    , m_fullCubeShape(CollisionShape::fullBlock())
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
            .add(BlockStateProperties::STAIRS_SHAPE())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
            .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

// ========== 状态容器 ==========

void StairsBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 属性已在构造函数中添加
    MC_UNUSED(container);
}

// ========== 放置和更新 ==========

BlockState StairsBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 参考: MC 1.16.5 StairsBlock.getStateForPlacement()
    Direction facing = context.horizontalDirection();

    // 根据点击位置决定上半/下半
    // 如果点击面是DOWN，或者（点击面不是UP且点击Y坐标大于0.5），则放置上半
    bool isTop = context.getHitY() > 0.5f;

    bool waterlogged = waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos());

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(),
            isTop ? BlockStateProperties::DoubleBlockHalf::Upper : BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::STAIRS_SHAPE(), BlockStateProperties::StairsShape::Straight)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState StairsBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingPos);
    MC_UNUSED(facingState);

    // 调度流体 tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 只在水平方向更新时重新计算形状
    if (Directions::getAxis(facing) == Axis::Y) {
        return state;
    }

    // 计算新的形状
    BlockStateProperties::StairsShape newShape = calculateShape(state, world, currentPos);

    if (state.get(BlockStateProperties::STAIRS_SHAPE()) != newShape) {
        return state.with(BlockStateProperties::STAIRS_SHAPE(), newShape);
    }

    return state;
}

// ========== 形状 ==========

const CollisionShape& StairsBlock::getShape(const BlockState& state) const
{
    // 参考: MC 1.16.5 StairsBlock.getShape()
    // return (state.get(HALF) == Half.TOP ? SLAB_TOP_SHAPES :
    // SLAB_BOTTOM_SHAPES)[field_196522_K[this.func_196511_x(state)]];
    return getShapeForState(state);
}

const CollisionShape& StairsBlock::getCollisionShape(const BlockState& state) const
{
    return getShape(state);
}

const CollisionShape& StairsBlock::getOcclusionShape(const BlockState& state) const
{
    // 楼梯不阻挡全部光照
    return getShape(state);
}

// ========== 旋转和镜像 ==========

const BlockState& StairsBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& StairsBlock::mirror(const BlockState& state, Mirror mirror) const
{
    // 参考: MC 1.16.5 StairsBlock.mirror()
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::StairsShape shape = state.get(BlockStateProperties::STAIRS_SHAPE());

    // MC中镜像逻辑较为复杂，这里实现完整逻辑
    switch (mirror) {
        case Mirror::LeftRight:
            // 左右镜像 (关于X轴对称)
            if (facing == Direction::South || facing == Direction::North) {
                // Z轴方向: 旋转180度并交换形状的左右
                Direction rotated = Directions::rotateDirection(facing, Rotation::Clockwise180);
                BlockStateProperties::StairsShape mirroredShape = shape;
                switch (shape) {
                    case BlockStateProperties::StairsShape::InnerLeft:
                        mirroredShape = BlockStateProperties::StairsShape::InnerRight;
                        break;
                    case BlockStateProperties::StairsShape::InnerRight:
                        mirroredShape = BlockStateProperties::StairsShape::InnerLeft;
                        break;
                    case BlockStateProperties::StairsShape::OuterLeft:
                        mirroredShape = BlockStateProperties::StairsShape::OuterRight;
                        break;
                    case BlockStateProperties::StairsShape::OuterRight:
                        mirroredShape = BlockStateProperties::StairsShape::OuterLeft;
                        break;
                    default:
                        break;
                }
                const BlockState& result = state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
                return result.with(BlockStateProperties::STAIRS_SHAPE(), mirroredShape);
            }
            break;

        case Mirror::FrontBack:
            // 前后镜像 (关于Z轴对称)
            if (facing == Direction::East || facing == Direction::West) {
                // X轴方向: 旋转180度并交换形状的左右
                Direction rotated = Directions::rotateDirection(facing, Rotation::Clockwise180);
                BlockStateProperties::StairsShape mirroredShape = shape;
                switch (shape) {
                    case BlockStateProperties::StairsShape::InnerLeft:
                        mirroredShape = BlockStateProperties::StairsShape::InnerRight;
                        break;
                    case BlockStateProperties::StairsShape::InnerRight:
                        mirroredShape = BlockStateProperties::StairsShape::InnerLeft;
                        break;
                    case BlockStateProperties::StairsShape::OuterLeft:
                        mirroredShape = BlockStateProperties::StairsShape::OuterRight;
                        break;
                    case BlockStateProperties::StairsShape::OuterRight:
                        mirroredShape = BlockStateProperties::StairsShape::OuterLeft;
                        break;
                    default:
                        break;
                }
                const BlockState& result = state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
                return result.with(BlockStateProperties::STAIRS_SHAPE(), mirroredShape);
            }
            break;

        case Mirror::None:
        default:
            break;
    }

    return state;
}

// ========== 静态方法 ==========

bool StairsBlock::isStairs(const BlockState& state)
{
    // 检查方块是否继承自 StairsBlock
    return state.hasProperty(BlockStateProperties::STAIRS_SHAPE());
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* StairsBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== 私有方法 ==========

BlockStateProperties::StairsShape StairsBlock::calculateShape(
    const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    // 参考: MC 1.16.5 StairsBlock.getShapeProperty()
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());

    // 检查朝向方向的邻居
    BlockPos forwardPos(pos.x + Directions::xOffset(facing), pos.y, pos.z + Directions::zOffset(facing));
    const BlockState* forwardState = world.getBlockState(forwardPos);

    if (forwardState != nullptr && isStairs(*forwardState)) {
        Direction forwardFacing = forwardState->get(BlockStateProperties::HORIZONTAL_FACING());
        BlockStateProperties::DoubleBlockHalf forwardHalf =
            forwardState->get(BlockStateProperties::DOUBLE_BLOCK_HALF());

        // 检查是否同层且朝向垂直
        if (forwardHalf == half && Directions::getAxis(forwardFacing) != Directions::getAxis(facing)) {
            // 检查第三个位置是否形成外角
            if (isDifferentStairs(state, world, pos, Directions::opposite(forwardFacing))) {
                // 判断是左外角还是右外角
                if (forwardFacing == Directions::rotateYCCW(facing)) {
                    return BlockStateProperties::StairsShape::OuterLeft;
                }
                return BlockStateProperties::StairsShape::OuterRight;
            }
        }
    }

    // 检查反方向的邻居
    BlockPos backwardPos(pos.x - Directions::xOffset(facing), pos.y, pos.z - Directions::zOffset(facing));
    const BlockState* backwardState = world.getBlockState(backwardPos);

    if (backwardState != nullptr && isStairs(*backwardState)) {
        Direction backwardFacing = backwardState->get(BlockStateProperties::HORIZONTAL_FACING());
        BlockStateProperties::DoubleBlockHalf backwardHalf =
            backwardState->get(BlockStateProperties::DOUBLE_BLOCK_HALF());

        // 检查是否同层且朝向垂直
        if (backwardHalf == half && Directions::getAxis(backwardFacing) != Directions::getAxis(facing)) {
            // 检查第三个位置是否形成内角
            if (isDifferentStairs(state, world, pos, backwardFacing)) {
                // 判断是左内角还是右内角
                if (backwardFacing == Directions::rotateYCCW(facing)) {
                    return BlockStateProperties::StairsShape::InnerLeft;
                }
                return BlockStateProperties::StairsShape::InnerRight;
            }
        }
    }

    // 默认直梯
    return BlockStateProperties::StairsShape::Straight;
}

bool StairsBlock::isDifferentStairs(const BlockState& state, IWorld& world, const BlockPos& pos, Direction face) const
{
    // 参考: MC 1.16.5 StairsBlock.isDifferentStairs()
    BlockPos checkPos(
        pos.x + Directions::xOffset(face), pos.y + Directions::yOffset(face), pos.z + Directions::zOffset(face));
    const BlockState* checkState = world.getBlockState(checkPos);

    if (checkState == nullptr || !isStairs(*checkState)) {
        return true;
    }

    // 检查朝向、half是否相同
    Direction checkFacing = checkState->get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::DoubleBlockHalf checkHalf = checkState->get(BlockStateProperties::DOUBLE_BLOCK_HALF());

    return checkFacing != state.get(BlockStateProperties::HORIZONTAL_FACING()) ||
        checkHalf != state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
}

size_t StairsBlock::getStateIndex(const BlockState& state)
{
    // 参考: MC 1.16.5 StairsBlock.func_196511_x()
    // return state.get(SHAPE).ordinal() * 4 + state.get(FACING).getHorizontalIndex();
    size_t shapeIdx = static_cast<size_t>(state.get(BlockStateProperties::STAIRS_SHAPE()));
    size_t facingIdx = 0;

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    switch (facing) {
        case Direction::North:
            facingIdx = 0;
            break;
        case Direction::South:
            facingIdx = 1;
            break;
        case Direction::West:
            facingIdx = 2;
            break;
        case Direction::East:
            facingIdx = 3;
            break;
        default:
            facingIdx = 0;
            break;
    }

    return shapeIdx * 4 + facingIdx;
}

const CollisionShape& StairsBlock::getShapeForState(const BlockState& state) const
{
    // 参考: MC 1.16.5 StairsBlock.getShape()
    // return (state.get(HALF) == Half.TOP ? SLAB_TOP_SHAPES :
    // SLAB_BOTTOM_SHAPES)[field_196522_K[this.func_196511_x(state)]];

    bool isTop = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == BlockStateProperties::DoubleBlockHalf::Upper;
    size_t stateIdx = getStateIndex(state);

    // 确保索引在有效范围内
    MC_ASSERT(stateIdx < SHAPE_INDEX_MAP.size());

    size_t shapeIdx = static_cast<size_t>(SHAPE_INDEX_MAP[stateIdx]);
    MC_ASSERT(shapeIdx < 16);

    return isTop ? SLAB_TOP_SHAPES[shapeIdx] : SLAB_BOTTOM_SHAPES[shapeIdx];
}

} // namespace blocks
} // namespace mc
