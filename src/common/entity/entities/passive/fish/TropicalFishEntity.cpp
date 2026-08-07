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
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <optional>

namespace mc {

// ==================== 同步数据参数静态成员初始化 ====================
// id 由 registerData 沿继承链分配为 17（续接 AbstractFish.FROM_BUCKET@16）。
entity::DataParameter<i32> TropicalFishEntity::DATA_VARIANT_PARAM = entity::EntityDataManager::createKey<i32>();

// 继承链标识（parent = AbstractGroupFishEntity::classInfo()）。
// vanilla 1.21.11 TropicalFish 自带 DATA_VARIANT@17(Int，packed: shape | baseColor<<8 | patternColor<<16)。
const entity::EntityClassInfo& TropicalFishEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"TropicalFishEntity", &AbstractGroupFishEntity::classInfo()};
    return s_classInfo;
}

void TropicalFishEntity::registerData()
{
    // 先调用父类方法。AbstractGroupFishEntity 无 registerData override，解析为
    // AbstractFishEntity::registerData（注册 FROM_BUCKET@16），确保父链参数已注册。
    AbstractGroupFishEntity::registerData();

    // 标记当前正在注册 TropicalFishEntity 类的字段，使 registerParam 沿继承链分配 id
    // （续接 AbstractFish id16 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 DATA_VARIANT 对齐 vanilla 1.21.11 TropicalFish.DATA_VARIANT@17(Int)。
    // 初始占位 0；构造函数中 randomizeVariant() 会经 setVariant 写入真实 packed id。
    m_dataManager.registerParam(DATA_VARIANT_PARAM, 0);
}

TropicalFishEntity::TropicalFishEntity(EntityInstanceId id)
    : AbstractGroupFishEntity(id)
{
    // 显式调用 registerData() 注册 DATA_VARIANT（C++ 基类构造期虚函数不派发，
    // Entity::Entity() 内部调用的 registerData() 解析到父类而非本类）。
    registerData();

    // 补调 registerAttributes：AbstractFishEntity 构造调基类版（vtable 指向基类），派生 override
    // 永不执行，须在派生类构造显式调用。TropicalFish 的 registerAttributes 设 MAX_HEALTH=3。
    registerAttributes();

    // 在 DATA_VARIANT_PARAM 注册完成后随机化变种，setVariant 会同步写入 DataParameter。
    randomizeVariant();
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
