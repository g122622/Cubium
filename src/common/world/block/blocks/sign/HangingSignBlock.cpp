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

#include "common/world/block/blocks/sign/HangingSignBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/sign/AbstractSignBlock.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== CeilingHangingSignBlock ==========

CeilingHangingSignBlock::CeilingHangingSignBlock(const BlockProperties& properties, const WoodType& woodType)
    : AbstractSignBlock(properties, woodType, woodType.hangingSignSoundType)
    , m_defaultShape(CollisionShape::fromPixelBox(3.0f, 0.0f, 3.0f, 13.0f, 16.0f, 13.0f))
{
    // 对齐原版：
    //   SHAPE_DEFAULT = column(10.0, 0.0, 16.0)  -> box(3, 0, 3, 13, 16, 13)   默认（Y 到 16）
    //   SHAPES        = rotateHorizontal(column(14.0, 2.0, 0.0, 10.0))
    //                                            -> box(1, 0, 1, 15, 10, 15)   旋转表（Y 只到 10）
    //
    // rotation 与旋转段的映射实测结果：
    //   0, 8   -> box(1, 0, 7, 15, 10, 9)   长边沿 X
    //   4, 12  -> box(7, 0, 1, 9, 10, 15)   长边沿 Z
    //
    // 注意 `SHAPE_DEFAULT` 的 Y 到 16，而旋转表的 Y 只到 10——这个高度差异会直接影响
    // `UP|CENTER` 的判定（面切片取的 VoxelShape 不同），必须严格按原版区分。
    m_shapesByRotation[0] = CollisionShape::fromPixelBox(1.0f, 0.0f, 7.0f, 15.0f, 10.0f, 9.0f);
    m_shapesByRotation[8] = m_shapesByRotation[0];
    m_shapesByRotation[4] = CollisionShape::fromPixelBox(7.0f, 0.0f, 1.0f, 9.0f, 10.0f, 15.0f);
    m_shapesByRotation[12] = m_shapesByRotation[4];
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ROTATION_0_15())
            .add(BlockStateProperties::ATTACHED())
            .add(BlockStateProperties::WATERLOGGED())
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
            .with(BlockStateProperties::ROTATION_0_15(), 0)
            .with(BlockStateProperties::ATTACHED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState CeilingHangingSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    f32 yaw = context.getPlayerYaw();
    i32 rotation = static_cast<i32>(std::floor((180.0f + yaw) * 16.0f / 360.0f + 0.5f)) & 15;

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 检查上方是否有另一个天花板悬挂告示牌（形成链条连接）
    bool attached = false;
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr) {
        const Block* aboveBlock = &aboveState->getBlock();
        // 如果上方也是天花板悬挂告示牌，则当前为连接状态
        if (dynamic_cast<const CeilingHangingSignBlock*>(aboveBlock) != nullptr) {
            attached = true;
        }
    }

    return defaultState()
        .with(BlockStateProperties::ROTATION_0_15(), rotation)
        .with(BlockStateProperties::ATTACHED(), attached)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CeilingHangingSignBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 需要上方有固体支撑
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    return aboveState != nullptr && aboveState->isSolidSide(world, abovePos, Direction::Down);
}

const BlockState& CeilingHangingSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& CeilingHangingSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const CollisionShape& CeilingHangingSignBlock::getShape(const BlockState& state) const
{
    // 对齐原版 `SHAPES.getOrDefault(state.getValue(ROTATION), SHAPE_DEFAULT)`：
    // 只有 rotation 0/8（长边沿 X）与 4/12（长边沿 Z）命中旋转表，其余走默认形状。
    const i32 rotation = static_cast<i32>(state.get(BlockStateProperties::ROTATION_0_15()));
    const auto it = m_shapesByRotation.find(rotation);
    return it != m_shapesByRotation.end() ? it->second : m_defaultShape;
}

void CeilingHangingSignBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
}

const ResourceLocation& CeilingHangingSignBlock::getWaxedInteractFailSound() const
{
    // 悬挂告示牌返回专属音效
    // 对应 MC Java HangingSignBlockEntity.getSignInteractionFailedSoundEvent()
    return SoundEvents::BLOCK_HANGING_SIGN_WAXED_INTERACT_FAIL;
}

// ========== WallHangingSignBlock ==========

