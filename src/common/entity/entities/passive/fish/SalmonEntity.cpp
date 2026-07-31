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

// ============================================================================
// 继承链标识（parent = AbstractGroupFishEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& SalmonEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"SalmonEntity", &AbstractGroupFishEntity::classInfo()};
    return s_classInfo;
}

SalmonEntity::SalmonEntity(EntityInstanceId id)
    : AbstractGroupFishEntity(id)
{
    // 显式调用 registerData() 确保沿正确继承链注册（C++ 基类构造期虚函数不派发，
    // 参考 MobEntity/AbstractSkeletonEntity 模式；Salmon 无自身字段，调用幂等）。
    registerData();
}

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
