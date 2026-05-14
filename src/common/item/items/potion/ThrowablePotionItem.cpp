/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#include "ThrowablePotionItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../potion/PotionUtils.hpp"

namespace mc {
namespace item {

// ========== 构造函数 ==========

ThrowablePotionItem::ThrowablePotionItem(const ItemProperties& properties)
    : ThrowableItem(properties)
{}

// ========== Item 接口重写 ==========

bool ThrowablePotionItem::hasEffect(const ItemStack& stack) const
{
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

std::string ThrowablePotionItem::getTranslationKey(const ItemStack& stack) const
{
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return getEffectTranslationKeyPrefix() + potion->baseName();
    }
    return getBaseTranslationKey();
}

// ========== ThrowableItem 接口重写 ==========

void ThrowablePotionItem::playThrowSound(Player& player) const
{
    // MC 1.16.5: 所有药水使用相同的投掷音效
    // 使用固定随机种子生成音调变化
    math::Random rng(static_cast<u64>(player.id()) ^ static_cast<u64>(player.ticksExisted()));
    f32 pitch = 0.4f / (rng.nextFloat() * 0.4f + 0.8f);
    player.playSound(SoundEvents::ENTITY_SPLASH_POTION_THROW, 0.5f, pitch);
}

} // namespace item
} // namespace mc
