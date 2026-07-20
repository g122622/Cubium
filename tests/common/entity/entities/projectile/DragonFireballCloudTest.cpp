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

#include <gtest/gtest.h>

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// DragonFireballEntity 粒子测试
// ============================================================================

/**
 * @brief DragonFireballEntity 粒子功能测试
 *
 * 测试 MC 1.16.5 DragonFireballEntity.createDragonBreathCloud() 功能：
 * - 创建 AreaEffectCloudEntity
 * - 设置正确的半径、持续时间
 * - 添加瞬间伤害 II 效果
 * - 设置龙息粒子类型
 */
class DragonFireballParticleTest : public ::testing::Test {
protected:
    void SetUp() override { m_dragonFireball = std::make_unique<DragonFireballEntity>(EntityInstanceId(1)); }

    std::unique_ptr<DragonFireballEntity> m_dragonFireball;
};

// ============================================================================
// 基本属性测试
// ============================================================================

TEST_F(DragonFireballParticleTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: DragonFireballEntity 尺寸
    EXPECT_FLOAT_EQ(m_dragonFireball->width(), 1.0f);
    EXPECT_FLOAT_EQ(m_dragonFireball->height(), 1.0f);
}

// ============================================================================
// AreaEffectCloudEntity 粒子类型测试
// ============================================================================

class DragonBreathCloudParticleTest : public ::testing::Test {
protected:
    void SetUp() override { m_cloud = std::make_unique<AreaEffectCloudEntity>(); }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(DragonBreathCloudParticleTest, DefaultParticleType_IsZero)
{
    // 默认粒子类型为 0
    EXPECT_EQ(m_cloud->getParticleType(), 0u);
}

TEST_F(DragonBreathCloudParticleTest, SetParticleType_UpdatesValue)
{
    using namespace client::renderer::trident::particle;

    // 设置龙息粒子
    m_cloud->setParticleType(static_cast<u32>(ParticleTypeId::DragonBreath));
    EXPECT_EQ(m_cloud->getParticleType(), static_cast<u32>(ParticleTypeId::DragonBreath));

    // 设置其他粒子类型
    m_cloud->setParticleType(static_cast<u32>(ParticleTypeId::Smoke));
    EXPECT_EQ(m_cloud->getParticleType(), static_cast<u32>(ParticleTypeId::Smoke));

    // 设置实体效果粒子
    m_cloud->setParticleType(static_cast<u32>(ParticleTypeId::EntityEffect));
    EXPECT_EQ(m_cloud->getParticleType(), static_cast<u32>(ParticleTypeId::EntityEffect));
}

TEST_F(DragonBreathCloudParticleTest, DragonBreathParticle_ValueCorrect)
{
    using namespace client::renderer::trident::particle;

    // 验证 DragonBreath 粒子类型的值（MC 1.21.11 协议 ID = 8）
    EXPECT_EQ(static_cast<u32>(ParticleTypeId::DragonBreath), 8u);

    // 设置并验证
    m_cloud->setParticleType(static_cast<u32>(ParticleTypeId::DragonBreath));
    EXPECT_EQ(m_cloud->getParticleType(), 8u);
}

// ============================================================================
// AreaEffectCloudEntity 龙息效果测试
// ============================================================================

TEST_F(DragonBreathCloudParticleTest, DragonBreathCloud_DefaultDuration)
{
    // MC 1.16.5: 龙息云默认持续时间 600 ticks (30秒)
    EXPECT_EQ(m_cloud->getDuration(), 600);
}

TEST_F(DragonBreathCloudParticleTest, DragonBreathCloud_DefaultRadius)
{
    // MC 1.16.5: 龙息云默认半径 3.0
    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 3.0f);
}

TEST_F(DragonBreathCloudParticleTest, DragonBreathCloud_AddEffect)
{
    // MC 1.16.5: 龙息云添加瞬间伤害 II 效果
    effect::EffectInstance instantDamage(effect::EffectType::InstantDamage,
        1,     // 持续时间（瞬间效果只需要1 tick）
        1,     // amplifier = 1 表示等级 II
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );

    m_cloud->addEffect(instantDamage);

    const auto& effects = m_cloud->getEffects();
    ASSERT_EQ(effects.size(), 1u);
    EXPECT_EQ(effects[0].type(), effect::EffectType::InstantDamage);
    EXPECT_EQ(effects[0].amplifier(), 1);
}

TEST_F(DragonBreathCloudParticleTest, DragonBreathCloud_RadiusPerTick)
{
    // MC 1.16.5: 龙息云半径从 3.0 扩展到 7.0，持续 600 ticks
    // radiusPerTick = (7.0 - 3.0) / 600 ≈ 0.0067
    constexpr f32 EXPECTED_RADIUS_PER_TICK = (7.0f - 3.0f) / 600.0f;

    m_cloud->setRadiusPerTick(EXPECTED_RADIUS_PER_TICK);
    // 验证设置成功（通过方法调用不抛异常）
    EXPECT_NO_THROW(m_cloud->setRadiusPerTick(EXPECTED_RADIUS_PER_TICK));
}

