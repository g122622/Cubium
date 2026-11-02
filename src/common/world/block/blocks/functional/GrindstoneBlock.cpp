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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::ATTACH_FACE(), AttachFace::Wall));

    // 创建砂轮形状
    // 砂轮由底座/支架 + 两根立柱 + 砂轮组成
    constexpr f32 P = 1.0f / 16.0f;

    // 砂轮形状：直接按原版 `forAllBoxes` 的实测结果建模。三种附着面 × 两个轴共 6 种，
    // 其余朝向由这两种绕 Y 轴旋转 180 度得到（南北同形、东西同形）。
    // Floor_North
    CollisionShape _gsFloorNorth = CollisionShape::box(0.125f, 0.0f, 0.375f, 0.25f, 0.8125f, 0.625f);
    _gsFloorNorth = CollisionShape::combine(_gsFloorNorth,
        CollisionShape::box(0.75f, 0.0f, 0.375f, 0.875f, 0.8125f, 0.625f),
        CollisionShape::CombineOp::OR);
    _gsFloorNorth = CollisionShape::combine(
        _gsFloorNorth, CollisionShape::box(0.25f, 0.25f, 0.125f, 0.75f, 1.0f, 0.875f), CollisionShape::CombineOp::OR);
    _gsFloorNorth = CollisionShape::combine(_gsFloorNorth,
        CollisionShape::box(0.125f, 0.4375f, 0.3125f, 0.25f, 0.8125f, 0.375f),
        CollisionShape::CombineOp::OR);
    _gsFloorNorth = CollisionShape::combine(_gsFloorNorth,
        CollisionShape::box(0.125f, 0.4375f, 0.625f, 0.25f, 0.8125f, 0.6875f),
        CollisionShape::CombineOp::OR);
    _gsFloorNorth = CollisionShape::combine(_gsFloorNorth,
        CollisionShape::box(0.75f, 0.4375f, 0.3125f, 0.875f, 0.8125f, 0.375f),
        CollisionShape::CombineOp::OR);
    _gsFloorNorth = CollisionShape::combine(_gsFloorNorth,
        CollisionShape::box(0.75f, 0.4375f, 0.625f, 0.875f, 0.8125f, 0.6875f),
        CollisionShape::CombineOp::OR);

    // Floor_East
    CollisionShape _gsFloorEast = CollisionShape::box(0.375f, 0.0f, 0.125f, 0.625f, 0.8125f, 0.25f);
    _gsFloorEast = CollisionShape::combine(
        _gsFloorEast, CollisionShape::box(0.375f, 0.0f, 0.75f, 0.625f, 0.8125f, 0.875f), CollisionShape::CombineOp::OR);
    _gsFloorEast = CollisionShape::combine(
        _gsFloorEast, CollisionShape::box(0.125f, 0.25f, 0.25f, 0.875f, 1.0f, 0.75f), CollisionShape::CombineOp::OR);
    _gsFloorEast = CollisionShape::combine(_gsFloorEast,
        CollisionShape::box(0.3125f, 0.4375f, 0.125f, 0.375f, 0.8125f, 0.25f),
        CollisionShape::CombineOp::OR);
    _gsFloorEast = CollisionShape::combine(_gsFloorEast,
        CollisionShape::box(0.3125f, 0.4375f, 0.75f, 0.375f, 0.8125f, 0.875f),
        CollisionShape::CombineOp::OR);
    _gsFloorEast = CollisionShape::combine(_gsFloorEast,
        CollisionShape::box(0.625f, 0.4375f, 0.125f, 0.6875f, 0.8125f, 0.25f),
        CollisionShape::CombineOp::OR);
    _gsFloorEast = CollisionShape::combine(_gsFloorEast,
        CollisionShape::box(0.625f, 0.4375f, 0.75f, 0.6875f, 0.8125f, 0.875f),
        CollisionShape::CombineOp::OR);

    // Wall_North
    CollisionShape _gsWallNorth = CollisionShape::box(0.25f, 0.125f, 0.0f, 0.75f, 0.875f, 0.75f);
    _gsWallNorth = CollisionShape::combine(_gsWallNorth,
        CollisionShape::box(0.125f, 0.3125f, 0.1875f, 0.25f, 0.6875f, 0.5625f),
        CollisionShape::CombineOp::OR);
    _gsWallNorth = CollisionShape::combine(_gsWallNorth,
        CollisionShape::box(0.75f, 0.3125f, 0.1875f, 0.875f, 0.6875f, 0.5625f),
        CollisionShape::CombineOp::OR);
    _gsWallNorth = CollisionShape::combine(
        _gsWallNorth, CollisionShape::box(0.125f, 0.375f, 0.5625f, 0.25f, 0.625f, 1.0f), CollisionShape::CombineOp::OR);
    _gsWallNorth = CollisionShape::combine(
        _gsWallNorth, CollisionShape::box(0.75f, 0.375f, 0.5625f, 0.875f, 0.625f, 1.0f), CollisionShape::CombineOp::OR);

    // Wall_East
    CollisionShape _gsWallEast = CollisionShape::box(0.25f, 0.125f, 0.25f, 1.0f, 0.875f, 0.75f);
    _gsWallEast = CollisionShape::combine(_gsWallEast,
        CollisionShape::box(0.4375f, 0.3125f, 0.125f, 0.8125f, 0.6875f, 0.25f),
        CollisionShape::CombineOp::OR);
    _gsWallEast = CollisionShape::combine(_gsWallEast,
        CollisionShape::box(0.4375f, 0.3125f, 0.75f, 0.8125f, 0.6875f, 0.875f),
        CollisionShape::CombineOp::OR);
    _gsWallEast = CollisionShape::combine(
        _gsWallEast, CollisionShape::box(0.0f, 0.375f, 0.125f, 0.4375f, 0.625f, 0.25f), CollisionShape::CombineOp::OR);
    _gsWallEast = CollisionShape::combine(
        _gsWallEast, CollisionShape::box(0.0f, 0.375f, 0.75f, 0.4375f, 0.625f, 0.875f), CollisionShape::CombineOp::OR);

    // Ceiling_North
    CollisionShape _gsCeilingNorth = CollisionShape::box(0.25f, 0.0f, 0.125f, 0.75f, 0.75f, 0.875f);
    _gsCeilingNorth = CollisionShape::combine(_gsCeilingNorth,
        CollisionShape::box(0.125f, 0.1875f, 0.3125f, 0.25f, 0.5625f, 0.6875f),
        CollisionShape::CombineOp::OR);
    _gsCeilingNorth = CollisionShape::combine(_gsCeilingNorth,
        CollisionShape::box(0.75f, 0.1875f, 0.3125f, 0.875f, 0.5625f, 0.6875f),
        CollisionShape::CombineOp::OR);
    _gsCeilingNorth = CollisionShape::combine(_gsCeilingNorth,
        CollisionShape::box(0.125f, 0.5625f, 0.375f, 0.25f, 1.0f, 0.625f),
        CollisionShape::CombineOp::OR);
    _gsCeilingNorth = CollisionShape::combine(_gsCeilingNorth,
        CollisionShape::box(0.75f, 0.5625f, 0.375f, 0.875f, 1.0f, 0.625f),
        CollisionShape::CombineOp::OR);

    // Ceiling_East
    CollisionShape _gsCeilingEast = CollisionShape::box(0.125f, 0.0f, 0.25f, 0.875f, 0.75f, 0.75f);
    _gsCeilingEast = CollisionShape::combine(_gsCeilingEast,
        CollisionShape::box(0.3125f, 0.1875f, 0.125f, 0.6875f, 0.5625f, 0.25f),
        CollisionShape::CombineOp::OR);
    _gsCeilingEast = CollisionShape::combine(_gsCeilingEast,
        CollisionShape::box(0.3125f, 0.1875f, 0.75f, 0.6875f, 0.5625f, 0.875f),
        CollisionShape::CombineOp::OR);
    _gsCeilingEast = CollisionShape::combine(_gsCeilingEast,
        CollisionShape::box(0.375f, 0.5625f, 0.125f, 0.625f, 1.0f, 0.25f),
        CollisionShape::CombineOp::OR);
    _gsCeilingEast = CollisionShape::combine(_gsCeilingEast,
        CollisionShape::box(0.375f, 0.5625f, 0.75f, 0.625f, 1.0f, 0.875f),
        CollisionShape::CombineOp::OR);

    m_shapes[getShapeIndex(AttachFace::Floor, Direction::North)] = _gsFloorNorth;
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::South)] = _gsFloorNorth;
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::West)] = _gsFloorEast;
    m_shapes[getShapeIndex(AttachFace::Floor, Direction::East)] = _gsFloorEast;
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::North)] = _gsWallNorth;
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::South)] = _gsWallNorth;
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::West)] = _gsWallEast;
    m_shapes[getShapeIndex(AttachFace::Wall, Direction::East)] = _gsWallEast;
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::North)] = _gsCeilingNorth;
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::South)] = _gsCeilingNorth;
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::West)] = _gsCeilingEast;
    m_shapes[getShapeIndex(AttachFace::Ceiling, Direction::East)] = _gsCeilingEast;
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

BlockActionResult GrindstoneBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
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

std::optional<ContainerType> GrindstoneBlock::getMenuProvider(
    const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    // 有界面方块声明自身菜单类型，服务端旁观者右键据此统一打开（对齐 vanilla MenuProvider）
    (void)state;
    (void)world;
    (void)pos;
    return ContainerType::Grindstone;
}

} // namespace blocks
} // namespace mc
