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
    auto entity = std::make_unique<entity::SnowballEntity>(LegacyEntityType::Snowball, 0);
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
    auto entity = std::make_unique<entity::EggEntity>(LegacyEntityType::Egg, 0);
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
    auto entity = std::make_unique<entity::EnderPearlEntity>(LegacyEntityType::EnderPearl, 0);
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
    auto entity = std::make_unique<entity::ExperienceBottleEntity>(LegacyEntityType::ExperienceBottle, 0);
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);

    entity::ProjectileItemEntity* result = entity.get();
    world.spawnEntity(std::move(entity));
    return result;
}

} // namespace item
} // namespace mc
