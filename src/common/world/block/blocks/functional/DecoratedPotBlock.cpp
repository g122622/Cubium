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

#include "common/world/block/blocks/functional/DecoratedPotBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include "common/world/effect/base/LevelEventId.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// DecoratedPotBlock
// ============================================================================

DecoratedPotBlock::DecoratedPotBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::CRACKED())
            .add(BlockStateProperties::WATERLOGGED())
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
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::CRACKED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 饰纹陶罐形状: 缩小为圆柱
    m_shape = CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15);
}

void DecoratedPotBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState DecoratedPotBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = defaultState()
                           .with(FACING(), Directions::opposite(context.horizontalDirection()))
                           .with(BlockStateProperties::CRACKED(), false)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState DecoratedPotBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& DecoratedPotBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool DecoratedPotBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 原版显式返回 false：碰撞形状不满格不代表可以穿过。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

const fluid::FluidState* DecoratedPotBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

const BlockSoundType& DecoratedPotBlock::getSoundType(const BlockState& state) const noexcept
{
    return state.get(BlockStateProperties::CRACKED()) ? BlockSoundTypes::DECORATED_POT_CRACKED
                                                      : BlockSoundTypes::DECORATED_POT;
}

std::unique_ptr<BlockEntity> DecoratedPotBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::DecoratedPotBlockEntity>(pos);
}

i32 DecoratedPotBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    BlockEntity* be = world.getBlockEntity(pos);
    if (be != nullptr && be->getType() == BlockEntityType::DecoratedPot) {
        auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(be);
        return potEntity->getComparatorSignal();
    }
    return 0;
}

BlockActionResult DecoratedPotBlock::useItemOn(ItemStack& stack,
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hit);

    // 副手不处理
    if (hand == Hand::OffHand) {
        return ActionResultType::TryEmptyHandInteraction;
    }

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::DecoratedPot) {
        return ActionResultType::Pass;
    }
    auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(blockEntity);

    // 客户端：直接返回成功以播放动画
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // === 服务端逻辑 ===
    ItemStack& heldItem = stack;

    // 空手：无物品可放入，交回 useWithoutItem（对齐 vanilla 返回 TRY_WITH_EMPTY_HAND）
    if (heldItem.isEmpty()) {
        return ActionResultType::TryEmptyHandInteraction;
    }

    ItemStack potItem = potEntity->getItem();

    // 检查是否可以放入：陶罐为空，或罐内物品与手持物品相同且未达到最大堆叠
    const bool canInsert =
        potItem.isEmpty() || (potItem.canMergeWith(heldItem) && potItem.getCount() < potItem.getMaxStackSize());

    // 物品无法放入：交回 useWithoutItem 处理负摇晃（对齐 vanilla 返回 TRY_WITH_EMPTY_HAND）
    if (!canInsert) {
        return ActionResultType::TryEmptyHandInteraction;
    }

    // 触发正摇晃动画
    potEntity->wobble(blockentity::DecoratedPotBlockEntity::WobbleStyle::Positive);

    // 分离1个物品（保留物品元数据如附魔、自定义名称等）
    ItemStack toInsert = heldItem.split(1);

    if (potItem.isEmpty()) {
        // 陶罐为空，直接设置
        potEntity->setItem(toInsert);
    } else {
        // 叠加到已有物品
        potItem.grow(1);
        potEntity->setItem(potItem);
    }

    // 创造模式不消耗手持物品
    if (player.isCreative()) {
        heldItem.grow(1);
    }

    // 播放插入音效
    // 音高随填充程度变化：0.7 + 0.5 * (count / maxStackSize)
    ItemStack newItem = potEntity->getItem();
    f32 pitch = 0.7f + 0.5f * (static_cast<f32>(newItem.getCount()) / static_cast<f32>(newItem.getMaxStackSize()));
    world.playSound(SoundEvents::BLOCK_DECORATED_POT_INSERT, sound::SoundCategory::Blocks, pos.center(), 1.0f, pitch);

    // 触发方块变化游戏事件
    world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);

    // 标记方块实体已修改
    potEntity->setChanged();

    // 通知红石比较器更新信号
    world::redstone::RedstoneSystem::instance().updateComparators(world, pos);

    return ActionResultType::Success;
}

