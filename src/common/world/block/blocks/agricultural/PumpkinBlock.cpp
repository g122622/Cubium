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

#include "common/world/block/blocks/agricultural/PumpkinBlock.hpp"

#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <optional>

namespace mc {
namespace blocks {

// ============================================================================
// PumpkinBlock
// ============================================================================

PumpkinBlock::PumpkinBlock(
    const Block* stem, const Block* attachedStem, const Block* carvedPumpkin, const BlockProperties& properties)
    : StemGrownBlock(properties)
    , m_stem(stem)
    , m_attachedStem(attachedStem)
    , m_carvedPumpkin(carvedPumpkin)
{}

// ============================================================================
// 南瓜雕刻功能
// ============================================================================

BlockActionResult PumpkinBlock::useItemOn(ItemStack& stack,
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);

    ItemStack& heldItem = stack;
    const Item* item = heldItem.getItem();
    if (item == nullptr || item != Items::SHEARS) {
        return ActionResultType::TryEmptyHandInteraction;
    }

    if (m_carvedPumpkin == nullptr) {
        return ActionResultType::TryEmptyHandInteraction;
    }

    // 计算雕刻南瓜的朝向：点上下两面时朝向取玩家的反方向。
    // 直接复用 `Directions::fromYRot`，手写四档 if 容易和它的边界（±45° 取最近象限）悄悄错开。
    Direction facing = hit.face();
    if (facing == Direction::Up || facing == Direction::Down) {
        facing = Directions::opposite(Directions::fromYRot(static_cast<f64>(player.yaw())));
    }

    // 播放雕刻音效
    world.playSound(SoundEvents::BLOCK_PUMPKIN_CARVE, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);

    // 将南瓜替换为雕刻南瓜
    const BlockState* carvedState = &m_carvedPumpkin->defaultState();
    std::optional<Direction> currentFacing = carvedState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    if (currentFacing.has_value()) {
        carvedState = &carvedState->with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // flag 11: 通知邻居 + 更新客户端
    world.setBlockState(pos, carvedState, world::BlockUpdateFlags::UPDATE_ALL_IMMEDIATE);

    // 用剪刀雕刻是「剪」这个动作，发 SHEAR 而不是 BLOCK_CHANGE ——
    // 幽匿感测体对剪切单独归类（和剪羊毛、剪蜂巢同一类）。
    world.gameEvent(gameevent::GameEvents::SHEAR, pos, carvedState);

    // 生成南瓜种子（4个）
    math::Random rng(static_cast<u64>(pos.x ^ pos.y ^ pos.z));

    f64 seedX = static_cast<f64>(pos.x) + 0.5 + static_cast<f64>(Directions::xOffset(facing)) * 0.65;
    f64 seedY = static_cast<f64>(pos.y) + 0.1;
    f64 seedZ = static_cast<f64>(pos.z) + 0.5 + static_cast<f64>(Directions::zOffset(facing)) * 0.65;

    ItemStack seedStack(*Items::PUMPKIN_SEEDS, 4);

    f32 vx = 0.05f * static_cast<f32>(Directions::xOffset(facing)) + static_cast<f32>(rng.nextDouble() * 0.02);
    f32 vy = 0.05f;
    f32 vz = 0.05f * static_cast<f32>(Directions::zOffset(facing)) + static_cast<f32>(rng.nextDouble() * 0.02);

    ItemDropHelper::spawnItemEntity(
        &world, seedStack, seedX, seedY, seedZ, vx, vy, vz, ItemEntity::DEFAULT_PICKUP_DELAY, "");

    // 消耗剪刀耐久度，若物品损坏则触发 onEquippedItemBroken 回调
    LivingEntity::hurtAndBreak(heldItem, 1, &player, LivingEntity::handToEquipmentSlot(hand));

    return ActionResultType::Success;
}

BlockActionResult PumpkinBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    // 本方块无空手交互分支。
    (void)state;
    (void)world;
    (void)pos;
    (void)player;
    (void)hit;
    return ActionResultType::Pass;
}

} // namespace blocks
} // namespace mc
