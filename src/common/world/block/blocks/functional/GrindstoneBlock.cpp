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

#include "GrindstoneBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 AttachFace
using AttachFace = BlockStateProperties::AttachFace;

// ========== GrindstoneBlock 实现 ==========

GrindstoneBlock::GrindstoneBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::ATTACH_FACE())
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
            .with(BlockStateProperties::ATTACH_FACE(), AttachFace::Wall));

    // 创建砂轮形状
    // 砂轮由底座/支架 + 两根立柱 + 砂轮组成
    constexpr f32 P = 1.0f / 16.0f;

    // ========== 地面附着形状 ==========
    // 地面附着时，支架立于地面，砂轮在支架之间
    // 北朝向 (facing=North): 支架在南北方向，砂轮从两侧夹住
    // 底座: (2, 0, 6) -> (14, 2, 10) - 踏板形状
    CollisionShape floorBase = CollisionShape::box(2.0f * P, 0.0f, 6.0f * P, 14.0f * P, 2.0f * P, 10.0f * P);
    // 左立柱 (沿Z轴方向): (4, 2, 7) -> (6, 13, 9)
    CollisionShape floorPostLeft = CollisionShape::box(4.0f * P, 2.0f * P, 7.0f * P, 6.0f * P, 13.0f * P, 9.0f * P);
    // 右立柱: (10, 2, 7) -> (12, 13, 9)
    CollisionShape floorPostRight = CollisionShape::box(10.0f * P, 2.0f * P, 7.0f * P, 12.0f * P, 13.0f * P, 9.0f * P);
    // 砂轮: (6, 4, 7.5) -> (10, 12, 8.5) - 简化为方块
    CollisionShape floorWheel = CollisionShape::box(6.0f * P, 4.0f * P, 7.0f * P, 10.0f * P, 12.0f * P, 9.0f * P);

    m_floorNorthShape = CollisionShape::combine(
        CollisionShape::combine(CollisionShape::combine(floorBase, floorPostLeft), floorPostRight), floorWheel);

    // ========== 墙面附着形状 ==========
    // 墙面附着时，支架平行于墙面，砂轮挂在支架上
    // 北朝向 (facing=North): 砂轮挂在北墙上，朝向北
    // 左立柱 (沿X轴方向): (0, 0, 0) -> (2, 14, 2)
    CollisionShape wallPostLeft = CollisionShape::box(0.0f, 0.0f, 0.0f, 2.0f * P, 14.0f * P, 2.0f * P);
    // 右立柱: (14, 0, 0) -> (16, 14, 2)
    CollisionShape wallPostRight = CollisionShape::box(14.0f * P, 0.0f, 0.0f, 16.0f * P, 14.0f * P, 2.0f * P);
    // 砂轮: (2, 4, 0) -> (14, 12, 2)
    CollisionShape wallWheel = CollisionShape::box(2.0f * P, 4.0f * P, 0.0f, 14.0f * P, 12.0f * P, 2.0f * P);

    m_wallNorthShape = CollisionShape::combine(CollisionShape::combine(wallPostLeft, wallPostRight), wallWheel);

    // ========== 天花板附着形状 ==========
    // 天花板附着时，支架从天花板垂下
    // 北朝向: 支架沿南北方向延伸
    // 顶座: (2, 14, 6) -> (14, 16, 10)
    CollisionShape ceilingBase = CollisionShape::box(2.0f * P, 14.0f * P, 6.0f * P, 14.0f * P, 16.0f * P, 10.0f * P);
    // 左立柱: (4, 3, 7) -> (6, 14, 9)
    CollisionShape ceilingPostLeft = CollisionShape::box(4.0f * P, 3.0f * P, 7.0f * P, 6.0f * P, 14.0f * P, 9.0f * P);
    // 右立柱: (10, 3, 7) -> (12, 14, 9)
    CollisionShape ceilingPostRight =
        CollisionShape::box(10.0f * P, 3.0f * P, 7.0f * P, 12.0f * P, 14.0f * P, 9.0f * P);
    // 砂轮: (6, 4, 7) -> (10, 12, 9)
    CollisionShape ceilingWheel = CollisionShape::box(6.0f * P, 4.0f * P, 7.0f * P, 10.0f * P, 12.0f * P, 9.0f * P);

    m_ceilingNorthShape = CollisionShape::combine(
        CollisionShape::combine(CollisionShape::combine(ceilingBase, ceilingPostLeft), ceilingPostRight), ceilingWheel);

    // ========== 生成所有朝向的形状 ==========
    // 索引计算: getShapeIndex(attachFace, facing)
    // FLOOR: 0-3 (North=0, South=1, West=2, East=3)
    // WALL: 4-7
    // CEILING: 8-11

    // 地面附着形状
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::North)] = m_floorNorthShape;
    // 南朝向: 绕Y轴旋转180度 (X/Z交换)
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)] =
        CollisionShape::box(2.0f * P, 0.0f, 6.0f * P, 14.0f * P, 2.0f * P, 10.0f * P); // 底座对称
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)] =
        CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)], floorPostLeft);
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)] =
        CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)], floorPostRight);
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)] =
        CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)], floorWheel);

    // 西朝向: 旋转90度 (X<->Z交换)
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::West)] =
        CollisionShape::box(6.0f * P, 0.0f, 2.0f * P, 10.0f * P, 2.0f * P, 14.0f * P);
    CollisionShape floorPostLeftW = CollisionShape::box(7.0f * P, 2.0f * P, 4.0f * P, 9.0f * P, 13.0f * P, 6.0f * P);
    CollisionShape floorPostRightW = CollisionShape::box(7.0f * P, 2.0f * P, 10.0f * P, 9.0f * P, 13.0f * P, 12.0f * P);
    CollisionShape floorWheelW = CollisionShape::box(7.0f * P, 4.0f * P, 6.0f * P, 9.0f * P, 12.0f * P, 10.0f * P);
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::West)] = CollisionShape::combine(
        CollisionShape::combine(
            CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Floor, Direction::West)], floorPostLeftW),
            floorPostRightW),
        floorWheelW);

    // 东朝向
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)] =
        CollisionShape::box(6.0f * P, 0.0f, 2.0f * P, 10.0f * P, 2.0f * P, 14.0f * P);
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)] =
        CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)], floorPostLeftW);
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)] =
        CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)], floorPostRightW);
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)] =
        CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)], floorWheelW);

    // 墙面附着形状
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::North)] = m_wallNorthShape;

    // 南朝向: 砂轮朝南，挂在南墙上
    CollisionShape wallPostLeftS = CollisionShape::box(0.0f, 0.0f, 14.0f * P, 2.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wallPostRightS = CollisionShape::box(14.0f * P, 0.0f, 14.0f * P, 16.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wallWheelS = CollisionShape::box(2.0f * P, 4.0f * P, 14.0f * P, 14.0f * P, 12.0f * P, 16.0f * P);
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::South)] =
        CollisionShape::combine(CollisionShape::combine(wallPostLeftS, wallPostRightS), wallWheelS);

    // 西朝向: 砂轮朝西，挂在西墙上
    CollisionShape wallPostLeftW2 = CollisionShape::box(0.0f, 0.0f, 0.0f, 2.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape wallPostRightW2 = CollisionShape::box(0.0f, 0.0f, 14.0f * P, 2.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wallWheelW = CollisionShape::box(0.0f, 4.0f * P, 2.0f * P, 2.0f * P, 12.0f * P, 14.0f * P);
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::West)] =
        CollisionShape::combine(CollisionShape::combine(wallPostLeftW2, wallPostRightW2), wallWheelW);

    // 东朝向: 砂轮朝东，挂在东墙上
    CollisionShape wallPostLeftE = CollisionShape::box(14.0f * P, 0.0f, 0.0f, 16.0f * P, 14.0f * P, 2.0f * P);
    CollisionShape wallPostRightE = CollisionShape::box(14.0f * P, 0.0f, 14.0f * P, 16.0f * P, 14.0f * P, 16.0f * P);
    CollisionShape wallWheelE = CollisionShape::box(14.0f * P, 4.0f * P, 2.0f * P, 16.0f * P, 12.0f * P, 14.0f * P);
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::East)] =
        CollisionShape::combine(CollisionShape::combine(wallPostLeftE, wallPostRightE), wallWheelE);

    // 天花板附着形状
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::North)] = m_ceilingNorthShape;

    // 南朝向
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::South)] =
        CollisionShape::box(2.0f * P, 14.0f * P, 6.0f * P, 14.0f * P, 16.0f * P, 10.0f * P);
    CollisionShape ceilingPostLeftS = CollisionShape::box(4.0f * P, 3.0f * P, 7.0f * P, 6.0f * P, 14.0f * P, 9.0f * P);
    CollisionShape ceilingPostRightS =
        CollisionShape::box(10.0f * P, 3.0f * P, 7.0f * P, 12.0f * P, 14.0f * P, 9.0f * P);
    CollisionShape ceilingWheelS = CollisionShape::box(6.0f * P, 4.0f * P, 7.0f * P, 10.0f * P, 12.0f * P, 9.0f * P);
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::South)] = CollisionShape::combine(
        CollisionShape::combine(
            CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::South)], ceilingPostLeftS),
            ceilingPostRightS),
        ceilingWheelS);

    // 西朝向
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::West)] =
        CollisionShape::box(6.0f * P, 14.0f * P, 2.0f * P, 10.0f * P, 16.0f * P, 14.0f * P);
    CollisionShape ceilingPostLeftW = CollisionShape::box(7.0f * P, 3.0f * P, 4.0f * P, 9.0f * P, 14.0f * P, 6.0f * P);
    CollisionShape ceilingPostRightW =
        CollisionShape::box(7.0f * P, 3.0f * P, 10.0f * P, 9.0f * P, 14.0f * P, 12.0f * P);
    CollisionShape ceilingWheelW = CollisionShape::box(7.0f * P, 4.0f * P, 6.0f * P, 9.0f * P, 12.0f * P, 10.0f * P);
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::West)] = CollisionShape::combine(
        CollisionShape::combine(
            CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::West)], ceilingPostLeftW),
            ceilingPostRightW),
        ceilingWheelW);

    // 东朝向
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::East)] =
        CollisionShape::box(6.0f * P, 14.0f * P, 2.0f * P, 10.0f * P, 16.0f * P, 14.0f * P);
    CollisionShape ceilingPostLeftE = CollisionShape::box(7.0f * P, 3.0f * P, 4.0f * P, 9.0f * P, 14.0f * P, 6.0f * P);
    CollisionShape ceilingPostRightE =
        CollisionShape::box(7.0f * P, 3.0f * P, 10.0f * P, 9.0f * P, 14.0f * P, 12.0f * P);
    CollisionShape ceilingWheelE = CollisionShape::box(7.0f * P, 4.0f * P, 6.0f * P, 9.0f * P, 12.0f * P, 10.0f * P);
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::East)] = CollisionShape::combine(
        CollisionShape::combine(
            CollisionShape::combine(m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::East)], ceilingPostLeftE),
            ceilingPostRightE),
        ceilingWheelE);
}

