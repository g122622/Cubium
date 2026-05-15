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

#include "../../../../item/Items.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/items/armor/HorseArmorItem.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"

namespace mc {

namespace {

[[nodiscard]] constexpr i32 packHorseVariant(CoatColors color, CoatTypes type)
{
    return static_cast<i32>(getCoatColorId(color)) | (static_cast<i32>(getCoatTypeId(type)) << 8);
}

} // namespace

HorseEntity::HorseEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    randomizeAppearance();
}

std::unique_ptr<Entity> HorseEntity::create(IWorld* /*world*/)
{
    return std::make_unique<HorseEntity>(LegacyEntityType::Unknown, 0);
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
    // TODO: 对齐 1.16.5 的金苹果 / 金胡萝卜繁殖逻辑。
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> HorseEntity::spawnBaby(AnimalEntity& partner)
{
    // TODO: 对齐 1.16.5 的马 x 马 / 马 x 驴 后代外观与属性遗传逻辑。
    (void)partner;
    return nullptr;
}

void HorseEntity::tick()
{
    AbstractHorseEntity::tick();

    // 更新扬蹄计时器 - 使用基类的 STATUS_FLAG_REARING 进行网络同步
    if (m_isRearing) {
        --m_rearingCounter;
        if (m_rearingCounter <= 0) {
            m_isRearing = false;
            setRearing(false);
        }
    }

    // 未驯服的马被骑乘时随机扬蹄（模拟不安行为）
    // 注意：扬蹄并甩下玩家的完整逻辑在 RunAroundLikeCrazyGoal 中实现
    if (!isTame() && isBeingRidden() && !m_isRearing) {
        math::Random random(ticksExisted());
        if (random.nextFloat() < 0.02f) {
            m_isRearing = true;
            m_rearingCounter = 20;
            setRearing(true);
        }
    }
}

void HorseEntity::registerGoals()
{
    AbstractHorseEntity::registerGoals();
    // TODO: 补齐马的 RunAroundLikeCrazyGoal 等目标。
}

void HorseEntity::registerAttributes()
{
    AbstractHorseEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed);
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
    // MC 1.16.5: HorseEntity.getAngrySound() 返回 ENTITY_HORSE_ANGRY
    // 注意：基类 makeMad() 会先调用 makeHorseRear() 然后调用此方法获取音效并播放
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
    // MC 1.16.5: HorseEntity.isArmor(ItemStack)
    // 检查物品是否为 HorseArmorItem 实例
    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return false;
    }

    // 使用 dynamic_cast 检查是否为 HorseArmorItem
    return dynamic_cast<const item::items::HorseArmorItem*>(itemPtr) != nullptr;
}

} // namespace mc
