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

#include "ThrowableItems.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../world/IWorld.hpp"
#include "../../core/ItemStack.hpp"
#include <memory>

namespace mc {
namespace item {

// ========== SnowballItem ==========

SnowballItem::SnowballItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* SnowballItem::createProjectile(
    IWorld& world, Player& player, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::SnowballEntity>(0);
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);

    // 返回原始指针，所有权转移到调用者
    entity::ProjectileItemEntity* result = entity.get();
    world.spawnEntity(std::move(entity));
    return result;
}

// ========== EggItem ==========

EggItem::EggItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* EggItem::createProjectile(IWorld& world, Player& player, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::EggEntity>(0);
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);

    entity::ProjectileItemEntity* result = entity.get();
    world.spawnEntity(std::move(entity));
    return result;
}

// ========== EnderPearlItem ==========

EnderPearlItem::EnderPearlItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* EnderPearlItem::createProjectile(
    IWorld& world, Player& player, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::EnderPearlEntity>(0);
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);

    entity::ProjectileItemEntity* result = entity.get();
    world.spawnEntity(std::move(entity));
    return result;
}

// ========== ExperienceBottleItem ==========

ExperienceBottleItem::ExperienceBottleItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* ExperienceBottleItem::createProjectile(
    IWorld& world, Player& player, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::ExperienceBottleEntity>(0);
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);

    entity::ProjectileItemEntity* result = entity.get();
    world.spawnEntity(std::move(entity));
    return result;
}

} // namespace item
} // namespace mc
