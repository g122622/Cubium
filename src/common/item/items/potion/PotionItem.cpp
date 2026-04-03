#include "PotionItem.hpp"
#include "../../potion/PotionUtils.hpp"
#include "../../potion/Potions.hpp"
#include "../../../entity/core/LivingEntity.hpp"

namespace mc {
namespace item {

// ========== PotionItem 实现 ==========

PotionItem::PotionItem(const ItemProperties& properties)
    : Item(properties) {
}

i32 PotionItem::getUseDuration(const ItemStack& /*stack*/) const {
    return 32;
}

UseAction PotionItem::getUseAction(const ItemStack& /*stack*/) const {
    return UseAction::Drink;
}

ItemStack PotionItem::onItemUseFinish(ItemStack& stack, IWorld& world, LivingEntity& entity) {
    // 应用效果
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr) {
        applyEffects(potion, entity, world);
    }

    // 减少物品数量
    stack.shrink(1);

    return stack;
}

ItemActionResult PotionItem::onItemRightClick(IWorld& /*world*/, Player& /*player*/, Hand /*hand*/) {
    // TODO: 检查是否可以饮用
    // if (player.canEat(false)) {
    //     player.startUsingItem(hand);
    //     return ItemActionResult(ActionResultType::Success, stack);
    // }

    return ItemActionResult(ActionResultType::Pass, ItemStack());
}

bool PotionItem::hasEffect(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

String PotionItem::getTranslationKey(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return String("item.minecraft.potion.effect.") + potion->baseName();
    }
    return Item::getTranslationKey(stack);
}

void PotionItem::applyEffects(const potion::Potion* potion, LivingEntity& entity, IWorld& /*world*/) {
    if (potion == nullptr) {
        return;
    }

    const auto& effects = potion->effects();
    for (const auto& effect : effects) {
        // 瞬间效果立即应用
        if (effect.type() == entity::effect::EffectType::InstantHealth ||
            effect.type() == entity::effect::EffectType::InstantDamage) {
            // TODO: 实现瞬间治疗效果
        } else {
            // 非瞬间效果添加到实体
            entity::effect::EffectInstance newEffect(effect);
            entity.addEffect(std::move(newEffect));
        }
    }
}

} // namespace item
} // namespace mc
