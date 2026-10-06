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

#include "SkullBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/SkullBlockEntity.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
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

/// 猪灵站立头颅碰撞形状：中心 10x8 像素、高 8 像素
const CollisionShape& _piglinSkullShape()
{
    static const CollisionShape shape = CollisionShape::fromPixelBox(3.0f, 0.0f, 4.0f, 13.0f, 8.0f, 12.0f);
    return shape;
}

/// 构造墙壁头颅的形状表（按 North/South/West/East 顺序，占据靠墙一侧 8/16 深度）
void _buildWallShapes(CollisionShape out[4], f32 widthPixels)
{
    // vanilla WallSkullBlock: Shapes.rotateHorizontal(Block.boxZ(8.0, 8.0, 16.0))
    //   boxZ(8.0, 8.0, 16.0) 表示 X 宽 8 居中、Y[8,16]、Z[0,8]
    // 本项目 CollisionShape 用方块本地 0-1 坐标，故 /16。
    const f32 half = widthPixels / 2.0f / 16.0f;
    const f32 cx = 0.5f;
    // North: Z 靠 0 侧（贴北墙）；South: Z 靠 1 侧；West: X 靠 0 侧；East: X 靠 1 侧
    out[0] = CollisionShape::box(cx - half, 8.0f / 16.0f, 0.0f, cx + half, 1.0f, 0.5f);        // North
    out[1] = CollisionShape::box(cx - half, 8.0f / 16.0f, 0.5f, cx + half, 1.0f, 1.0f);        // South
    out[2] = CollisionShape::box(0.0f, 8.0f / 16.0f, cx - half, 0.5f, 1.0f, cx + half);        // West
    out[3] = CollisionShape::box(0.5f, 8.0f / 16.0f, cx - half, 1.0f, 1.0f, cx + half);        // East
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
// AbstractSkullBlock
// ============================================================================

AbstractSkullBlock::AbstractSkullBlock(SkullType type, const BlockProperties& properties)
    : Block(properties)
    , m_type(type)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::POWERED(), false));
}

std::unique_ptr<BlockEntity> AbstractSkullBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SkullBlockEntity>(pos);
}

BlockState AbstractSkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(
        BlockStateProperties::POWERED(), world::redstone::RedstonePower::isPowered(context.getWorld(), context.placementPos()));
}

void AbstractSkullBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    if (world.isClientSide()) {
        return;
    }

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    const bool powered = world::redstone::RedstonePower::isPowered(world, pos);
    if (powered != state->get(BlockStateProperties::POWERED())) {
        BlockState newState = state->with(BlockStateProperties::POWERED(), powered);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    }
}

// ============================================================================
// SkullBlock（站立）
// ============================================================================

SkullBlock::SkullBlock(SkullType type, const BlockProperties& properties)
    : AbstractSkullBlock(type, properties)
    , m_shape(_skullShape())
    , m_piglinShape(_piglinSkullShape())
{
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

    setDefaultState(defaultState().with(BlockStateProperties::POWERED(), false).with(BlockStateProperties::ROTATION_0_15(), 0));
}

const CollisionShape& SkullBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return (m_type == SkullType::Piglin) ? m_piglinShape : m_shape;
}

BlockState SkullBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 旋转值：根据玩家 yaw 换算为 0-15（每 22.5 度），对齐 vanilla RotationSegment.convertToSegment
    i32 rotation = static_cast<i32>(std::floor((180.0f + context.getPlayerYaw()) * 16.0f / 360.0f + 0.5f)) & 15;

    bool powered = world::redstone::RedstonePower::isPowered(context.getWorld(), context.placementPos());
    return defaultState().with(BlockStateProperties::POWERED(), powered).with(
        BlockStateProperties::ROTATION_0_15(), rotation);
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

// ============================================================================
// WallSkullBlock（贴墙）
// ============================================================================

