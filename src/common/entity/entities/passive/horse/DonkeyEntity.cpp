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

#include "DonkeyEntity.hpp"

#include "HorseEntity.hpp"
#include "MuleEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractChestedHorseEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {

DonkeyEntity::DonkeyEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractChestedHorseEntity(id, registry)
{
    setJumpStrength(0.5f);

    // 补调 registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。详见 AbstractHorseEntity 构造注释。
    registerAttributes();
}

std::unique_ptr<Entity> DonkeyEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<DonkeyEntity>(0, registry);
}

bool DonkeyEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 用于 TemptGoal AI 目标（玩家手持食物时会被诱惑）
    // 注意：只有金苹果和金胡萝卜会触发繁殖（在 handleEating 中处理）
    return isFoodItem(itemStack);
}

bool DonkeyEntity::canMateWith(const AnimalEntity& other) const
{
    // 驴可以与驴或马交配
    if (this == &other) {
        return false;
    }

    // 检查是否是驴或马
    const DonkeyEntity* otherDonkey = dynamic_cast<const DonkeyEntity*>(&other);
    const HorseEntity* otherHorse = dynamic_cast<const HorseEntity*>(&other);

    if (otherDonkey == nullptr && otherHorse == nullptr) {
        return false;
    }

    // 检查双方都满足繁殖条件（成体且不在爱心状态）
    return canBreed() && (otherDonkey != nullptr ? otherDonkey->canBreed() : otherHorse->canBreed());
}

std::unique_ptr<AnimalEntity> DonkeyEntity::spawnBaby(AnimalEntity& partner)
{
    // 驴 + 驴 = 驴，驴 + 马 = 骡

    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = m_world->entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 检查配偶是否是马（产生骡）
    const HorseEntity* partnerHorse = dynamic_cast<const HorseEntity*>(&partner);

    if (partnerHorse != nullptr) {
        // 驴 + 马 = 骡
        auto mule = std::make_unique<MuleEntity>(0, *registry);
        mule->setChild(true);
        mule->setPosition(x(), y(), z());

        // 遗传属性
        setOffspringAttributes(partner, *mule);
        return mule;
    }

    // 驴 + 驴 = 驴
    auto baby = std::make_unique<DonkeyEntity>(0, *registry);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());

    // 遗传属性
    setOffspringAttributes(partner, *baby);
    return baby;
}

void DonkeyEntity::registerGoals()
{
    // 驴完全使用 AbstractHorseEntity 的 AI 目标
    AbstractChestedHorseEntity::registerGoals();
}

void DonkeyEntity::registerAttributes()
{
    AbstractChestedHorseEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth > 0 ? m_horseHealth : 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed > 0 ? m_speed : 0.175f);
}

} // namespace mc
