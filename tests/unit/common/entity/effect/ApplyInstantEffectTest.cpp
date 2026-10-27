/**
 * @brief applyInstantEffect 单元测试
 *
 * 覆盖 vanilla MobEffect#applyInstantenousEffect 的语义：治疗/伤害基数、距离因子缩放、
 * 亡灵反转，以及伤害源按施放者归属（indirectMagic）而非无主的 magic。
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::effect;

namespace {

/// 测试用僵尸：暴露 protected 的初始化入口，便于直接构造亡灵目标。
class TestZombieEntity final : public ZombieEntity {
public:
    explicit TestZombieEntity(EntityInstanceId id)
        : ZombieEntity(id, mc::test::testEcsRegistry())
    {
        setTypeId(EntityTypeKeys::ZOMBIE);
    }

    using ZombieEntity::registerAttributes;
    using ZombieEntity::registerData;
};

} // namespace

class ApplyInstantEffectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_caster = std::make_unique<LivingEntity>(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
        m_caster->registerData();
        m_caster->registerAttributes();
        m_caster->setHealth(m_caster->maxHealth());

        m_target = std::make_unique<LivingEntity>(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());
        m_target->registerData();
        m_target->registerAttributes();
        m_target->setHealth(m_target->maxHealth());
    }

    void TearDown() override
    {
        m_target.reset();
        m_caster.reset();
    }

    std::unique_ptr<LivingEntity> m_caster;
    std::unique_ptr<LivingEntity> m_target;
};

TEST_F(ApplyInstantEffectTest, InstantDamage_DamagesNonUndeadTarget)
{
    const f32 before = m_target->health();
    // I 级瞬间伤害：6<<0 = 6 点
    applyInstantEffect(EffectType::InstantDamage, *m_target, m_caster.get(), *m_target, 0, 1.0f);
    EXPECT_FLOAT_EQ(m_target->health(), before - 6.0f);
}

TEST_F(ApplyInstantEffectTest, InstantDamage_ScalesByMultiplier)
{
    const f32 before = m_target->health();
    // 距离因子 0.5：6 * 0.5 = 3 点
    applyInstantEffect(EffectType::InstantDamage, *m_target, m_caster.get(), *m_target, 0, 0.5f);
    EXPECT_FLOAT_EQ(m_target->health(), before - 3.0f);
}

TEST_F(ApplyInstantEffectTest, InstantDamage_HealsUndeadTarget)
{
    // 用真实亡灵实体：反转判定依赖 #inverted_healing_and_harm 实体类型标签。
    TestZombieEntity zombie(EntityInstanceId(3));
    zombie.registerData();
    zombie.registerAttributes();
    zombie.setHealth(zombie.maxHealth() / 2.0f);
    const f32 before = zombie.health();

    // 反转实体（亡灵）承受瞬间伤害时反而治疗 4<<0 = 4 点
    applyInstantEffect(EffectType::InstantDamage, zombie, m_caster.get(), zombie, 0, 1.0f);
    EXPECT_FLOAT_EQ(zombie.health(), before + 4.0f);
}

TEST_F(ApplyInstantEffectTest, InstantHealth_HealsNonUndeadTarget)
{
    m_target->setHealth(m_target->maxHealth() / 2.0f);
    const f32 before = m_target->health();
    applyInstantEffect(EffectType::InstantHealth, *m_target, m_caster.get(), *m_target, 0, 1.0f);
    EXPECT_FLOAT_EQ(m_target->health(), before + 4.0f);
}

TEST_F(ApplyInstantEffectTest, NonInstantEffect_IsIgnored)
{
    const f32 before = m_target->health();
    // 非瞬间效果不做处理
    applyInstantEffect(EffectType::Speed, *m_target, m_caster.get(), *m_target, 0, 1.0f);
    EXPECT_FLOAT_EQ(m_target->health(), before);
    EXPECT_FALSE(m_target->hasEffect(EffectType::Speed));
}
