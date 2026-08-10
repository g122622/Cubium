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

#include "HorseEntity.hpp"

#include "DonkeyEntity.hpp"
#include "MuleEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/horse/CoatColors.hpp"
#include "common/entity/entities/passive/horse/CoatTypes.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/armor/HorseArmorItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <optional>

namespace mc {

namespace {

[[nodiscard]] constexpr i32 packHorseVariant(CoatColors color, CoatTypes type)
{
    return static_cast<i32>(getCoatColorId(color)) | (static_cast<i32>(getCoatTypeId(type)) << 8);
}

} // namespace

HorseEntity::HorseEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractHorseEntity(id, registry)
{
    randomizeAppearance();

    // 补调 registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。Horse 的 registerAttributes 设
    // MAX_HEALTH=getHorseHealth()。详见 AbstractHorseEntity 构造注释。
    registerAttributes();
}

std::unique_ptr<Entity> HorseEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<HorseEntity>(0, registry);
}

i32 HorseEntity::getVariant() const
{
    return packHorseVariant(m_color, m_marking);
}

void HorseEntity::setVariant(i32 variant)
{
    m_color = getCoatColorById(variant & 0xFF);
    m_marking = getCoatTypeById((variant >> 8) & 0xFF);
}

void HorseEntity::randomizeAppearance()
{
    math::Random random(ticksExisted());
    m_color = getCoatColorById(random.nextInt(COAT_COLORS_COUNT));
    m_marking = getCoatTypeById(random.nextInt(COAT_TYPES_COUNT));
}

bool HorseEntity::isTameItem(const ItemStack& /*itemStack*/) const
{
    return false;
}

bool HorseEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 用于 TemptGoal AI 目标（玩家手持食物时会被诱惑）
    // 注意：只有金苹果和金胡萝卜会触发繁殖（在 handleEating 中处理）
    return isFoodItem(itemStack);
}

bool HorseEntity::canMateWith(const AnimalEntity& other) const
{
    // 马可以与马或驴交配
    if (this == &other) {
        return false;
    }

    // 检查是否是马或驴
    const HorseEntity* otherHorse = dynamic_cast<const HorseEntity*>(&other);
    const DonkeyEntity* otherDonkey = dynamic_cast<const DonkeyEntity*>(&other);

    if (otherHorse == nullptr && otherDonkey == nullptr) {
        return false;
    }

    // 检查双方都满足繁殖条件（成体且不在爱心状态）
    return canBreed() && (otherHorse != nullptr ? otherHorse->canBreed() : otherDonkey->canBreed());
}

std::unique_ptr<AnimalEntity> HorseEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    math::Random rng(ticksExisted());

    // 检查配偶是否是驴（产生骡）
    const DonkeyEntity* partnerDonkey = dynamic_cast<const DonkeyEntity*>(&partner);

    if (partnerDonkey != nullptr) {
        // 马 + 驴 = 骡
        auto mule = std::make_unique<MuleEntity>(0, *registry);
        mule->setChild(true);
        mule->setPosition(x(), y(), z());

        // 遗传属性
        setOffspringAttributes(partner, *mule);
        return mule;
    }

    // 马 + 马 = 马
    auto baby = std::make_unique<HorseEntity>(0, *registry);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());

    // 遗传属性
    setOffspringAttributes(partner, *baby);

    // 遗传毛色和花纹：4/9 继承父本，4/9 继承母本，1/9 随机
    const HorseEntity* partnerHorse = dynamic_cast<const HorseEntity*>(&partner);
    if (partnerHorse != nullptr) {
        i32 colorRoll = rng.nextInt(9);
        CoatColors babyColor;
        if (colorRoll < 4) {
            babyColor = getColor(); // 继承父本
        } else if (colorRoll < 8) {
            babyColor = partnerHorse->getColor(); // 继承母本
        } else {
            babyColor = getCoatColorById(rng.nextInt(COAT_COLORS_COUNT)); // 随机
        }
        baby->setColor(babyColor);

        // 花纹遗传：2/5 继承父本，2/5 继承母本，1/5 随机
        i32 markingRoll = rng.nextInt(5);
        CoatTypes babyMarking;
        if (markingRoll < 2) {
            babyMarking = getMarking(); // 继承父本
        } else if (markingRoll < 4) {
            babyMarking = partnerHorse->getMarking(); // 继承母本
        } else {
            babyMarking = getCoatTypeById(rng.nextInt(COAT_TYPES_COUNT)); // 随机
        }
        baby->setMarking(babyMarking);
    }

    return baby;
}