BlockActionResult DecoratedPotBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(player);
    MC_UNUSED(hit);

    // 空手交互（对齐 vanilla DecoratedPotBlock.useWithoutItem）：负摇晃动画 + 插入失败音效。
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::DecoratedPot) {
        return ActionResultType::Pass;
    }
    auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(blockEntity);

    potEntity->wobble(blockentity::DecoratedPotBlockEntity::WobbleStyle::Negative);

    // 播放插入失败音效
    world.playSound(
        SoundEvents::BLOCK_DECORATED_POT_INSERT_FAIL, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);

    // 触发方块变化游戏事件
    world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);

    return ActionResultType::Success;
}

void DecoratedPotBlock::playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player)
{
    // 如果玩家手持的物品具有 BREAKS_DECORATED_POTS 标签（剑、斧、镐、铲、锄、三叉戟、重锤），
    // 且没有精准采集附魔（精准采集可阻止陶罐碎裂），
    // 则将陶罐设为 CRACKED 状态，使其掉落4个单独的陶片而非陶罐物品。
    if (!state.get(BlockStateProperties::CRACKED())) {
        const ItemStack& mainHandItem = player.getMainHandItem();
        if (!mainHandItem.isEmpty() && mainHandItem.getItem()->isIn(item::tag::ItemTags::BREAKS_DECORATED_POTS())) {
            if (!item::enchant::EnchantmentHelper::hasSilkTouch(mainHandItem)) {
                BlockState crackedState = state.with(BlockStateProperties::CRACKED(), true);
                world.setBlockState(pos, &crackedState, world::BlockUpdateFlags::UPDATE_NONE);
            }
        }
    }

    Block::playerWillDestroy(world, pos, state, player);
}

void DecoratedPotBlock::onProjectileHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
{
    MC_UNUSED(hitResult);

    if (world.isClientSide()) {
        return;
    }

    // MC Java: DecoratedPotBlock.onProjectileHit
    // 条件: projectile.mayInteract(serverlevel, blockpos) && projectile.mayBreak(serverlevel)
    // mayBreak 检查投射物是否属于 #minecraft:impact_projectiles 标签
    // 且 PROJECTILES_CAN_BREAK_BLOCKS 游戏规则为 true
    auto* projEntity = dynamic_cast<entity::ProjectileEntity*>(&projectile);
    if (projEntity != nullptr && projEntity->mayInteract(world, hitResult.blockPos()) && projEntity->mayBreak(world)) {
        BlockPos blockPos = hitResult.blockPos();
        if (!state.get(BlockStateProperties::CRACKED())) {
            BlockState crackedState = state.with(BlockStateProperties::CRACKED(), true);
            world.setBlockState(blockPos, &crackedState, world::BlockUpdateFlags::UPDATE_NONE);
        }
        // 将方块设为空气以触发 onBlockRemoved（掉落陶片）和方块移除
        world.setBlockState(blockPos, nullptr, world::BlockUpdateFlags::UPDATE_ALL);
    }
}

void DecoratedPotBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 掉落已迁至 DecoratedPotBlockEntity::preRemoveSideEffects（对齐 vanilla），由
    // ServerWorld::setBlockState 在方块实体被丢弃前调用，并受
    // UPDATE_SKIP_BLOCK_ENTITY_SIDEEFFECTS 门控——此处不再负责掉落，避免重复。
    Block::onBlockRemoved(world, pos, state, movedByPiston);
}

ItemStack DecoratedPotBlock::getCloneItemStack(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);

    // 中键选取：返回带有图案数据的陶罐物品
    if (world != nullptr && pos != nullptr) {
        BlockEntity* entity = world->getBlockEntity(*pos);
        if (entity != nullptr && entity->getType() == BlockEntityType::DecoratedPot) {
            auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(entity);
            return blockentity::createDecoratedPotItem(potEntity->getDecorations());
        }
    }

    // 如果无法获取方块实体，返回默认的空陶罐物品
    return Block::getCloneItemStack(state, world, pos);
}

} // namespace blocks
} // namespace mc
