#include "LingeringPotionItem.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include <memory>

namespace mc {
namespace item {

// ========== LingeringPotionItem 实现 ==========

LingeringPotionItem::LingeringPotionItem(const ItemProperties& properties)
    : ThrowablePotionItem(properties)
{
}

entity::ProjectileItemEntity* LingeringPotionItem::createProjectile(
    IWorld& world,
    Player& player,
    const ItemStack& stack) const
{
    // 创建药水实体（滞留型）
    auto entity = std::make_unique<entity::PotionEntity>(LegacyEntityType::Potion, 0);
    entity->setWorld(&world);
    entity->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
    entity->setShooter(&player);
    entity->setItemStack(stack);
    // 滞留药水设置为滞留型
    entity->setLingering(true);

    // 生成实体到世界，并返回原始指针
    entity::ProjectileItemEntity* result = entity.get();
    world.spawnEntity(std::move(entity));
    return result;
}

} // namespace item
} // namespace mc
