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

#include "GlowSquidEntity.hpp"

#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"

namespace mc {

using namespace mc::entity::serialization;

// 分配同步数据参数的唯一 ID（对应 MC Java SynchedEntityData.defineId）
entity::DataParameter<i32> GlowSquidEntity::DATA_DARK_TICKS_REMAINING_PARAM =
    entity::EntityDataManager::createKey<i32>();

GlowSquidEntity::GlowSquidEntity(EntityInstanceId id)
    : SquidEntity(id)
{
    // 显式调用 registerData() 注册同步数据参数。
    // C++ 虚函数在基类构造函数中不会动态派发到派生类，因此 SquidEntity / WaterMobEntity
    // 等基类构造期间调用的 registerData() 只会调用基类版本，必须在此处显式调用。
    registerData();
}

std::unique_ptr<Entity> GlowSquidEntity::create(IWorld* /*world*/)
{
    return std::make_unique<GlowSquidEntity>(0);
}

void GlowSquidEntity::registerData()
{
    // 先调用父类方法，确保基类数据参数已注册
    SquidEntity::registerData();

    // 注册剩余暗化 tick 同步参数，默认值 0
    m_dataManager.registerParam(DATA_DARK_TICKS_REMAINING_PARAM, static_cast<i32>(0));
}

i32 GlowSquidEntity::getDarkTicksRemaining() const
{
    // 优先从 DataParameter 读取以获取同步值
    if (m_dataManager.hasParam(DATA_DARK_TICKS_REMAINING_PARAM.id())) {
        return m_dataManager.get<i32>(DATA_DARK_TICKS_REMAINING_PARAM);
    }
    return m_darkTicksRemaining;
}

void GlowSquidEntity::setDarkTicks(i32 ticks)
{
    m_darkTicksRemaining = ticks;
    m_dataManager.set(DATA_DARK_TICKS_REMAINING_PARAM, ticks);
}

std::optional<ResourceLocation> GlowSquidEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_GLOW_SQUID_AMBIENT;
}

std::optional<ResourceLocation> GlowSquidEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_GLOW_SQUID_HURT;
}

std::optional<ResourceLocation> GlowSquidEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_GLOW_SQUID_DEATH;
}

bool GlowSquidEntity::hurt(DamageSource& source, f32 amount)
{
    // 调用父类 hurt（会触发喷墨）；成功受伤后设置暗化状态
    bool flag = SquidEntity::hurt(source, amount);
    if (flag) {
        setDarkTicks(DARK_TICKS_ON_HURT);
    }
    return flag;
}

void GlowSquidEntity::tick()
{
    // 先调用父类 tick 处理喷墨计时、游泳行为等
    SquidEntity::tick();

    // 暗化计时器递减
    const i32 darkTicks = getDarkTicksRemaining();
    if (darkTicks > 0) {
        setDarkTicks(darkTicks - 1);
    }

    // 持续生成 GLOW 粒子（对应 MC Java GlowSquid.aiStep 中的 addParticle(GLOW, ...)）
    if (world() != nullptr && world()->isClientSide()) {
        math::Random& rng = getRandom();
        // getRandomX/Y/Z 等价：在实体 AABB 范围内随机偏移
        const f32 offsetX = (rng.nextFloat() - 0.5f) * width() * 1.2f;
        const f32 offsetY = rng.nextFloat() * height();
        const f32 offsetZ = (rng.nextFloat() - 0.5f) * width() * 1.2f;
        const Vector3 pos(
            static_cast<f32>(x()) + offsetX, static_cast<f32>(y()) + offsetY, static_cast<f32>(z()) + offsetZ);
        world()->addParticle(particle::ParticleTypeId::Glow, pos, Vector3(0.0f, 0.0f, 0.0f));
    }
}

void GlowSquidEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用父类方法保存父类数据
    SquidEntity::addAdditionalSaveData(tag);

    // 持久化剩余暗化 tick 数
    tag.put(nbt_keys::DARK_TICKS_REMAINING, getDarkTicksRemaining());
}

Result<void> GlowSquidEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用父类方法加载父类数据
    MC_TRY(SquidEntity::readAdditionalSaveData(tag));

    // 对应 MC Java GlowSquid.readAdditionalSaveData:
    //   this.setDarkTicks(p_480156_.getIntOr("DarkTicksRemaining", 0));
    // getIntOr 语义：键缺失时使用默认值 0，始终调用 setDarkTicks。
    // 这确保从旧存档（无此字段）加载时也能正确初始化为 0。
    const i32 darkTicks = nbt_helper::tryGetInt(tag, nbt_keys::DARK_TICKS_REMAINING).value_or(0);
    setDarkTicks(darkTicks);
    return Result<void>::ok();
}

} // namespace mc
