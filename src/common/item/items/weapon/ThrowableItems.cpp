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

#include "ThrowableItems.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {
namespace item {

// ========== SnowballItem ==========

SnowballItem::SnowballItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* SnowballItem::createProjectile(
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

std::unique_ptr<entity::ProjectileEntity> SnowballItem::createProjectileEntity(
    IWorld& /*world*/, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::SnowballEntity>(EntityInstanceId(0));
    entity->setTypeId(entity::EntityTypeKeys::SNOWBALL);
    return entity;
}

// ========== EggItem ==========

EggItem::EggItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* EggItem::createProjectile(IWorld& world, Player& player, const ItemStack& stack) const
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

std::unique_ptr<entity::ProjectileEntity> EggItem::createProjectileEntity(
    IWorld& /*world*/, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::EggEntity>(EntityInstanceId(0));
    entity->setTypeId(entity::EntityTypeKeys::EGG);
    return entity;
}

// ========== EnderPearlItem ==========

EnderPearlItem::EnderPearlItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* EnderPearlItem::createProjectile(
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

std::unique_ptr<entity::ProjectileEntity> EnderPearlItem::createProjectileEntity(
    IWorld& /*world*/, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::EnderPearlEntity>(EntityInstanceId(0));
    entity->setTypeId(entity::EntityTypeKeys::ENDER_PEARL);
    return entity;
}

// ========== ExperienceBottleItem ==========

ExperienceBottleItem::ExperienceBottleItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

entity::ProjectileItemEntity* ExperienceBottleItem::createProjectile(
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

std::unique_ptr<entity::ProjectileEntity> ExperienceBottleItem::createProjectileEntity(
    IWorld& /*world*/, const ItemStack& /*stack*/) const
{
    auto entity = std::make_unique<entity::ExperienceBottleEntity>(EntityInstanceId(0));
    entity->setTypeId(entity::EntityTypeKeys::EXPERIENCE_BOTTLE);
    return entity;
}

} // namespace item
} // namespace mc
