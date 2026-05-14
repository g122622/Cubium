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

#include "MuleEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

DonkeyEntity::DonkeyEntity(LegacyEntityType type, EntityId id)
    : AbstractChestedHorseEntity(type, id)
{
    setJumpStrength(0.5f);
}

std::unique_ptr<Entity> DonkeyEntity::create(IWorld* /*world*/)
{
    return std::make_unique<DonkeyEntity>(LegacyEntityType::Unknown, 0);
}

bool DonkeyEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // TODO: 对齐 1.16.5 的金苹果 / 金胡萝卜繁殖逻辑。
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> DonkeyEntity::spawnBaby(AnimalEntity& partner)
{
    // TODO: 对齐 1.16.5 的驴 x 马 -> 骡、驴 x 驴 -> 驴 的后代逻辑。
    (void)partner;
    return nullptr;
}

void DonkeyEntity::registerGoals()
{
    AbstractChestedHorseEntity::registerGoals();
    // TODO: 补齐驴专属 AI 目标。
}

void DonkeyEntity::registerAttributes()
{
    AbstractChestedHorseEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth > 0 ? m_horseHealth : 15.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed > 0 ? m_speed : 0.175f);
}

} // namespace mc
