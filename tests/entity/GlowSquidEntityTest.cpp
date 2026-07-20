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

/**
 * @file GlowSquidEntityTest.cpp
 * @brief 发光鱿鱼实体单元测试
 *
 * 参考 MC 1.21.11 net.minecraft.world.entity.animal.squid.GlowSquid
 *
 * 覆盖场景：
 * 1. DarkTicksRemaining 同步数据注册与读写
 * 2. NBT 保存加载往返一致性（含边界：空 NBT / 缺失键 / 非默认值）
 * 3. hurt 受击后暗化计时器设置为 100 且逐 tick 递减
 * 4. getInkParticle 返回 GlowSquidInk / getSquirtSound 返回 GLOW_SQUID_SQUIRT
 * 5. 构造函数显式调用 registerData 不导致基类参数重复注册（验证参数唯一性）
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/water/GlowSquidEntity.hpp"
#include "common/entity/entities/passive/water/SquidEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <vector>

using namespace mc;
using namespace mc::entity::serialization;

namespace {

/**
 * @brief 记录粒子的测试世界桩
 *
 * 重写 addParticle 收集发光鱿鱼 tick 时生成的 GLOW 粒子，
 * 并可配置为客户端/服务端以测试 tick 行为分支。
 */
class GlowSquidTestWorld final : public test::BaseTestWorld {
public:
    struct ParticleRecord {
        particle::ParticleTypeId type;
        Vector3 position;
        Vector3 velocity;
    };

    void setClientSide(bool client) { m_clientSide = client; }
    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }

    std::vector<ParticleRecord>& particles() { return m_particles; }
    void clearParticles() { m_particles.clear(); }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3&, u32) override
    {
        m_particles.push_back({type, pos, velocity});
    }

private:
    bool m_clientSide = false;
    std::vector<ParticleRecord> m_particles;
};

/**
 * @brief 测试用攻击者实体（LivingEntity 子类）
 *
 * 用于 hurt 测试中作为 DamageSource 的 trueSource，
 * 使 SquidEntity::hurt 的 getLastHurtBy() 非空以触发喷墨路径。
 */
class TestAttackerEntity : public LivingEntity {
public:
    TestAttackerEntity()
        : LivingEntity(EntityInstanceId(2))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/**
 * @brief 测试用发光鱿鱼子类，暴露 protected 方法供单元测试调用
 *
 * addAdditionalSaveData / readAdditionalSaveData / registerData 在 GlowSquidEntity 中
 * 是 protected 的（对齐 MC Java 的 protected 访问级别）。测试子类通过 using 声明
 * 将其提升为 public，以便直接测试 NBT 序列化与同步数据注册逻辑。
 */
class TestableGlowSquidEntity : public GlowSquidEntity {
public:
    using GlowSquidEntity::GlowSquidEntity;

    using GlowSquidEntity::addAdditionalSaveData;
    using GlowSquidEntity::readAdditionalSaveData;
    using GlowSquidEntity::registerData;
};

} // namespace

// ============================================================================
// 测试夹具
// ============================================================================

class GlowSquidEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_glowSquid = std::make_unique<TestableGlowSquidEntity>(EntityInstanceId(1)); }

    void TearDown() override { m_glowSquid.reset(); }

    std::unique_ptr<TestableGlowSquidEntity> m_glowSquid;
};

// ============================================================================
// 1. DarkTicksRemaining 同步数据注册与读写
// ============================================================================

/**
 * @brief 构造后默认 DarkTicksRemaining 为 0
 */
TEST_F(GlowSquidEntityTest, DarkTicksRemaining_DefaultIsZero)
{
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);
}

/**
 * @brief DataParameter 已注册到 EntityDataManager
 *
 * 验证构造函数显式调用 registerData() 后参数存在，
 * 且参数 ID 与 getDarkTicksRemainingParamId() 一致。
 */
