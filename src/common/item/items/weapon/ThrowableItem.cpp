/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR USE OF
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ThrowableItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

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

// ========== ProjectileItem 接口实现 ==========

std::unique_ptr<entity::ProjectileEntity> ThrowableItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& stack,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/) const
{
    // 调用子类实现的 createProjectileEntity 创建弹射物实体
    auto entity = createProjectileEntity(world, stack);
    if (entity == nullptr) {
        return nullptr;
    }

    // 设置世界引用（部分实体方法如 getShooter() 依赖 m_world，
    // 需在 spawnEntity 之前设置）
    entity->setWorld(&world);

    // 设置位置（不设置发射者和方向，由调用方负责）
    entity->setPosition(position.x, position.y, position.z);

    return entity;
}

// ========== 投掷物品特有方法 ==========

void ThrowableItem::playThrowSound(Player& /*player*/) const
{
    // 子类可覆盖以播放特定音效
}

} // namespace item
} // namespace mc