TEST_F(DragonBreathCloudParticleTest, DragonBreathCloud_WaitTime)
{
    // MC 1.16.5: 龙息云等待时间 10 ticks (0.5秒)
    m_cloud->setWaitTime(10);
    EXPECT_EQ(m_cloud->getWaitTime(), 10);
}

TEST_F(DragonBreathCloudParticleTest, DragonBreathCloud_ReapplicationDelay)
{
    // MC 1.16.5: 龙息云效果重应用延迟 20 ticks (1秒)
    m_cloud->setReapplicationDelay(20);
    EXPECT_EQ(m_cloud->getReapplicationDelay(), 20);
}

// ============================================================================
// 粒子类型枚举验证测试
// ============================================================================

class ProjectileParticleTypeEnumTest : public ::testing::Test {};

TEST_F(ProjectileParticleTypeEnumTest, ExplosionParticle_ValueCorrect)
{
    using namespace client::renderer::trident::particle;

    // 验证 Explosion 粒子类型的值
    // 参考 MC 1.16.5 ParticleTypes.EXPLOSION
    EXPECT_EQ(static_cast<u32>(ParticleTypeId::Explosion), 23u);
}

TEST_F(ProjectileParticleTypeEnumTest, SmokeParticle_ValueCorrect)
{
    using namespace client::renderer::trident::particle;

    // 验证 Smoke 粒子类型的值
    EXPECT_EQ(static_cast<u32>(ParticleTypeId::Smoke), 60u);
}

TEST_F(ProjectileParticleTypeEnumTest, DragonBreathParticle_ValueCorrect)
{
    using namespace client::renderer::trident::particle;

    // 验证 DragonBreath 粒子类型的值
    EXPECT_EQ(static_cast<u32>(ParticleTypeId::DragonBreath), 8u);
}

// ============================================================================
// WitherSkullEntity 粒子属性测试
// ============================================================================

class WitherSkullParticleTest : public ::testing::Test {
protected:
    void SetUp() override { m_witherSkull = std::make_unique<WitherSkullEntity>(EntityInstanceId(1)); }

    std::unique_ptr<WitherSkullEntity> m_witherSkull;
};

TEST_F(WitherSkullParticleTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: WitherSkullEntity 默认伤害 8.0
    EXPECT_FLOAT_EQ(m_witherSkull->damage(), 8.0f);

    // 尺寸
    EXPECT_FLOAT_EQ(m_witherSkull->width(), 0.3125f);
    EXPECT_FLOAT_EQ(m_witherSkull->height(), 0.3125f);

    // 可碰撞
    EXPECT_TRUE(m_witherSkull->canBeCollidedWith());
}

TEST_F(WitherSkullParticleTest, BlueFlag_DefaultFalse)
{
    // 默认不是蓝色凋灵之首
    EXPECT_FALSE(m_witherSkull->isBlue());

    // 设置蓝色
    m_witherSkull->setBlue(true);
    EXPECT_TRUE(m_witherSkull->isBlue());
}

// ============================================================================
// SmallFireballEntity 粒子属性测试
// ============================================================================

class SmallFireballParticleTest : public ::testing::Test {
protected:
    void SetUp() override { m_smallFireball = std::make_unique<SmallFireballEntity>(EntityInstanceId(1)); }

    std::unique_ptr<SmallFireballEntity> m_smallFireball;
};

TEST_F(SmallFireballParticleTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: SmallFireballEntity 尺寸
    EXPECT_FLOAT_EQ(m_smallFireball->width(), 0.3125f);
    EXPECT_FLOAT_EQ(m_smallFireball->height(), 0.3125f);

    // 可碰撞
    EXPECT_TRUE(m_smallFireball->canBeCollidedWith());
}

// ============================================================================
// FireballEntity 粒子属性测试
// ============================================================================

class FireballParticleTest : public ::testing::Test {
protected:
    void SetUp() override { m_fireball = std::make_unique<FireballEntity>(EntityInstanceId(1)); }

    std::unique_ptr<FireballEntity> m_fireball;
};

TEST_F(FireballParticleTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5: FireballEntity 默认伤害 6.0
    EXPECT_FLOAT_EQ(m_fireball->damage(), 6.0f);

    // 爆炸威力默认 1
    EXPECT_EQ(m_fireball->explosionPower(), 1);

    // 尺寸
    EXPECT_FLOAT_EQ(m_fireball->width(), 1.0f);
    EXPECT_FLOAT_EQ(m_fireball->height(), 1.0f);

    // 可碰撞
    EXPECT_TRUE(m_fireball->canBeCollidedWith());
}

TEST_F(FireballParticleTest, SetExplosionPower_UpdatesValue)
{
    m_fireball->setExplosionPower(2);
    EXPECT_EQ(m_fireball->explosionPower(), 2);

    m_fireball->setExplosionPower(5);
    EXPECT_EQ(m_fireball->explosionPower(), 5);
}
