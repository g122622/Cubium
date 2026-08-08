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

#include "CodEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include <memory>
#include <optional>

namespace mc {

// ============================================================================
// 继承链标识（parent = AbstractGroupFishEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& CodEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"CodEntity", &AbstractGroupFishEntity::classInfo()};
    return s_classInfo;
}

CodEntity::CodEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractGroupFishEntity(id, registry)
{
    // 显式调用 registerData() 确保沿正确继承链注册（C++ 基类构造期虚函数不派发，
    // 参考 MobEntity/AbstractSkeletonEntity 模式；Cod 无自身字段，调用幂等）。
    registerData();

    // 补调 registerAttributes：AbstractFishEntity 构造调基类版（vtable 指向基类），派生 override
    // 永不执行，须在派生类构造显式调用。Cod 的 registerAttributes 设 MAX_HEALTH=3。
    registerAttributes();
}

std::unique_ptr<Entity> CodEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<CodEntity>(0, registry);
}

void CodEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

std::optional<ResourceLocation> CodEntity::getFlopSound() const
{
    return SoundEvents::ENTITY_COD_FLOP;
}

std::optional<ResourceLocation> CodEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_COD_AMBIENT;
}

std::optional<ResourceLocation> CodEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_COD_DEATH;
}

std::optional<ResourceLocation> CodEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_COD_HURT;
}

} // namespace mc
