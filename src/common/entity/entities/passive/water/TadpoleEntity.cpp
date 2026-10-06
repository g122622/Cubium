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

#include "TadpoleEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include <algorithm>
#include <memory>
#include <optional>

namespace mc {

const entity::EntityClassInfo& TadpoleEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"TadpoleEntity", &AbstractFishEntity::classInfo()};
    return s_classInfo;
}

TadpoleEntity::TadpoleEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractFishEntity(id, registry)
{
    // 显式调用 registerData/registerAttributes：C++ 基类构造期虚函数不派发，须在派生类构造补调。
    registerData();
    registerAttributes();
}

std::unique_ptr<Entity> TadpoleEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<TadpoleEntity>(0, registry);
}

void TadpoleEntity::registerAttributes()
{
    AbstractFishEntity::registerAttributes();
    // 对齐 vanilla Tadpole.createAttributes：MOVEMENT_SPEED=1.0、MAX_HEALTH=6.0
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 1.0);
}

void TadpoleEntity::tick()
{
    AbstractFishEntity::tick();

    if (m_world != nullptr && !m_world->isClientSide()) {
        setAge(m_age + 1);
    }
}

void TadpoleEntity::setAge(i32 age)
{
    m_age = age;
    // 对齐 vanilla：达到成年年龄时转化为青蛙
    if (m_age >= TICKS_TO_BE_FROG) {
        // TODO: 转化为青蛙实体（对齐 vanilla Tadpole.ageUp → convertTo(EntityType.FROG)）。
        //   青蛙实体（FrogEntity）尚未在本项目实现（VanillaEntities 未注册 minecraft:frog），
        //   待青蛙实体实现后补 convertTo + finalizeSpawn + TADPOLE_GROW_UP 音效。
    }
}

i32 TadpoleEntity::getTicksLeftUntilAdult() const noexcept
{
    return std::max(0, TICKS_TO_BE_FROG - m_age);
}

void TadpoleEntity::ageUp(i32 ticks)
{
    setAge(m_age + ticks * 20);
}

std::optional<ResourceLocation> TadpoleEntity::getFlopSound() const
{
    return SoundEvents::ENTITY_TADPOLE_FLOP;
}

std::optional<ResourceLocation> TadpoleEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_TADPOLE_DEATH;
}

std::optional<ResourceLocation> TadpoleEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_TADPOLE_HURT;
}

ItemStack TadpoleEntity::getBucketItemStack() const
{
    return ItemStack(Items::TADPOLE_BUCKET, 1);
}

} // namespace mc