WallSkullBlock::WallSkullBlock(SkullType type, const BlockProperties& properties)
    : AbstractSkullBlock(type, properties)
{
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
    // 对齐 vanilla WallSkullBlock.getStateForPlacement：遍历最近视线方向，取首个水平方向的
    // 反方向作为 FACING，且要求该方向的相邻方块不可替换（即有可附着的实心墙面）。
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

bool WallSkullBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction attachDir = Directions::opposite(facing);
    const BlockPos attachPos = pos.offset(attachDir);
    const BlockState* attachState = world.getBlockState(attachPos);
    if (attachState == nullptr || attachState->isAir()) {
        return false;
    }
    return attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full);
}

BlockState WallSkullBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 附着面消失时自毁
    Direction skullFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (facing == Directions::opposite(skullFacing)) {
        if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
            if (const BlockState* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

// ============================================================================
// PlayerHeadBlock / PlayerWallHeadBlock
// ============================================================================

PlayerHeadBlock::PlayerHeadBlock(const BlockProperties& properties)
    : SkullBlock(SkullType::Player, properties)
{}

void PlayerHeadBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    // 从物品的 SkullOwner 组件恢复档案写入方块实体（对齐 vanilla PlayerHeadBlock.setPlacedBy）
    if (BlockEntity* be = world.getBlockEntity(pos)) {
        auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(be);
        if (skull != nullptr) {
            const nlohmann::json* tag = stack.getTag();
            if (tag != nullptr && tag->contains("SkullOwner")) {
                auto result = skin::GameProfile::fromJson((*tag)["SkullOwner"]);
                if (result.success()) {
                    skull->setOwnerProfile(result.value());
                }
            }
        }
    }
}

PlayerWallHeadBlock::PlayerWallHeadBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::Player, properties)
{}

void PlayerWallHeadBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    if (BlockEntity* be = world.getBlockEntity(pos)) {
        auto* skull = dynamic_cast<blockentity::SkullBlockEntity*>(be);
        if (skull != nullptr) {
            const nlohmann::json* tag = stack.getTag();
            if (tag != nullptr && tag->contains("SkullOwner")) {
                auto result = skin::GameProfile::fromJson((*tag)["SkullOwner"]);
                if (result.success()) {
                    skull->setOwnerProfile(result.value());
                }
            }
        }
    }
}

// ============================================================================
// WitherSkullBlock / WitherWallSkullBlock
// ============================================================================

WitherSkullBlock::WitherSkullBlock(const BlockProperties& properties)
    : SkullBlock(SkullType::WitherSkeleton, properties)
{}

void WitherSkullBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(stack);
    checkSpawn(world, pos);
}

void WitherSkullBlock::checkSpawn(IWorld& world, const BlockPos& pos)
{
    // TODO: 凋灵生成结构检测（4 灵魂沙/灵魂土 T 形 + 3 凋灵骷髅头颅）尚未实现。
    //   需要 BlockPatternBuilder 构造 wither 图案（aisle "^^^"/"###"/"~#~"，
    //   '#' 匹配 wither_summon_base_blocks 标签，'^' 匹配凋灵骷髅头颅，'~' 匹配空气），
    //   匹配后清除图案方块并在中心生成 WitherBoss（Difficulty != PEACEFUL 时）。
    //   当前仅留占位，避免结构误判。待凋灵 Boss 实体与图案匹配链路接入后补全。
    MC_UNUSED(world);
    MC_UNUSED(pos);
}

WitherWallSkullBlock::WitherWallSkullBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::WitherSkeleton, properties)
{}

void WitherWallSkullBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(state);
    MC_UNUSED(stack);
    WitherSkullBlock::checkSpawn(world, pos);
}

// ============================================================================
// PiglinWallSkullBlock
// ============================================================================

PiglinWallSkullBlock::PiglinWallSkullBlock(const BlockProperties& properties)
    : WallSkullBlock(SkullType::Piglin, properties)
{
    _buildWallShapes(m_piglinShapes, 10.0f);
}

const CollisionShape& PiglinWallSkullBlock::getShape(const BlockState& state) const
{
    return m_piglinShapes[_facingIndex(state.get(BlockStateProperties::HORIZONTAL_FACING()))];
}

} // namespace blocks
} // namespace mc