WallHangingSignBlock::WallHangingSignBlock(const BlockProperties& properties, const WoodType& woodType)
    : AbstractSignBlock(properties, woodType, woodType.hangingSignSoundType)
{
    // 墙面悬挂告示牌各方向的形状。
    //
    // 原版：
    //
    //   SHAPES_PLANK = rotateHorizontalAxis(Block.column(16.0, 4.0, 14.0, 16.0));
    //   SHAPES       = rotateHorizontalAxis(Shapes.or(SHAPES_PLANK.get(Axis.Z),
    //                                                 Block.column(14.0, 2.0, 0.0, 10.0)));
    //   getShape / getBlockSupportShape  ->  SHAPES.get(FACING.getAxis())
    //
    // `Block.column(w, d, yMin, yMax)` 的具体展开（见 Block.column 的源码：
    // `box(8 - w/2, yMin, 8 - d/2, 8 + w/2, yMax, 8 + d/2)`，单位为像素）：
    //
    //   column(16.0, 4.0, 14.0, 16.0)  ->  box(0, 14,  6, 16, 16, 10)   顶部横板
    //   column(14.0, 2.0,  0.0, 10.0)  ->  box(1,  0,  7, 15, 10,  9)   中心立柱
    //
    // **必须按原版的方式合成**：两个分量盒 OR 之后坐标列表取并集，得到细分的离散网格
    //
    //   X = [0, 0.0625, 0.9375, 1]
    //   Z = [0, 0.125, 0.25, 0.375, 0.4375, 0.5, 0.5625, 0.625, 0.75, 0.875, 1]
    //
    // 这个**网格密度**（而不只是外轮廓）决定了面切片的结果。举例：X 轴只有 3 格
    // （0..0.0625、0.0625..0.9375、0.9375..1），中间那一格大部分是空的；对 EAST 面做
    // `findIndex(X, 0.9999999)` 得到的切片落在没有体素的范围内，`getFaceShape` 返回空，
    // 于是原版的 `EAST|CENTER` 为 false。
    //
    // 若只用单个 `fromPixelBox` 画出外轮廓盒子，网格只有 2 个坐标、格子粗得多，
    // 切片必然非空，该面就会被误判为 true。
    constexpr f32 P = 1.0f / 16.0f;
    // 顶部横板：column(16.0, 4.0, 14.0, 16.0) -> box(0, 14, 6, 16, 16, 10)
    const CollisionShape plank = CollisionShape::box(0.0f, 14.0f * P, 6.0f * P, 1.0f, 1.0f, 10.0f * P);
    // 中心立柱：column(14.0, 2.0, 0.0, 10.0) -> box(1, 0, 7, 15, 10, 9)
    const CollisionShape pillar = CollisionShape::box(1.0f * P, 0.0f, 7.0f * P, 15.0f * P, 10.0f * P, 9.0f * P);

    // Z 轴朝向（FACING = NORTH / SOUTH）：先 OR 出细分网格
    const CollisionShape alongZ = CollisionShape::combine(plank, pillar, CollisionShape::CombineOp::OR);
    // X 轴朝向（FACING = EAST / WEST）：把上面两个分量盒绕 Y 轴旋转 90 度后再 OR
    const CollisionShape plankRot = CollisionShape::box(6.0f * P, 14.0f * P, 0.0f, 10.0f * P, 1.0f, 1.0f);
    const CollisionShape pillarRot = CollisionShape::box(7.0f * P, 0.0f, 1.0f * P, 9.0f * P, 10.0f * P, 15.0f * P);
    const CollisionShape alongX = CollisionShape::combine(plankRot, pillarRot, CollisionShape::CombineOp::OR);

    m_shapesByDirection[Direction::North] = alongZ;
    m_shapesByDirection[Direction::South] = alongZ;
    m_shapesByDirection[Direction::East] = alongX;
    m_shapesByDirection[Direction::West] = alongX;

    // 碰撞形状只取顶部横板（原版 SHAPES_PLANK），不含中心立柱。
    m_plankShapes[Direction::North] = plank;
    m_plankShapes[Direction::South] = plank;
    m_plankShapes[Direction::East] = plankRot;
    m_plankShapes[Direction::West] = plankRot;

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::WATERLOGGED())
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
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState WallHangingSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = waterloggable::isWaterFluidState(fluidState);

    for (Direction dir : context.getNearestLookingDirections()) {
        if (Directions::isHorizontal(dir)) {
            Direction facing = Directions::opposite(dir);
            BlockState state = defaultState()
                                   .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
                                   .with(BlockStateProperties::WATERLOGGED(), waterlogged);

            IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));
            if (isValidPosition(state, blockReader, pos)) {
                return state;
            }
        }
    }

    return defaultState();
}

const BlockState& WallHangingSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& WallHangingSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& WallHangingSignBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& WallHangingSignBlock::getCollisionShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    auto it = m_plankShapes.find(facing);
    if (it != m_plankShapes.end()) {
        return it->second;
    }
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

void WallHangingSignBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
}

const ResourceLocation& WallHangingSignBlock::getWaxedInteractFailSound() const
{
    // 墙面悬挂告示牌返回专属音效（与天花板悬挂告示牌相同）
    // 对应 MC Java HangingSignBlockEntity.getSignInteractionFailedSoundEvent()
    return SoundEvents::BLOCK_HANGING_SIGN_WAXED_INTERACT_FAIL;
}

bool WallHangingSignBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 该方块族原版显式返回 false：形状不完整不等于可以穿过。
    // 即使水logged=true 也不因此变得可通行。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

} // namespace blocks
} // namespace mc