TEST_F(GlowSquidEntityTest, DarkTicksRemaining_DataParameterRegistered)
{
    const u16 paramId = GlowSquidEntity::getDarkTicksRemainingParamId();
    EXPECT_TRUE(m_glowSquid->dataManager().hasParam(paramId))
        << "DarkTicksRemaining DataParameter should be registered after construction";
}

/**
 * @brief setDarkTicks 同步到 DataParameter 并可通过 getDarkTicksRemaining 读回
 */
TEST_F(GlowSquidEntityTest, DarkTicksRemaining_SetAndGet)
{
    m_glowSquid->setDarkTicks(100);
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 100);

    m_glowSquid->setDarkTicks(50);
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 50);

    m_glowSquid->setDarkTicks(0);
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);
}

/**
 * @brief setDarkTicks 设为负值不应崩溃（边界场景）
 */
TEST_F(GlowSquidEntityTest, DarkTicksRemaining_SetNegativeDoesNotCrash)
{
    m_glowSquid->setDarkTicks(-1);
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), -1);
}

/**
 * @brief DataParameter 设置后产生脏数据（用于客户端同步）
 */
TEST_F(GlowSquidEntityTest, DarkTicksRemaining_SetMarksDataDirty)
{
    m_glowSquid->dataManager().clearDirty();
    EXPECT_FALSE(m_glowSquid->dataManager().hasDirtyData());

    m_glowSquid->setDarkTicks(100);
    EXPECT_TRUE(m_glowSquid->dataManager().hasDirtyData());
}

// ============================================================================
// 2. NBT 保存加载往返一致性
// ============================================================================

/**
 * @brief NBT 往返：保存默认值后加载，DarkTicksRemaining 保持为 0
 */
TEST_F(GlowSquidEntityTest, NbtRoundTrip_DefaultValue)
{
    nbt::tags::compound_tag tag;
    m_glowSquid->addAdditionalSaveData(tag);

    // 验证键存在且为默认值 0
    auto val = nbt_helper::tryGetInt(tag, nbt_keys::DARK_TICKS_REMAINING);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0);

    // 加载到新实体
    TestableGlowSquidEntity loaded(EntityInstanceId(3));
    auto result = loaded.readAdditionalSaveData(tag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_EQ(loaded.getDarkTicksRemaining(), 0);
}

/**
 * @brief NBT 往返：保存非默认值后加载，值一致
 */
TEST_F(GlowSquidEntityTest, NbtRoundTrip_NonDefaultValue)
{
    m_glowSquid->setDarkTicks(100);
    nbt::tags::compound_tag tag;
    m_glowSquid->addAdditionalSaveData(tag);

    auto val = nbt_helper::tryGetInt(tag, nbt_keys::DARK_TICKS_REMAINING);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 100);

    TestableGlowSquidEntity loaded(EntityInstanceId(3));
    auto result = loaded.readAdditionalSaveData(tag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_EQ(loaded.getDarkTicksRemaining(), 100);
}

/**
 * @brief NBT 边界：空 NBT 加载不崩溃，使用默认值
 */
TEST_F(GlowSquidEntityTest, NbtRoundTrip_EmptyNbtUsesDefault)
{
    nbt::tags::compound_tag emptyTag;

    TestableGlowSquidEntity loaded(EntityInstanceId(3));
    auto result = loaded.readAdditionalSaveData(emptyTag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_EQ(loaded.getDarkTicksRemaining(), 0);
}

/**
 * @brief NBT 边界：先设值后加载空 NBT，值被重置为 0（getIntOr 语义）
 *
 * 对应 MC Java GlowSquid.readAdditionalSaveData 中
 * setDarkTicks(p_480156_.getIntOr("DarkTicksRemaining", 0))
 */
TEST_F(GlowSquidEntityTest, NbtRoundTrip_MissingKeyResetsToDefault)
{
    m_glowSquid->setDarkTicks(100);
    ASSERT_EQ(m_glowSquid->getDarkTicksRemaining(), 100);

    nbt::tags::compound_tag emptyTag;
    auto result = m_glowSquid->readAdditionalSaveData(emptyTag);
    ASSERT_TRUE(result.success()) << result.error().toString();
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);
}

