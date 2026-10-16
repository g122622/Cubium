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

#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <optional>

namespace mc {

// ==================== 同步数据参数静态成员初始化 ====================
// id 由 registerData 沿继承链分配为 17（续接 AbstractFish.FROM_BUCKET@16）。
entity::DataParameter<i32> SalmonEntity::DATA_TYPE_PARAM = entity::EntityDataManager::createKey<i32>();

// 继承链标识（parent = AbstractGroupFishEntity::classInfo()）。
const entity::EntityClassInfo& SalmonEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"SalmonEntity", &AbstractGroupFishEntity::classInfo()};
    return s_classInfo;
}

void SalmonEntity::registerData()
{
    // 先调用父类方法。AbstractGroupFishEntity 无 registerData override，解析为
    // AbstractFishEntity::registerData（注册 FROM_BUCKET@16），确保父链参数已注册。
    AbstractGroupFishEntity::registerData();

    // 标记当前正在注册 SalmonEntity 类的字段，使 registerParam 沿继承链分配 id
    // （续接 AbstractFish id16 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 DATA_TYPE 对齐 vanilla 1.21.11 Salmon.DATA_TYPE@17(Int，体型 0=small/1=medium/2=large)。
    // 默认值 1 = Salmon.Variant.DEFAULT.id()（DEFAULT = MEDIUM）。
    m_dataManager.registerParam(DATA_TYPE_PARAM, static_cast<i32>(Variant::Medium));
}

SalmonEntity::SalmonEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractGroupFishEntity(id, registry)
{
    // 显式调用 registerData() 确保沿正确继承链注册（C++ 基类构造期虚函数不派发，
    // 参考 MobEntity/AbstractSkeletonEntity 模式）。
    registerData();

    // 补调 registerAttributes：AbstractFishEntity 构造调基类版（vtable 指向基类），派生 override
    // 永不执行，须在派生类构造显式调用。Salmon 的 registerAttributes 设 MAX_HEALTH=3。
    registerAttributes();
}

std::unique_ptr<Entity> SalmonEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SalmonEntity>(0, registry);
}

void SalmonEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
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

ItemStack SalmonEntity::getBucketItemStack() const
{
    return ItemStack(Items::SALMON_BUCKET, 1);
}

void SalmonEntity::setVariant(Variant variant)
{
    m_dataManager.set(DATA_TYPE_PARAM, static_cast<i32>(variant));
    // 对齐 vanilla Salmon.setVariant → refreshDimensions()：体型改变后立即重算碰撞箱，
    // 否则 width()/height() 的新缩放要到下次尺寸刷新才生效。
    refreshDimensions();
}

f32 SalmonEntity::width() const
{
    // 基础尺寸取 EntityType 登记值（鲑鱼 0.7），再乘体型缩放（vanilla getDefaultDimensions().scale）。
    // entityType() 懒查询在早期（未 setTypeId）可能为 nullptr，此时回退到基类默认宽度。
    const entity::EntityType* type = entityType();
    const f32 baseWidth = type != nullptr ? type->size().width() : Entity::width();
    return baseWidth * variantScale(getVariant());
}

f32 SalmonEntity::height() const
{
    const entity::EntityType* type = entityType();
    const f32 baseHeight = type != nullptr ? type->size().height() : Entity::height();
    return baseHeight * variantScale(getVariant());
}

void SalmonEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 对齐 vanilla Salmon.finalizeSpawn：按权重 small 30 / medium 50 / large 15 随机决定体型。
    const i32 roll = world.getRandom().nextInt(VARIANT_TOTAL_WEIGHT);
    if (roll < VARIANT_SMALL_WEIGHT) {
        setVariant(Variant::Small);
    } else if (roll < VARIANT_SMALL_WEIGHT + VARIANT_MEDIUM_WEIGHT) {
        setVariant(Variant::Medium);
    } else {
        setVariant(Variant::Large);
    }

    AbstractGroupFishEntity::finalizeSpawn(world, difficulty, spawnReason);
}

void SalmonEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AbstractGroupFishEntity::addAdditionalSaveData(tag);
    // 对齐 vanilla：以字符串形式存 "type"（small/medium/large）。
    static constexpr const char* VARIANT_NAMES[] = {"small", "medium", "large"};
    const i32 id = static_cast<i32>(getVariant());
    tag.put(entity::serialization::nbt_keys::SALMON_TYPE, std::string(VARIANT_NAMES[id]));
}

Result<void> SalmonEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(AbstractGroupFishEntity::readAdditionalSaveData(tag));
    const std::string name =
        entity::serialization::nbt_helper::tryGetString(tag, entity::serialization::nbt_keys::SALMON_TYPE)
            .value_or("medium");
    if (name == "small") {
        setVariant(Variant::Small);
    } else if (name == "large") {
        setVariant(Variant::Large);
    } else {
        setVariant(Variant::Medium);
    }
    return Result<void>::ok();
}

} // namespace mc