size_t GrindstoneBlock::getShapeIndex(AttachFace attachFace, Direction facing)
{
    size_t faceIndex = static_cast<size_t>(attachFace);
    size_t facingIndex = 0;
    switch (facing) {
        case Direction::North:
            facingIndex = 0;
            break;
        case Direction::South:
            facingIndex = 1;
            break;
        case Direction::West:
            facingIndex = 2;
            break;
        case Direction::East:
            facingIndex = 3;
            break;
        default:
            facingIndex = 0;
            break;
    }
    return faceIndex * 4 + facingIndex;
}

BlockState GrindstoneBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction clickedFace = context.getClickedFace();
    Direction horizontalFacing = context.horizontalDirection();

    AttachFace attachFace;
    Direction finalFacing = horizontalFacing;

    if (clickedFace == Direction::Up) {
        // 点击地面 -> 地面附着
        attachFace = AttachFace::Floor;
    } else if (clickedFace == Direction::Down) {
        // 点击天花板 -> 天花板附着
        attachFace = AttachFace::Ceiling;
    } else {
        // 点击墙面 -> 墙面附着
        attachFace = AttachFace::Wall;
        finalFacing = clickedFace;
    }

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), finalFacing)
        .with(BlockStateProperties::ATTACH_FACE(), attachFace);
}

