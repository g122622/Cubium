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

#include "TropicalFishEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <optional>

namespace mc {

// ============================================================================
// 继承链标识（parent = AbstractGroupFishEntity::classInfo()）。
// TODO(实体同步对齐): vanilla 1.21.11 TropicalFish 有 DATA_VARIANT(Int,id17)，项目当前
// 用 m_variant 普通成员承载、不同步。本次仅补 classInfo 占位对齐 id 上限，DATA_VARIANT
// 同步留后续逐实体字段对齐任务。
// ============================================================================
const entity::EntityClassInfo& TropicalFishEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"TropicalFishEntity", &AbstractGroupFishEntity::classInfo()};
    return s_classInfo;
}

TropicalFishEntity::TropicalFishEntity(EntityInstanceId id)
    : AbstractGroupFishEntity(id)
{
    randomizeVariant();

    // 显式调用 registerData() 确保沿正确继承链注册（C++ 基类构造期虚函数不派发，
    // 参考 MobEntity/AbstractSkeletonEntity 模式；本类暂无同步字段，调用幂等）。
    registerData();
}

std::unique_ptr<Entity> TropicalFishEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TropicalFishEntity>(0);
}

TropicalFishEntity::FishShape TropicalFishEntity::getShape() const
{
    return static_cast<FishShape>(m_variant & SHAPE_MASK);
}

u8 TropicalFishEntity::getBaseColor() const
{
    return static_cast<u8>((m_variant & BASE_COLOR_MASK) >> 8);
}

u8 TropicalFishEntity::getPatternColor() const
{
    return static_cast<u8>((m_variant & PATTERN_COLOR_MASK) >> 16);
}

void TropicalFishEntity::randomizeVariant()
{
    math::Random& rng = getRandom();

    const u8 shape = static_cast<u8>(rng.nextInt(0, 11));
    const u8 baseColor = static_cast<u8>(rng.nextInt(0, 15));
    const u8 patternColor = static_cast<u8>(rng.nextInt(0, 15));

    m_variant = shape | (baseColor << 8) | (patternColor << 16);
}

void TropicalFishEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

std::optional<ResourceLocation> TropicalFishEntity::getFlopSound() const
{
    return SoundEvents::ENTITY_TROPICAL_FISH_FLOP;
}

std::optional<ResourceLocation> TropicalFishEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_TROPICAL_FISH_AMBIENT;
}

std::optional<ResourceLocation> TropicalFishEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_TROPICAL_FISH_DEATH;
}

std::optional<ResourceLocation> TropicalFishEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_TROPICAL_FISH_HURT;
}

} // namespace mc
