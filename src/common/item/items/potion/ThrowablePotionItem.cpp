#include "ThrowablePotionItem.hpp"
#include "../../potion/PotionUtils.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"

namespace mc {
namespace item {

// ========== 构造函数 ==========

ThrowablePotionItem::ThrowablePotionItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{
}

// ========== Item 接口重写 ==========

bool ThrowablePotionItem::hasEffect(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

String ThrowablePotionItem::getTranslationKey(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return getEffectTranslationKeyPrefix() + potion->baseName();
    }
    return getBaseTranslationKey();
}

// ========== ThrowableItem 接口重写 ==========

void ThrowablePotionItem::playThrowSound(Player& player) const {
    // MC 1.16.5: 所有药水使用相同的投掷音效
    // 使用固定随机种子生成音调变化
    math::Random rng(static_cast<u64>(player.id()) ^ static_cast<u64>(player.ticksExisted()));
    f32 pitch = 0.4f / (rng.nextFloat() * 0.4f + 0.8f);
    player.playSound(SoundEvents::ENTITY_SPLASH_POTION_THROW, 0.5f, pitch);
}

} // namespace item
} // namespace mc