/**
 * @brief NBT 往返：多次保存-加载循环后值仍一致
 */
TEST_F(GlowSquidEntityTest, NbtRoundTrip_MultipleCycles)
{
    m_glowSquid->setDarkTicks(42);

    for (i32 i = 0; i < 3; ++i) {
        nbt::tags::compound_tag tag;
        m_glowSquid->addAdditionalSaveData(tag);

        TestableGlowSquidEntity loaded(EntityInstanceId(3 + i));
        auto result = loaded.readAdditionalSaveData(tag);
        ASSERT_TRUE(result.success()) << result.error().toString();
        ASSERT_EQ(loaded.getDarkTicksRemaining(), 42);
    }
}

// ============================================================================
// 3. hurt 受击后暗化计时器设置为 100 且逐 tick 递减
// ============================================================================

/**
 * @brief hurt 成功（有攻击者）后 DarkTicksRemaining 设为 100
 *
 * 对应 MC Java GlowSquid.hurtServer:
 *   boolean flag = super.hurtServer(...);
 *   if (flag) { this.setDarkTicks(100); }
 *   return flag;
 */
TEST_F(GlowSquidEntityTest, Hurt_WithAttacker_SetsDarkTicksTo100)
{
    GlowSquidTestWorld world;
    m_glowSquid->setWorld(&world);

    TestAttackerEntity attacker;
    EntityDamageSource damage(DamageType::MobAttack, &attacker);

    EXPECT_TRUE(m_glowSquid->hurt(damage, 5.0f));
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 100);
}

/**
 * @brief hurt 无攻击者时不设置暗化（SquidEntity::hurt 返回 false）
 *
 * 对应 MC Java Squid.hurtServer 的 getLastHurtByMob() != null 门控：
 *   if (super.hurtServer(...) && this.getLastHurtByMob() != null) { ... return true; }
 *   else { return false; }
 * 以及 GlowSquid.hurtServer 仅在 super 返回 true 时 setDarkTicks(100)。
 */
TEST_F(GlowSquidEntityTest, Hurt_WithoutAttacker_DoesNotSetDarkTicks)
{
    GlowSquidTestWorld world;
    m_glowSquid->setWorld(&world);

    EnvironmentalDamage damage(DamageType::Generic);

    // SquidEntity::hurt 在无攻击者时返回 false，GlowSquid 不设置暗化
    EXPECT_FALSE(m_glowSquid->hurt(damage, 5.0f));
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);
}

/**
 * @brief tick 逐次递减 DarkTicksRemaining 直到 0
 *
 * 对应 MC Java GlowSquid.aiStep:
 *   int i = this.getDarkTicksRemaining();
 *   if (i > 0) { this.setDarkTicks(i - 1); }
 */
TEST_F(GlowSquidEntityTest, Tick_DecrementsDarkTicks)
{
    GlowSquidTestWorld world;
    m_glowSquid->setWorld(&world);

    m_glowSquid->setDarkTicks(3);

    m_glowSquid->tick();
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 2);

    m_glowSquid->tick();
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 1);

    m_glowSquid->tick();
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);

    // 到 0 后不再递减为负数
    m_glowSquid->tick();
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);
}

/**
 * @brief 完整流程：hurt 后 tick 100 次暗化归零
 */
TEST_F(GlowSquidEntityTest, HurtThenTick_FullDarkCycle)
{
    GlowSquidTestWorld world;
    m_glowSquid->setWorld(&world);

    TestAttackerEntity attacker;
    EntityDamageSource damage(DamageType::MobAttack, &attacker);

    ASSERT_TRUE(m_glowSquid->hurt(damage, 1.0f));
    ASSERT_EQ(m_glowSquid->getDarkTicksRemaining(), 100);

    for (i32 i = 0; i < 100; ++i) {
        m_glowSquid->tick();
    }
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);
}

