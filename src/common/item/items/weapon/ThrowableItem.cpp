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

#include "ThrowableItem.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 构造函数 ==========

ThrowableItem::ThrowableItem(const ItemProperties& properties)
    : Item(properties)
{}

// ========== Item 接口重写 ==========

i32 ThrowableItem::getUseDuration(const ItemStack& /*stack*/) const
{
    return 0;
}

ItemActionResult ThrowableItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    ItemStack& heldStack = player.getHeldItem(hand);

    // 创建投掷实体
    entity::ProjectileItemEntity* projectile = createProjectile(world, player, heldStack);
    if (projectile == nullptr) {
        // 实体创建失败，只消耗物品
        if (!player.isCreative()) {
            heldStack.shrink(1);
        }
        return ItemActionResult::success(heldStack);
    }

    // 设置投掷参数
    // 参考 MC 1.16.5 ProjectileItemEntity.shoot()
    projectile->shootFrom(player,
        player.pitch(),
        player.yaw(),
        0.0f, // 滚动角
        getThrowVelocity(),
        getThrowInaccuracy());

    // 播放投掷音效
    playThrowSound(player);

    // 消耗物品（非创造模式）
    if (!player.isCreative()) {
        heldStack.shrink(1);
    }

    return ItemActionResult::success(heldStack);
}

// ========== 投掷物品特有方法 ==========

void ThrowableItem::playThrowSound(Player& /*player*/) const
{
    // 子类可覆盖以播放特定音效
}

} // namespace item
} // namespace mc
