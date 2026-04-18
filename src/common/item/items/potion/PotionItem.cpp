#include "PotionItem.hpp"

#include "../../potion/PotionUtils.hpp"
#include "../../potion/Potions.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"

namespace mc {
namespace item {

/**
 * @brief 构造药水物品
 */
PotionItem::PotionItem(const ItemProperties& properties)
    : Item(properties) {
}

/**
 * @brief 获取使用时长
 */
i32 PotionItem::getUseDuration(const ItemStack& /*stack*/) const {
    return 32;
}

/**
 * @brief 获取使用动作
 */
UseAction PotionItem::getUseAction(const ItemStack& /*stack*/) const {
    return UseAction::Drink;
}

/**
 * @brief 使用完成
 */
ItemStack PotionItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr) {
        applyEffects(potion, entity, world);
    }

    stack.shrink(1);
    return stack;
}

/**
 * @brief 右键使用物品
 */
ItemActionResult PotionItem::onItemRightClick(IWorld& /*world*/, Player& /*player*/, Hand /*hand*/) {
    return ItemActionResult(ActionResultType::Pass, ItemStack());
}

/**
 * @brief 是否有药水效果
 */
bool PotionItem::hasEffect(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

/**
 * @brief 获取翻译键
 */
String PotionItem::getTranslationKey(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return String("item.minecraft.potion.effect.") + potion->baseName();
    }
    return Item::getTranslationKey(stack);
}

/**
 * @brief 将药水效果应用到实体
 */
void PotionItem::applyEffects(const potion::Potion* potion, Entity& entity, IWorld& /*world*/) {
    if (potion == nullptr) {
        return;
    }

    if (auto* livingEntity = dynamic_cast<LivingEntity*>(&entity)) {
        for (const auto& effect : potion->effects()) {
            if (effect.type() == entity::effect::EffectType::InstantHealth ||
                effect.type() == entity::effect::EffectType::InstantDamage) {
                continue;
            }

            entity::effect::EffectInstance newEffect(effect);
            livingEntity->addEffect(std::move(newEffect));
        }
        return;
    }

    if (auto* player = dynamic_cast<Player*>(&entity)) {
        for (const auto& effect : potion->effects()) {
            if (effect.type() == entity::effect::EffectType::InstantHealth ||
                effect.type() == entity::effect::EffectType::InstantDamage) {
                continue;
            }

            entity::effect::EffectInstance newEffect(effect);
            player->addEffect(std::move(newEffect));
        }
    }
}

} // namespace item
} // namespace mc
