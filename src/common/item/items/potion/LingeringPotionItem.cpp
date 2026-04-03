#include "LingeringPotionItem.hpp"
#include "../../potion/PotionUtils.hpp"
#include "../../potion/Potions.hpp"

namespace mc {
namespace item {

// ========== LingeringPotionItem 实现 ==========

LingeringPotionItem::LingeringPotionItem(const ItemProperties& properties)
    : Item(properties) {
}

ItemActionResult LingeringPotionItem::onItemRightClick(IWorld& /*world*/, Player& /*player*/, Hand /*hand*/) {
    // TODO: 投掷滞留药水并创建滞留区域
    // 目前先简单处理，后续实现投掷物系统

    return ItemActionResult(ActionResultType::Pass, ItemStack());
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
