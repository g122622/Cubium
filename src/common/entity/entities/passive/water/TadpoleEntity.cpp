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
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/ConversionParams.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
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
    // 对齐 vanilla Tadpole.setAge：达到成体年龄即变态为青蛙。
    if (m_age >= TICKS_TO_BE_FROG) {
        _metamorphoseIntoFrog();
    }
}

void TadpoleEntity::_metamorphoseIntoFrog()
{
    // 对齐 vanilla Tadpole.ageUp（Tadpole.java:221-230）：
    //   convertTo(EntityType.FROG, ConversionParams.single(this, false, false), frog -> {
    //       frog.finalizeSpawn(serverlevel, getCurrentDifficultyAt(frog.blockPosition()), CONVERSION, null);
    //       frog.setPersistenceRequired();
    //       frog.fudgePositionAfterSizeChange(getDimensions(getPose()));
    //       playSound(TADPOLE_GROW_UP, 0.15F, 1.0F);
    //   });
    // 原实体由 convertTo 静默移除（不掉落、不产生死亡经验）。
    IWorld* world = m_world;
    if (world == nullptr || world->isClientSide()) {
        return;
    }
    // 已在移除流程中（重复触发/区块卸载）时不再转化
    if (isRemoved()) {
        return;
    }

    const ConversionParams params = ConversionParams::single(*this, false, false);
    (void)convertTo(std::string(entity::EntityTypeKeys::FROG), params, [this, world](Entity& frog) {
        auto* frogMob = dynamic_cast<MobEntity*>(&frog);
        if (frogMob == nullptr) {
            // 目标类型不是生物（注册表损坏），直接结束，避免访问空成员
            return;
        }
        entity::combat::DifficultyInstance difficulty = entity::combat::DifficultyInstance::at(*world, frog.onPos());
        frogMob->finalizeSpawn(*world, difficulty, world::spawn::SpawnReason::Conversion);
        // 转化产生的实体必须持久化，避免被消失管理器立刻清除
        frogMob->enablePersistence();
        // 尺寸变化后的位置修正：以原实体中心为基准重新贴地（vanilla fudgePositionAfterSizeChange）
        frogMob->refreshDimensions();
        // 音效由原实体播放：tick 推进的转化发生在原实体生命周期的最后一刻，
        // 位置一致，保持与原版的发声来源相同。
        playSound(SoundEvents::ENTITY_TADPOLE_GROW_UP, 0.15f, 1.0f);
    });
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

// ========== NBT ==========

void TadpoleEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    AbstractFishEntity::addAdditionalSaveData(tag);
    // 对齐 vanilla Tadpole.addAdditionalSaveData：putInt("Age", age)。
    tag.put(entity::serialization::nbt_keys::TADPOLE_AGE, m_age);
}

Result<void> TadpoleEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization;
    MC_TRY(AbstractFishEntity::readAdditionalSaveData(tag));
    // 对齐 vanilla Tadpole.readAdditionalSaveData：getIntOr("Age", 0) → setAge（达到阈值即变态）。
    if (const auto age =
            entity::serialization::nbt_helper::tryGetInt(tag, entity::serialization::nbt_keys::TADPOLE_AGE)) {
        setAge(*age);
    }
    return Result<void>::ok();
}

} // namespace mc