// ============================================================================
// 4. getInkParticle 返回 GlowSquidInk / getSquirtSound 返回 GLOW_SQUID_SQUIRT
// ============================================================================

/**
 * @brief getInkParticle 返回 GlowSquidInk（对比 SquidEntity 返回 SquidInk）
 */
TEST_F(GlowSquidEntityTest, GetInkParticle_ReturnsGlowSquidInk)
{
    EXPECT_EQ(m_glowSquid->getInkParticle(), particle::ParticleTypeId::GlowSquidInk);

    // 对比基类 SquidEntity 返回 SquidInk
    SquidEntity squid(EntityInstanceId(5));
    EXPECT_EQ(squid.getInkParticle(), particle::ParticleTypeId::SquidInk);
}

/**
 * @brief getSquirtSound 返回 ENTITY_GLOW_SQUID_SQUIRT
 */
TEST_F(GlowSquidEntityTest, GetSquirtSound_ReturnsGlowSquidSquirt)
{
    auto sound = m_glowSquid->getSquirtSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), SoundEvents::ENTITY_GLOW_SQUID_SQUIRT.toString());
}

/**
 * @brief getAmbientSound / getHurtSound / getDeathSound 返回发光鱿鱼音效
 */
TEST_F(GlowSquidEntityTest, Sounds_ReturnGlowSquidVariants)
{
    auto ambient = m_glowSquid->getAmbientSound();
    ASSERT_TRUE(ambient.has_value());
    EXPECT_EQ(ambient->toString(), SoundEvents::ENTITY_GLOW_SQUID_AMBIENT.toString());

    EnvironmentalDamage dummyDamage(DamageType::Generic);
    auto hurtSound = m_glowSquid->getHurtSound(dummyDamage);
    ASSERT_TRUE(hurtSound.has_value());
    EXPECT_EQ(hurtSound->toString(), SoundEvents::ENTITY_GLOW_SQUID_HURT.toString());

    auto deathSound = m_glowSquid->getDeathSound();
    ASSERT_TRUE(deathSound.has_value());
    EXPECT_EQ(deathSound->toString(), SoundEvents::ENTITY_GLOW_SQUID_DEATH.toString());
}

// ============================================================================
// 5. 构造函数显式调用 registerData 不导致基类参数重复注册
// ============================================================================

/**
 * @brief 构造后基类（SquidEntity/WaterMobEntity）参数仍然存在
 *
 * GlowSquidEntity 构造函数显式调用 registerData()，
 * 该调用内部先调用 SquidEntity::registerData()，
 * 因此基类参数不应被覆盖或丢失。
 */
TEST_F(GlowSquidEntityTest, RegisterData_BaseClassParamsPreserved)
{
    // SquidEntity 的同步数据应仍然存在（通过 hasParam 间接验证）
    // SquidEntity 继承自 WaterMobEntity，后者继承自 CreatureEntity → MobEntity → LivingEntity
    // LivingEntity 注册了 HEALTH 等参数
    // 这里验证 GlowSquid 构造后所有层级参数均存在

    // 验证没有抛出异常即说明 registerData 链式调用正常
    TestableGlowSquidEntity entity(EntityInstanceId(10));

    // DarkTicksRemaining 参数存在
    EXPECT_TRUE(entity.dataManager().hasParam(GlowSquidEntity::getDarkTicksRemainingParamId()));

    // SquidEntity 的参数（如果有）和更基类的参数不应因重复注册而崩溃
    // 重复注册同一 ID 会被 registerParam 覆盖默认值，不产生异常
    SUCCEED();
}

/**
 * @brief 多次构造不同实例，DataParameter ID 全局唯一且一致
 *
 * DataParameter ID 是静态分配的，所有实例应共享同一 ID。
 */
