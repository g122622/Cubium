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

#include "SalmonEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"

namespace mc {

SalmonEntity::SalmonEntity(EntityInstanceId id)
    : AbstractGroupFishEntity(id)
{}

std::unique_ptr<Entity> SalmonEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SalmonEntity>(0);
}

void SalmonEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
}

std::optional<ResourceLocation> SalmonEntity::getFlopSound() const
{
    return SoundEvents::ENTITY_SALMON_FLOP;
}

std::optional<ResourceLocation> SalmonEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_SALMON_AMBIENT;
}

std::optional<ResourceLocation> SalmonEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_SALMON_DEATH;
}

std::optional<ResourceLocation> SalmonEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_SALMON_HURT;
}

} // namespace mc
