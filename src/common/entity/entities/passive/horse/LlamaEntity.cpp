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

#include "LlamaEntity.hpp"

#include "../../../../item/Items.hpp"
#include "../../../../item/core/Item.hpp"
#include "../../../../item/tag/ItemTags.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/AgeableEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../entities/player/Player.hpp"

#include <algorithm>

namespace mc {

LlamaEntity::LlamaEntity(LegacyEntityType type, EntityId id)
    : AbstractChestedHorseEntity(type, id)
{
    randomizeAppearance();
}

std::unique_ptr<Entity> LlamaEntity::create(IWorld* /*world*/)
{
    return std::make_unique<LlamaEntity>(LegacyEntityType::Unknown, 0);
}

void LlamaEntity::randomizeAppearance()
{
    math::Random random(ticksExisted());
    m_color = static_cast<LlamaColor>(random.nextInt(4));
    setStrength(1 + random.nextInt(5));
}

bool LlamaEntity::canBeRiddenBy(Player* player) const
{
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }

    return true;
}

i32 LlamaEntity::getInventoryColumns() const
{
    return m_strength;
}

void LlamaEntity::setStrength(i32 strength)
{
    m_strength = std::clamp(strength, 1, 5);
}

bool LlamaEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: LlamaEntity.isBreedingItem() 返回 isFoodItem()
    // 用于 TemptGoal AI 目标（玩家手持食物时会被诱惑）
    // 注意：只有干草块会触发繁殖（在 handleEating 中处理）
    return isFoodItem(itemStack);
}

bool LlamaEntity::isTameItem(const ItemStack& /*itemStack*/) const
{
    return false;
}

bool LlamaEntity::canMateWith(const AnimalEntity& other) const
{
    // MC 1.16.5: LlamaEntity.canMateWith(AnimalEntity otherAnimal)
    // 羊驼只能与羊驼交配
    if (this == &other) {
        return false;
    }

    const LlamaEntity* otherLlama = dynamic_cast<const LlamaEntity*>(&other);
    if (otherLlama == nullptr) {
        return false;
    }

    // 检查双方都满足繁殖条件（成体且不在爱心状态）
    return canBreed() && otherLlama->canBreed();
}

std::unique_ptr<AnimalEntity> LlamaEntity::spawnBaby(AnimalEntity& partner)
{
    // MC 1.16.5: LlamaEntity.func_241840_a(ServerWorld, AgeableEntity)
    math::Random rng(ticksExisted());

    auto baby = std::make_unique<LlamaEntity>(LegacyEntityType::Unknown, 0);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());

    // 遗传属性
    setOffspringAttributes(partner, *baby);

    // 遗传强度和颜色
    const LlamaEntity* partnerLlama = dynamic_cast<const LlamaEntity*>(&partner);
    if (partnerLlama != nullptr) {
        // 强度遗传：MC 1.16.5 取父母强度的最大值，然后随机 +1/+0
        i32 parentStrength = std::max(getStrength(), partnerLlama->getStrength());
        i32 babyStrength = parentStrength + rng.nextInt(1); // +0 或 +1
        baby->setStrength(babyStrength);

        // 颜色遗传：随机选择父本或母本的颜色
        if (rng.nextBoolean()) {
            baby->setColor(getColor());
        } else {
            baby->setColor(partnerLlama->getColor());
        }
    }

    return baby;
}

bool LlamaEntity::handleEating(Player* player, ItemStack& itemStack)
{
    // MC 1.16.5: LlamaEntity.handleEating()
    // 羊驼的食物效果与马不同：
    // - 小麦：治疗 2，成长 10 ticks，驯服 +3
    // - 干草块：治疗 10，成长 90 ticks，驯服 +6，可触发繁殖

    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    bool isWheat = (item == Items::WHEAT);
    bool isHayBlock = (item == Items::HAY_BLOCK);

    if (!isWheat && !isHayBlock) {
        return false;
    }

    i32 healAmount = 0;
    i32 growthTime = 0;
    i32 temperIncrease = 0;

    if (isWheat) {
        // MC 1.16.5: 小麦效果
        healAmount = 2;
        growthTime = 10;
        temperIncrease = 3;
    } else { // isHayBlock
        // MC 1.16.5: 干草块效果
        healAmount = 10;
        growthTime = 90;
        temperIncrease = 6;
    }

    bool hadEffect = false;

    // 治疗生命值
    if (health() < maxHealth()) {
        heal(static_cast<f32>(healAmount));
        hadEffect = true;
    }

    // 加速幼体成长
    if (isChild()) {
        addGrowingAge(growthTime);
        hadEffect = true;
    }

    // 增加驯服进度
    if (!isTame()) {
        increaseTemper(temperIncrease);
        hadEffect = true;
    }

    // 播放进食音效
    auto eatSound = getEatSound();
    if (eatSound.has_value()) {
        playSound(eatSound.value(), 1.0f, 1.0f);
    }

    // 触发繁殖（只有干草块可以触发繁殖）
    if (isHayBlock && getGrowingAge() == 0 && canBreed()) {
        setInLove();
        hadEffect = true;
    }

    // 消耗一个物品（非创造模式玩家）
    if (player != nullptr && !player->isCreative()) {
        itemStack.shrink(1);
    }

    return hadEffect;
}

bool LlamaEntity::isFoodItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: LlamaEntity.field_234243_bC_
    // 羊驼食物：小麦、干草块
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    return item == Items::WHEAT || item == Items::HAY_BLOCK;
}

std::optional<ResourceLocation> LlamaEntity::getEatSound() const
{
    return SoundEvents::ENTITY_LLAMA_EAT;
}

void LlamaEntity::tick()
{
    AbstractChestedHorseEntity::tick();

    if (m_spitCooldown > 0) {
        --m_spitCooldown;
    }

    if (m_inCaravan && m_caravanLeader != nullptr) {
        // TODO: 补齐商队跟随逻辑。
    }
}

void LlamaEntity::registerGoals()
{
    AbstractChestedHorseEntity::registerGoals();
    // TODO: 补齐羊驼的 FollowCaravan / RangedAttack 等 Goal。
}

void LlamaEntity::registerAttributes()
{
    AbstractChestedHorseEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f + static_cast<f32>(m_strength) * 5.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.175f);
}

bool LlamaEntity::isValidArmorForSlot(const ItemStack& item) const
{
    // MC 1.16.5: LlamaEntity.isArmor(ItemStack)
    // 检查物品是否在 ItemTags.CARPETS 中
    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return false;
    }

    return itemPtr->isIn(item::tag::ItemTags::CARPETS());
}

} // namespace mc
