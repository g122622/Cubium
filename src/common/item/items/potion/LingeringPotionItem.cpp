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

#include "LingeringPotionItem.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/potion/ThrowablePotionItem.hpp"
#include "common/world/IWorld.hpp"

#include <memory>
#include <utility>

namespace mc {
namespace item {

// ========== LingeringPotionItem 实现 ==========

LingeringPotionItem::LingeringPotionItem(const ItemProperties& properties)
    : ThrowablePotionItem(properties)
{}

entity::ProjectileItemEntity* LingeringPotionItem::createProjectile(
    IWorld& world, Player& player, const ItemStack& stack) const
{
    auto entity = createProjectileEntity(world, stack);
    if (entity == nullptr) {
        return nullptr;
    }

    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);

    entity::ProjectileItemEntity* result = dynamic_cast<entity::ProjectileItemEntity*>(entity.get());
    world.spawnEntity(std::move(entity));
    return result;
}

} // namespace item
} // namespace mc
