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

#include "WindChargeItem.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace item {

// ============================================================================
// 构造函数
// ============================================================================

WindChargeItem::WindChargeItem(const ItemProperties& properties)
    : Item(properties)
{}

// ============================================================================
// Item 接口重写
// ============================================================================

ItemActionResult WindChargeItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    ItemStack& heldStack = player.getHeldItem(hand);

    // 冷却检查：如果物品正在冷却中，不允许使用
    if (player.hasItemCooldown(this)) {
        return ItemActionResult::fail(heldStack);
    }

    // 创建风弹弹射物实体
    auto entity = std::make_unique<entity::WindChargeEntity>(EntityId(0));
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);

    // 将实体添加到世界（需在 shootFrom 之前，因为 shootFrom 可能访问世界引用）
    entity::WindChargeEntity* projectile = entity.get();
    world.spawnEntity(std::move(entity));

    // 设置投掷参数：速度1.5，散布1.0
    projectile->shootFrom(player, player.pitch(), player.yaw(), 0.0f, THROW_VELOCITY, THROW_INACCURACY);

    // 播放投掷音效
    world.playSound(SoundEvents::ENTITY_WIND_CHARGE_THROW,
        sound::SoundCategory::Neutral,
        player.position(),
        0.5f,
        0.4f / (world.getRandom().nextFloat() * 0.4f + 0.8f));

    // 设置冷却时间
    player.setItemCooldown(this, COOLDOWN_TICKS);

    // 消耗物品（非创造模式）
    if (!player.isCreative()) {
        heldStack.shrink(1);
    }

    return ItemActionResult::success(heldStack);
}

} // namespace item
} // namespace mc
