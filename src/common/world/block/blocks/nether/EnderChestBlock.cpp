/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the notice should be included in all
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

#include "common/world/block/blocks/nether/EnderChestBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/PlayerEnderChestInventory.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/EnderChestEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== EnderChestBlock 实现 ==========

EnderChestBlock::EnderChestBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器：HORIZONTAL_FACING + WATERLOGGED
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
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
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 末影箱形状：14x14底部（与普通箱子相同）
    m_shape = CollisionShape::fullBlock();
}

BlockState EnderChestBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 朝向玩家看向方向的反方向
    Direction facing = context.horizontalDirection();

    // 检查含水状态
    bool waterlogged = waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos());

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

const BlockState& EnderChestBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& EnderChestBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& EnderChestBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

std::unique_ptr<BlockEntity> EnderChestBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::EnderChestEntity>(pos);
}

BlockActionResult EnderChestBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 客户端只返回Success
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 检查方块上方是否有不透明方块阻挡末影箱打开（与普通箱子逻辑一致）
    BlockPos abovePos = pos.up();
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
        // 上方被阻挡，无法打开末影箱
        return ActionResultType::Success;
    }

    // 获取末影箱方块实体（用于动画和音效）
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::EnderChest) {
        auto* enderChest = static_cast<blockentity::EnderChestEntity*>(entity);

        // 关联末影箱方块实体到玩家的末影箱物品栏，用于开盖动画和距离检查
        player.enderChestInventory().setActiveChest(enderChest);

        // 打开末影箱容器菜单（Generic9x3，27格）
        // 参考 MC Java: EnderChestBlock.useWithoutItem() — 使用玩家的末影箱物品栏创建 ChestMenu
        if (world.openContainer(ContainerType::Generic9x3, pos, player)) {
            // 打开盖子动画和音效（通过 PlayerEnderChestInventory.startOpen 委托到 EnderChestEntity）
            player.enderChestInventory().startOpen(player);

            // 奖励统计
            player.awardCustomStat(ResourceLocation(stats::OPEN_ENDERCHEST), 1);

            // 打开末影箱时激怒附近能看到玩家的猪灵
            entity::PiglinAi::angerNearbyPiglins(world, player, true);

            return ActionResultType::Consume;
        }

        // 容器打开失败，清除关联
        player.enderChestInventory().setActiveChest(nullptr);
    }

    return ActionResultType::Pass;
}

BlockState EnderChestBlock::updatePostPlacement(const BlockState& state,
    Direction direction,
    const BlockState& neighborState,
    IWorld& world,
    const BlockPos& pos,
    const BlockPos& neighborPos)
{
    MC_UNUSED(neighborState);
    MC_UNUSED(neighborPos);

    // 含水方块需要调度流体tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, pos);
    }

    return Block::updatePostPlacement(state, direction, neighborState, world, pos, neighborPos);
}

const fluid::FluidState* EnderChestBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

} // namespace blocks
} // namespace mc
