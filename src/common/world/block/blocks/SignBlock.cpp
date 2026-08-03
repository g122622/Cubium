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

#include "SignBlock.hpp"
#include "../../../core/Types.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../IWorld.hpp"
#include "../../WorldEvents.hpp"
#include "../../blockentity/BlockEntityType.hpp"
#include "../../blockentity/core/BlockEntityRegistry.hpp"
#include "../../blockentity/interactive/SignEntity.hpp"
#include "../../gameevent/GameEvents.hpp"
#include "../IWaterLoggable.hpp"
#include "../WaterLoggableHelpers.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== AbstractSignBlock ==========

AbstractSignBlock::AbstractSignBlock(const BlockProperties& properties, WoodType woodType)
    : Block(properties)
    , m_woodType(woodType)
{
    // 子类负责创建状态容器
}

BlockState AbstractSignBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

std::unique_ptr<BlockEntity> AbstractSignBlock::createBlockEntity(const BlockPos& pos)
{
    return blockentity::BlockEntityRegistry::instance().create(BlockEntityType::Sign, pos);
}

BlockActionResult AbstractSignBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);
    MC_UNUSED(hit);

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Sign) {
        return ActionResultType::Pass;
    }

    auto* signEntity = static_cast<blockentity::SignEntity*>(blockEntity);

    // 检查玩家手持物品是否为蜜脾（涂蜡交互）
    ItemStack& heldItem = player.getHeldItem(hand);
    if (!heldItem.isEmpty() && heldItem.getItem() == Items::HONEYCOMB) {
        // 如果另一玩家正在编辑告示牌，阻止涂蜡交互
        // 对应 MC Java SignBlock.useItemOn() 中的 otherPlayerIsEditingSign() 检查
        if (signEntity->otherPlayerIsEditing(player)) {
            return ActionResultType::Consume;
        }
        // 涂蜡：如果告示牌未涂蜡，则设置涂蜡状态
        if (!signEntity->isWaxed()) {
            if (signEntity->setWaxed(true)) {
                // 播放涂蜡粒子与音效
                world.playEvent(world::WorldEvents::WAX_ON, pos, 0);

                // 涂蜡成功后触发方块变更游戏事件（通知附近的幽匿感测体等监听器）
                world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE,
                    pos,
                    gameevent::GameEvent::Context::of(static_cast<const Entity*>(&player), &state));

                // 涂蜡成功后记录玩家使用蜜脾的统计
                player.awardUsedStat(Items::HONEYCOMB->itemLocation(), 1);

                // 消耗一个蜜脾（非创造模式）
                if (!player.abilities().creativeMode) {
                    heldItem.shrink(1);
                }

                return ActionResultType::Success;
            }
        }
        // 告示牌已涂蜡或涂蜡失败，返回 Consume 防止物品被放置
        return ActionResultType::Consume;
    }

    // 已涂蜡的告示牌不可编辑，播放交互失败音效并仅执行命令
    // 对应 MC Java SignBlock.useWithoutItem() 中 isWaxed() 分支：
    //   serverlevel.playSound(null, signblockentity.getBlockPos(),
    //       signblockentity.getSignInteractionFailedSoundEvent(), SoundSource.BLOCKS);
    if (signEntity->isWaxed()) {
        world.playSound(getWaxedInteractFailSound(), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    }

    // 执行告示牌上的命令
    signEntity->executeCommand(world, player);

    // 如果另一玩家正在编辑告示牌，不允许打开编辑器
    // 对应 MC Java SignBlock.useWithoutItem() 中的 otherPlayerIsEditingSign() 检查
    // 注意：命令执行（click_events）不受编辑锁影响，即使其他玩家正在编辑也应正常执行
    if (signEntity->otherPlayerIsEditing(player)) {
        return ActionResultType::Pass;
    }

    // 检查玩家是否有建造权限
    if (!player.mayBuild()) {
        return ActionResultType::Pass;
    }

    // 检查告示牌文本是否可编辑（涂蜡的告示牌不可编辑）
    // 对应 MC Java SignBlock.useWithoutItem() 中的 hasEditableText() 检查
    if (!signEntity->hasEditableText()) {
        return ActionResultType::Pass;
    }

    // 设置编辑锁，防止其他玩家同时编辑
    // 对应 MC Java SignBlock.openTextEdit() 中的 setAllowedPlayerEditor()
    signEntity->setAllowedPlayerEditor(player.uuid());

    // 打开告示牌编辑界面（发送 OpenSignEditorPacket 给客户端）
    // 对应 MC Java SignBlock.openTextEdit() 中的 player.openTextEdit()
    // 项目告示牌为单面文本模型，始终编辑正面
    player.openSignEditor(pos, true);

    return ActionResultType::Success;
}