void HorseEntity::tick()
{
    AbstractHorseEntity::tick();

    // 未驯服的马被骑乘时随机扬蹄（模拟不安行为）
    // MC 1.21.11: RunAroundLikeCrazyGoal 中的扬蹄逻辑已由 AI 目标处理，
    // 这里仅处理未驯服马匹的随机扬蹄（1/50 概率）
    if (!isTame() && isBeingRidden() && !isRearing()) {
        math::Random random(ticksExisted());
        if (random.nextFloat() < 0.02f) {
            makeHorseRear();
        }
    }
}

void HorseEntity::registerGoals()
{
    // 马完全使用 AbstractHorseEntity 的 AI 目标
    AbstractHorseEntity::registerGoals();
}

void HorseEntity::registerAttributes()
{
    AbstractHorseEntity::registerAttributes();
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, getHorseHealth());
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, getSpeed());
}

std::optional<ResourceLocation> HorseEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_HORSE_AMBIENT;
}

std::optional<ResourceLocation> HorseEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_HORSE_HURT;
}

std::optional<ResourceLocation> HorseEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_HORSE_DEATH;
}

std::optional<ResourceLocation> HorseEntity::getAngrySound() const
{
    // 基类 makeMad() 会先调用 makeHorseRear() 然后调用此方法获取音效并播放
    return SoundEvents::ENTITY_HORSE_ANGRY;
}

void HorseEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_HORSE_EAT, 1.0f, 1.0f);
}

void HorseEntity::playJumpSound()
{
    playSound(SoundEvents::ENTITY_HORSE_JUMP, 0.4f, 1.0f);
}

void HorseEntity::playAngrySound()
{
    playSound(SoundEvents::ENTITY_HORSE_ANGRY, 1.0f, 1.0f);
}

bool HorseEntity::isValidArmorForSlot(const ItemStack& item) const
{
    // 检查物品是否为 HorseArmorItem 实例
    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return false;
    }

    // 使用 dynamic_cast 检查是否为 HorseArmorItem
    return dynamic_cast<const item::items::HorseArmorItem*>(itemPtr) != nullptr;
}

ActionResultType HorseEntity::interactMob(Player& player, Hand hand)
{
    // 马在基类逻辑之前增加食物优先级和未驯服时的愤怒反应

    bool isSecondaryUse = !isChild() && isTame() && player.isSneaking();

    // 如果没有被骑乘且不是打开容器的场景
    if (!isBeingRidden() && !isSecondaryUse) {
        ItemStack& itemStack = player.getHeldItem(hand);
        const Item* item = itemStack.getItem();

        if (item != nullptr && !itemStack.isEmpty()) {
            // 1. 手持食物时优先喂食
            if (isFoodItem(itemStack)) {
                bool hadEffect = handleEating(&player, itemStack);
                if (hadEffect) {
                    if (m_world != nullptr && m_world->isClientSide()) {
                        return ActionResultType::Consume;
                    }
                    return ActionResultType::Success;
                }
            }

            // 2. 未驯服时让马愤怒
            if (!isTame()) {
                makeMad();
                return ActionResultType::Success;
            }
        }
        // 交给基类处理（鞍、马铠装备和骑乘）
        return AbstractHorseEntity::interactMob(player, hand);
    }

    // 被骑乘中或 Shift+已驯服，交给基类处理
    return AbstractHorseEntity::interactMob(player, hand);
}

} // namespace mc
