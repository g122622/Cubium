#include "LingeringPotionItem.hpp"
#include "../../potion/PotionUtils.hpp"
#include "../../potion/Potions.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../entity/core/Entity.hpp"
#include <memory>

namespace mc {
namespace item {

// ========== LingeringPotionItem 实现 ==========

LingeringPotionItem::LingeringPotionItem(const ItemProperties& properties)
    : ThrowableItem(properties) {
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
    // 设置为滞留型
    entity->setLingering(true);

    // 生成实体到世界，并返回原始指针
    entity::ProjectileItemEntity* result = entity.get();
    world.spawnEntity(std::move(entity));
    return result;
}

void LingeringPotionItem::playThrowSound(Player& player) const {
    // MC 1.16.5: 滞留药水使用与喷溅药水相同的投掷音效
    math::Random rng(static_cast<u64>(player.id()) ^ static_cast<u64>(player.ticksExisted()));
    f32 pitch = 0.4f / (rng.nextFloat() * 0.4f + 0.8f);
    player.playSound(SoundEvents::ENTITY_SPLASH_POTION_THROW, 0.5f, pitch);
}

bool LingeringPotionItem::hasEffect(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

String LingeringPotionItem::getTranslationKey(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return String("item.minecraft.lingering_potion.effect.") + potion->baseName();
    }
    return String("item.minecraft.lingering_potion");
}

} // namespace item
} // namespace mc
