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

#include "WindChargeItem.hpp"

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
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
    auto entity = std::make_unique<entity::WindChargeEntity>(EntityInstanceId(0));
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);
    entity->setTypeId(entity::EntityTypeKeys::WIND_CHARGE);

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

// ============================================================================
// ProjectileItem 接口实现
// ============================================================================

std::unique_ptr<entity::ProjectileEntity> WindChargeItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& /*stack*/,
    f32 directionX,
    f32 directionY,
    f32 directionZ) const
{
    auto entity = std::make_unique<entity::WindChargeEntity>(EntityInstanceId(0));
    entity->setWorld(&world);
    entity->setPosition(position.x, position.y, position.z);
    entity->setTypeId(entity::EntityTypeKeys::WIND_CHARGE);

    // 风弹在 asProjectile 中根据方向预设初速度
    // 使用 DispenseConfig 中的 power 来设置初速度
    auto config = getDispenseConfig();
    // 预设 deltaMovement，方向归一化后乘以 power
    f32 length = std::sqrt(directionX * directionX + directionY * directionY + directionZ * directionZ);
    if (length > 0.0f) {
        f32 nx = directionX / length;
        f32 ny = directionY / length;
        f32 nz = directionZ / length;
        entity->setVelocity(nx * config.power, ny * config.power, nz * config.power);
    }

    return entity;
}

void WindChargeItem::shoot(entity::ProjectileEntity& /*projectile*/,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/,
    f32 /*power*/,
    f32 /*uncertainty*/) const
{
    // 风弹在 asProjectile 中已预设 deltaMovement，不需要再调用 shoot()
}

} // namespace item
} // namespace mc
