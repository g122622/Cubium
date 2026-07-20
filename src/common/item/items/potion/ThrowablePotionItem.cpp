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

#include "common/item/items/potion/ThrowablePotionItem.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileItemEntity.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

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
    // 使用固定随机种子生成音调变化
    math::Random rng(static_cast<u64>(player.id()) ^ static_cast<u64>(player.ticksExisted()));
    f32 pitch = 0.4f / (rng.nextFloat() * 0.4f + 0.8f);
    player.playSound(SoundEvents::ENTITY_SPLASH_POTION_THROW, 0.5f, pitch);
}

// ========== ProjectileItem 接口实现 ==========

std::unique_ptr<entity::ProjectileEntity> ThrowablePotionItem::createProjectileEntity(
    IWorld& /*world*/, const ItemStack& stack) const
{
    auto entity = std::make_unique<entity::PotionEntity>(EntityInstanceId(0));
    entity->setItemStack(stack);
    entity->setLingering(isLingering());
    entity->setTypeId(entity::EntityTypeKeys::POTION);
    return entity;
}

} // namespace item
} // namespace mc
