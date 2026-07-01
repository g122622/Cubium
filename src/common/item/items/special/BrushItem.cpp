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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/item/items/special/BrushItem.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace item {

BrushItem::BrushItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType BrushItem::onItemUse(ItemUseContext& context)
{
    Player* player = context.getPlayer();
    if (player != nullptr) {
        // 玩家对准方块右键时，开始持续使用刷子
        player->setActiveHand(context.getHand());
    }
    return ActionResultType::Consume;
}

ItemActionResult BrushItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    MC_UNUSED(world);
    player.setActiveHand(hand);
    return ItemActionResult::success(
        player.getMutableEquipment(hand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand));
}

void BrushItem::onUseTick(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 elapsedTicks)
{
    MC_UNUSED(stack);

    // 仅玩家可以使用刷子
    auto* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        entity.stopActiveHand();
        return;
    }

    // 计算当前是否为刷扫触发tick
    // MC原版：每 ANIMATION_DURATION ticks 的第 BRUSH_TICK_IN_CYCLE tick 触发刷扫
    // elapsedTicks 是1-based的，MC原版逻辑为 (i % 10 == 5)，其中 i = useDuration - count + 1
    // 即 elapsedTicks 从1开始，当 (elapsedTicks % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1) 时触发
    bool shouldBrush = (elapsedTicks % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1);

    if (!shouldBrush) {
        return;
    }

    // TODO: 当玩家视线射线检测（raycast）系统完善后，需要检测玩家视线是否对准方块。
    // MC原版在 onUseTick 中调用 calculateHitResult 检查玩家视线，如果未对准方块则取消使用。
    // 当前实现中暂无法从 onUseTick 获取射线检测结果，因此先跳过此检查。

    // TODO: 当 BrushableBlockEntity 实现后，在此处添加完整的刷扫逻辑：
    // 1. 获取射线检测命中的方块位置
    // 2. 检查方块是否为 BrushableBlock
    // 3. 播放方块对应的刷扫音效（可疑沙: BRUSH_SAND, 可疑沙砾: BRUSH_GRAVEL, 其他: BRUSH_GENERIC）
    // 4. 获取 BrushableBlockEntity 并调用其 brush() 方法
    // 5. 如果刷扫成功，消耗1耐久: LivingEntity::hurtAndBreak(stack, 1, player, slot)
    // 6. 如果刷扫完成（brushCount 达到10），播放 BRUSH_BLOCK_COMPLETE 世界事件
    //    world.playEvent(WorldEvents::BRUSH_BLOCK_COMPLETE, blockPos, 0);
    // 7. 播放方块碎屑粒子

    // 简化实现：播放刷扫粒子效果（待 BrushableBlockEntity 实现后替换为完整逻辑）
    world.addParticle(particle::ParticleTypeId::DustPlume,
        entity.position() + Vector3(0.0f, entity.eyeHeight() * 0.5f, 0.0f),
        Vector3(0.0f, 0.1f, 0.0f));
}

bool BrushItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // TODO: 当 ArmadilloEntity 实现后，在此处添加刷犰狳的逻辑：
    // 1. 检查 target 是否为 ArmadilloEntity
    // 2. 检查犰狳是否处于可刷状态（scuteCooldown == 0 且非幼年）
    // 3. 掉落 armadillo_scute 物品
    // 4. 播放刷犰狳音效 (ARMADILLO_BRUSH)
    // 5. 消耗 ARMADILLO_DURABILITY_COST (16) 耐久:
    //    LivingEntity::hurtAndBreak(stack, ARMADILLO_DURABILITY_COST, &player,
    //        LivingEntity::handToEquipmentSlot(hand));
    // 6. 设置犰狳的 scuteCooldown
    //
    // MC原版逻辑参考: Armadillo.mobInteract() 和 DispenseItemBehavior 中的刷子逻辑

    MC_UNUSED(stack);
    MC_UNUSED(player);
    MC_UNUSED(target);
    MC_UNUSED(hand);
    return false;
}

i32 BrushItem::getUseDuration(const ItemStack& /*stack*/) const
{
    return USE_DURATION;
}

} // namespace item
} // namespace mc
