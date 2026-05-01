#include "ThrowableItems.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../item/Items.hpp"
#include "../../../entity/core/Entity.hpp"

namespace mc {
namespace item {

// ========== SnowballItem ==========

SnowballItem::SnowballItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{
}

entity::ProjectileItemEntity* SnowballItem::createProjectile(
    IWorld& /*world*/,
    Player& /*player*/,
    const ItemStack& /*stack*/) const
{
    // TODO: 创建 SnowballEntity 实体
    // 目前实体类未完全实现
    return nullptr;
}

// ========== EggItem ==========

EggItem::EggItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{
}

entity::ProjectileItemEntity* EggItem::createProjectile(
    IWorld& /*world*/,
    Player& /*player*/,
    const ItemStack& /*stack*/) const
{
    // TODO: 创建 EggEntity 实体
    // 目前实体类未完全实现
    return nullptr;
}

// ========== EnderPearlItem ==========

EnderPearlItem::EnderPearlItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{
}

entity::ProjectileItemEntity* EnderPearlItem::createProjectile(
    IWorld& /*world*/,
    Player& /*player*/,
    const ItemStack& /*stack*/) const
{
    // TODO: 创建 EnderPearlEntity 实体
    // 目前实体类未完全实现
    return nullptr;
}

// ========== ExperienceBottleItem ==========

ExperienceBottleItem::ExperienceBottleItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{
}

entity::ProjectileItemEntity* ExperienceBottleItem::createProjectile(
    IWorld& /*world*/,
    Player& /*player*/,
    const ItemStack& /*stack*/) const
{
    // TODO: 创建 ExperienceBottleEntity 实体
    // 目前实体类未完全实现
    return nullptr;
}

} // namespace item
} // namespace mc
