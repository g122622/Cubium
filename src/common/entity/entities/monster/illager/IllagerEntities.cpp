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

#include "IllagerEntities.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/LivingEntity.hpp"

namespace mc {

// PillagerEntity
std::unique_ptr<Entity> PillagerEntity::create(IWorld* world)
{
    return std::make_unique<PillagerEntity>(LegacyEntityType::Pillager, EntityId(0));
}

PillagerEntity::PillagerEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{}

void PillagerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // TODO: 实现弩攻击逻辑
    (void)target;
    (void)charge;
}

void PillagerEntity::onCrossbowLoadComplete(ItemStack& crossbow)
{
    // TODO: 实现弩装填完成逻辑
    (void)crossbow;
}

void PillagerEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge)
{
    // TODO: 实现弩射击逻辑
    (void)target;
    (void)crossbow;
    (void)charge;
}

void PillagerEntity::registerGoals()
{
    AbstractIllagerEntity::registerGoals();
    // TODO: 添加掠夺者特有AI（弩攻击等）
}

void PillagerEntity::registerAttributes()
{
    AbstractIllagerEntity::registerAttributes();

    // MC 1.16.5 PillagerEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    // MC 1.16.5: FOLLOW_RANGE = 32.0
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 32.0);
}

// VindicatorEntity
std::unique_ptr<Entity> VindicatorEntity::create(IWorld* world)
{
    return std::make_unique<VindicatorEntity>(LegacyEntityType::Vindicator, EntityId(0));
}

VindicatorEntity::VindicatorEntity(LegacyEntityType type, EntityId id)
    : AbstractIllagerEntity(type, id)
{}

void VindicatorEntity::registerGoals()
{
    AbstractIllagerEntity::registerGoals();
    // TODO: 添加卫道士特有AI（斧攻击等）
}

void VindicatorEntity::registerAttributes()
{
    AbstractIllagerEntity::registerAttributes();

    // MC 1.16.5 VindicatorEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    // MC 1.16.5: 基础攻击伤害为 5.0（铁斧额外 +3，总计 8）
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    // MC 1.16.5: FOLLOW_RANGE = 12.0
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 12.0);
}

} // namespace mc
