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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "SpectralArrowItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/weapon/ArrowItem.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {
namespace item {

SpectralArrowItem::SpectralArrowItem(const ItemProperties& properties)
    : ArrowItem(properties)
{}

entity::AbstractArrowEntity* SpectralArrowItem::createArrow(
    IWorld& world, const ItemStack& /*stack*/, LivingEntity& shooter) const
{
    auto entity = entity::SpectralArrowEntity::create(&world);
    if (entity) {
        auto* arrow = dynamic_cast<entity::SpectralArrowEntity*>(entity.get());
        if (arrow) {
            arrow->setWorld(&world);
            arrow->setPosition(shooter.x(), shooter.y() + shooter.eyeHeight() - 0.1f, shooter.z());
            arrow->setShooter(&shooter);
            // 玩家射出的箭矢允许拾取
            if (dynamic_cast<Player*>(&shooter) != nullptr) {
                arrow->setPickupStatus(entity::PickupStatus::Allowed);
            }
        }
        // 释放所有权，返回裸指针（调用者负责管理）
        return static_cast<entity::AbstractArrowEntity*>(entity.release());
    }
    return nullptr;
}

bool SpectralArrowItem::isInfinite(const ItemStack& /*arrowStack*/, const ItemStack& /*bowStack*/, Player& player) const
{
    // MC 1.16.5: 光灵箭不受益于无限附魔
    return player.isCreative();
}

std::unique_ptr<entity::ProjectileEntity> SpectralArrowItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& /*stack*/,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/) const
{
    auto entity = entity::SpectralArrowEntity::create(&world);
    if (entity) {
        entity->setTypeId(entity::EntityTypeKeys::SPECTRAL_ARROW);
        entity->setPosition(position.x, position.y, position.z);
        auto* arrow = dynamic_cast<entity::SpectralArrowEntity*>(entity.get());
        if (arrow) {
            // 发射器发射的箭矢允许被玩家拾取
            arrow->setPickupStatus(entity::PickupStatus::Allowed);
        }
    }
    // SpectralArrowEntity 继承自 ProjectileEntity，安全的 unique_ptr 转换
    return std::unique_ptr<entity::ProjectileEntity>(static_cast<entity::ProjectileEntity*>(entity.release()));
}

} // namespace item
} // namespace mc
