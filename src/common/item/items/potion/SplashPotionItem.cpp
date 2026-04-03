#include "SplashPotionItem.hpp"
#include "../../potion/PotionUtils.hpp"
#include "../../potion/Potions.hpp"

namespace mc {
namespace item {

// ========== SplashPotionItem 实现 ==========

SplashPotionItem::SplashPotionItem(const ItemProperties& properties)
    : Item(properties) {
}

ItemActionResult SplashPotionItem::onItemRightClick(IWorld& /*world*/, Player& /*player*/, Hand /*hand*/) {
    // TODO: 投掷喷溅药水
    // 目前先简单处理，后续实现投掷物系统

    return ItemActionResult(ActionResultType::Pass, ItemStack());
}

bool SplashPotionItem::hasEffect(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

String SplashPotionItem::getTranslationKey(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return String("item.minecraft.splash_potion.effect.") + potion->baseName();
    }
    return String("item.minecraft.splash_potion");
}

void SplashPotionItem::applySplashEffects(const potion::Potion* potion, IWorld& /*world*/,
                                          const BlockPos& /*pos*/, f32 /*radius*/) {
    // TODO: 实现区域效果应用
}

} // namespace item
} // namespace mc