const ResourceLocation& AbstractSignBlock::getWaxedInteractFailSound() const
{
    // 普通告示牌（站立/墙面）返回 WAXED_SIGN_INTERACT_FAIL
    // 对应 MC Java SignBlockEntity.getSignInteractionFailedSoundEvent()
    return SoundEvents::BLOCK_SIGN_WAXED_INTERACT_FAIL;
}

const fluid::FluidState* AbstractSignBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== StandingSignBlock ==========

StandingSignBlock::StandingSignBlock(const BlockProperties& properties, WoodType woodType)
    : AbstractSignBlock(properties, woodType)
    , m_shape(CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f))
{

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ROTATION_0_15())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(
        defaultState().with(BlockStateProperties::ROTATION_0_15(), 0).with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState StandingSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 根据玩家朝向计算旋转（16个方向）
    f32 yaw = context.getPlayerYaw();
    i32 rotation = static_cast<i32>(std::floor((180.0f + yaw) * 16.0f / 360.0f + 0.5f)) & 15;

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::ROTATION_0_15(), rotation)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool StandingSignBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 需要下方有固体支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    return belowState != nullptr && belowState->isSolidSide(world, belowPos, Direction::Up);
}

const BlockState& StandingSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& StandingSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const CollisionShape& StandingSignBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

// ========== WallSignBlock ==========

WallSignBlock::WallSignBlock(const BlockProperties& properties, WoodType woodType)
    : AbstractSignBlock(properties, woodType)
{

    // 各方向的碰撞形状（贴在墙面的薄板）
    m_shapesByDirection[Direction::North] = CollisionShape::box(0.0f, 0.28125f, 0.875f, 1.0f, 0.78125f, 1.0f);
    m_shapesByDirection[Direction::South] = CollisionShape::box(0.0f, 0.28125f, 0.0f, 1.0f, 0.78125f, 0.125f);
    m_shapesByDirection[Direction::East] = CollisionShape::box(0.0f, 0.28125f, 0.0f, 0.125f, 0.78125f, 1.0f);
    m_shapesByDirection[Direction::West] = CollisionShape::box(0.875f, 0.28125f, 0.0f, 1.0f, 0.78125f, 1.0f);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState WallSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = waterloggable::isWaterFluidState(fluidState);

    // 尝试找到可以附着的墙面
    for (Direction dir : context.getNearestLookingDirections()) {
        if (Directions::isHorizontal(dir)) {
            Direction facing = Directions::opposite(dir);
            BlockState state = defaultState()
                                   .with(BlockStateProperties::FACING(), facing)
                                   .with(BlockStateProperties::WATERLOGGED(), waterlogged);

            // 使用 IBlockReader 接口检查是否可放置
            IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));
            if (isValidPosition(state, blockReader, pos)) {
                return state;
            }
        }
    }

    // 无法放置，返回空状态（会让放置失败）
    return defaultState();
}

bool WallSignBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    Direction facing = state.get(BlockStateProperties::FACING());
    Direction oppositeDir = Directions::opposite(facing);
    BlockPos adjPos = pos.offset(oppositeDir);
    const BlockState* adjState = world.getBlockState(adjPos);

    return adjState != nullptr && adjState->isSolidSide(world, adjPos, facing);
}

const BlockState& WallSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const BlockState& WallSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const CollisionShape& WallSignBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