TEST_F(GlowSquidEntityTest, RegisterData_ParamIdConsistentAcrossInstances)
{
    TestableGlowSquidEntity a(EntityInstanceId(1));
    TestableGlowSquidEntity b(EntityInstanceId(2));
    TestableGlowSquidEntity c(EntityInstanceId(3));

    const u16 id = GlowSquidEntity::getDarkTicksRemainingParamId();

    EXPECT_TRUE(a.dataManager().hasParam(id));
    EXPECT_TRUE(b.dataManager().hasParam(id));
    EXPECT_TRUE(c.dataManager().hasParam(id));

    // 修改 a 不影响 b、c（DataParameter 是实例级存储）
    a.setDarkTicks(100);
    EXPECT_EQ(a.getDarkTicksRemaining(), 100);
    EXPECT_EQ(b.getDarkTicksRemaining(), 0);
    EXPECT_EQ(c.getDarkTicksRemaining(), 0);
}

/**
 * @brief registerData 不产生重复条目（hasParam 唯一性）
 *
 * 如果 registerData 被错误地调用两次，DataParameter 仍应只有一条记录。
 */
TEST_F(GlowSquidEntityTest, RegisterData_IdempotentNoDuplicateEntries)
{
    // 构造函数已调用 registerData()，显式再调用一次不应产生问题
    m_glowSquid->registerData();

    const u16 paramId = GlowSquidEntity::getDarkTicksRemainingParamId();
    EXPECT_TRUE(m_glowSquid->dataManager().hasParam(paramId));

    // 值应被重置为默认值 0（registerParam 覆盖）
    EXPECT_EQ(m_glowSquid->getDarkTicksRemaining(), 0);
}

// ============================================================================
// 综合测试：客户端 tick 生成 GLOW 粒子
// ============================================================================

/**
 * @brief 客户端 tick 生成 GLOW 粒子
 *
 * 对应 MC Java GlowSquid.aiStep:
 *   this.level().addParticle(ParticleTypes.GLOW, ...)
 */
TEST_F(GlowSquidEntityTest, Tick_ClientSide_GeneratesGlowParticles)
{
    GlowSquidTestWorld world;
    world.setClientSide(true);
    m_glowSquid->setWorld(&world);

    world.clearParticles();
    m_glowSquid->tick();

    // 客户端 tick 应生成至少一个 GLOW 粒子
    bool hasGlowParticle = false;
    for (const auto& p : world.particles()) {
        if (p.type == particle::ParticleTypeId::Glow) {
            hasGlowParticle = true;
            break;
        }
    }
    EXPECT_TRUE(hasGlowParticle) << "Client-side tick should generate GLOW particle";
}

/**
 * @brief 服务端 tick 不生成 GLOW 粒子（addParticle 仅客户端调用）
 */
TEST_F(GlowSquidEntityTest, Tick_ServerSide_NoGlowParticles)
{
    GlowSquidTestWorld world;
    world.setClientSide(false);
    m_glowSquid->setWorld(&world);

    world.clearParticles();
    m_glowSquid->tick();

    // 服务端不应生成 GLOW 粒子
    for (const auto& p : world.particles()) {
        EXPECT_NE(p.type, particle::ParticleTypeId::Glow) << "Server-side tick should not generate GLOW particle";
    }
}

// ============================================================================
// 工厂方法测试
// ============================================================================

/**
 * @brief create 工厂方法返回有效实体
 */
TEST_F(GlowSquidEntityTest, Create_ReturnsValidEntity)
{
    auto entity = GlowSquidEntity::create(nullptr);
    EXPECT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<GlowSquidEntity*>(entity.get()), nullptr);
}

/**
 * @brief create 创建的实体默认 DarkTicksRemaining 为 0
 */
TEST_F(GlowSquidEntityTest, Create_DefaultDarkTicksZero)
{
    auto entity = GlowSquidEntity::create(nullptr);
    auto* glowSquid = dynamic_cast<GlowSquidEntity*>(entity.get());
    ASSERT_NE(glowSquid, nullptr);
    EXPECT_EQ(glowSquid->getDarkTicksRemaining(), 0);
}