bool GrindstoneBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    BlockPos supportPos;
    switch (attachFace) {
        case AttachFace::Floor:
            // 地面附着 -> 需要下方有固体方块
            supportPos = pos.down();
            break;
        case AttachFace::Ceiling:
            // 天花板附着 -> 需要上方有固体方块
            supportPos = pos.up();
            break;
        case AttachFace::Wall:
            // 墙面附着 -> 需要背面有固体方块
            supportPos = pos.offset(Directions::opposite(facing));
            break;
        default:
            return false;
    }

    const BlockState* supportState = world.getBlockState(supportPos);
    if (supportState == nullptr) {
        return false;
    }

    return supportState->isSolid();
}

BlockState GrindstoneBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    Direction grindstoneFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 计算支撑方块位置
    BlockPos supportPos;
    switch (attachFace) {
        case AttachFace::Floor:
            supportPos = currentPos.down();
            break;
        case AttachFace::Ceiling:
            supportPos = currentPos.up();
            break;
        case AttachFace::Wall:
            supportPos = currentPos.offset(Directions::opposite(grindstoneFacing));
            break;
    }

    // 检查附着的支撑是否还存在
    if (facingPos == supportPos) {
        if (!facingState.isSolid()) {
            // 支撑被移除，掉落砂轮物品
            const Block* block = &state.getBlock();
            if (block != nullptr) {
                const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*block);
                if (blockItem != nullptr) {
                    ItemStack dropStack(blockItem, 1);
                    math::Random rng;
                    ItemDropHelper::spawnItemEntity(&world,
                        dropStack,
                        static_cast<f64>(currentPos.x) + 0.5,
                        static_cast<f64>(currentPos.y) + 0.5,
                        static_cast<f64>(currentPos.z) + 0.5,
                        rng);
                }
            }
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const BlockState& GrindstoneBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& GrindstoneBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& GrindstoneBlock::getShape(const BlockState& state) const
{
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = getShapeIndex(attachFace, facing);
    MC_ASSERT(index < 12);
    return m_shapes[index];
}

const CollisionShape& GrindstoneBlock::getCollisionShape(const BlockState& state) const
{
    // 碰撞形状与渲染形状相同
    return getShape(state);
}

BlockActionResult GrindstoneBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    if (world.openContainer(ContainerType::Grindstone, pos, player)) {
        player.awardCustomStat(ResourceLocation(stats::INTERACT_WITH_GRINDSTONE), 1);
        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

} // namespace blocks
} // namespace mc
