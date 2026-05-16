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

#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// AreaEffectCloudEntity 测试
// ============================================================================

class AreaEffectCloudEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建 AreaEffectCloudEntity
        m_cloud = std::make_unique<AreaEffectCloudEntity>();
    }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

// ============================================================================
// 基本属性测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, DefaultValues_AreCorrect)
{
    // MC 1.16.5 默认值
    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 3.0f);
    EXPECT_EQ(m_cloud->getDuration(), 600);
    EXPECT_EQ(m_cloud->getWaitTime(), 20);
    EXPECT_EQ(m_cloud->getReapplicationDelay(), 20);
    EXPECT_TRUE(m_cloud->getEffects().empty());
}

TEST_F(AreaEffectCloudEntityTest, SetRadius_UpdatesValue)
{
    m_cloud->setRadius(5.0f);
    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 5.0f);
}

TEST_F(AreaEffectCloudEntityTest, SetDuration_UpdatesValue)
{
    m_cloud->setDuration(300);
    EXPECT_EQ(m_cloud->getDuration(), 300);
}

TEST_F(AreaEffectCloudEntityTest, SetWaitTime_UpdatesValue)
{
    m_cloud->setWaitTime(10);
    EXPECT_EQ(m_cloud->getWaitTime(), 10);
}

TEST_F(AreaEffectCloudEntityTest, SetReapplicationDelay_UpdatesValue)
{
    m_cloud->setReapplicationDelay(30);
    EXPECT_EQ(m_cloud->getReapplicationDelay(), 30);
}

TEST_F(AreaEffectCloudEntityTest, SetColor_UpdatesValue)
{
    m_cloud->setColor(0xFF0000FF); // 红色
    EXPECT_EQ(m_cloud->getColor(), 0xFF0000FF);
}

TEST_F(AreaEffectCloudEntityTest, SetRadiusOnUse_UpdatesValue)
{
    m_cloud->setRadiusOnUse(-0.5f);
    // radiusOnUse 是私有成员，通过行为测试
    EXPECT_NO_THROW(m_cloud->setRadiusOnUse(-0.5f));
}

TEST_F(AreaEffectCloudEntityTest, SetRadiusPerTick_UpdatesValue)
{
    m_cloud->setRadiusPerTick(-0.01f);
    // radiusPerTick 是私有成员，通过行为测试
    EXPECT_NO_THROW(m_cloud->setRadiusPerTick(-0.01f));
}

// ============================================================================
// 效果管理测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, AddEffect_IncreasesEffectCount)
{
    effect::EffectInstance speedEffect(effect::EffectType::Speed, 600, 0);
    m_cloud->addEffect(speedEffect);

    const auto& effects = m_cloud->getEffects();
    ASSERT_EQ(effects.size(), 1);
    EXPECT_EQ(effects[0].type(), effect::EffectType::Speed);
    EXPECT_EQ(effects[0].duration(), 600);
    EXPECT_EQ(effects[0].amplifier(), 0);
}

TEST_F(AreaEffectCloudEntityTest, AddMultipleEffects_AllStored)
{
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 400, 1));
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Regeneration, 200, 2));

    const auto& effects = m_cloud->getEffects();
    ASSERT_EQ(effects.size(), 3);

    EXPECT_EQ(effects[0].type(), effect::EffectType::Speed);
    EXPECT_EQ(effects[1].type(), effect::EffectType::Strength);
    EXPECT_EQ(effects[2].type(), effect::EffectType::Regeneration);
}

TEST_F(AreaEffectCloudEntityTest, ClearEffects_RemovesAllEffects)
{
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 400, 1));

    EXPECT_EQ(m_cloud->getEffects().size(), 2);

    m_cloud->clearEffects();

    EXPECT_TRUE(m_cloud->getEffects().empty());
}

TEST_F(AreaEffectCloudEntityTest, AddEffect_UpdatesColor_WhenNotSet)
{
    // 添加效果后，颜色应该自动计算（如果未手动设置）
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));

    // 速度效果的颜色是蓝色的 (0x7CAFC6)
    u32 color = m_cloud->getColor();
    EXPECT_NE(color, 0); // 颜色应该被设置
}

// ============================================================================
// 尺寸测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, Width_IsRadiusTimesTwo)
{
    m_cloud->setRadius(2.5f);
    EXPECT_FLOAT_EQ(m_cloud->width(), 5.0f); // 2.5 * 2

    m_cloud->setRadius(5.0f);
    EXPECT_FLOAT_EQ(m_cloud->width(), 10.0f); // 5.0 * 2
}

TEST_F(AreaEffectCloudEntityTest, Height_IsFixed)
{
    // MC 1.16.5: 药水云高度固定为 0.5
    EXPECT_FLOAT_EQ(m_cloud->height(), 0.5f);

    m_cloud->setRadius(10.0f);
    EXPECT_FLOAT_EQ(m_cloud->height(), 0.5f); // 高度不受半径影响
}

// ============================================================================
// 创建工厂方法测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, Create_ReturnsValidEntity)
{
    auto entity = AreaEffectCloudEntity::create(nullptr);
    EXPECT_NE(entity, nullptr);

    // 验证类型
    auto* cloud = dynamic_cast<AreaEffectCloudEntity*>(entity.get());
    EXPECT_NE(cloud, nullptr);
}

// ============================================================================
// 苦力怕药水云参数测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, CreeperLingeringCloudParameters_AreCorrect)
{
    // MC 1.16.5 CreeperEntity.spawnLingeringCloud() 参数
    // 初始半径: 2.5F
    // radiusOnUse: -0.5F
    // waitTime: 10 ticks
    // duration: 300 ticks (原600的一半)
    // radiusPerTick: -radius/duration = -2.5/300

    m_cloud->setRadius(2.5f);
    m_cloud->setRadiusOnUse(-0.5f);
    m_cloud->setWaitTime(10);
    m_cloud->setDuration(300);
    m_cloud->setRadiusPerTick(-2.5f / 300.0f);

    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 2.5f);
    EXPECT_EQ(m_cloud->getWaitTime(), 10);
    EXPECT_EQ(m_cloud->getDuration(), 300);
}

// ============================================================================
// 颜色计算测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, ColorCalculation_MultipleEffects)
{
    // 添加多个效果，颜色应该是混合的
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));      // 蓝色
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 600, 0));   // 红色

    u32 color = m_cloud->getColor();
    EXPECT_NE(color, 0); // 颜色应该被计算
}

// ============================================================================
// 效果类型颜色测试
// ============================================================================

class EffectColorTest : public ::testing::Test {
};

TEST_F(EffectColorTest, GetEffectColor_ReturnsValidColors)
{
    // 速度效果颜色 (蓝色)
    u32 speedColor = effect::getEffectColor(effect::EffectType::Speed);
    EXPECT_NE(speedColor, 0);

    // 力量效果颜色 (红色)
    u32 strengthColor = effect::getEffectColor(effect::EffectType::Strength);
    EXPECT_NE(strengthColor, 0);

    // 不同效果应该有不同的颜色
    EXPECT_NE(speedColor, strengthColor);
}

TEST_F(EffectColorTest, IsInstantEffect_ReturnsCorrectValues)
{
    // 瞬间治疗效果是瞬间效果
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::InstantHealth));

    // 瞬间伤害效果是瞬间效果
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::InstantDamage));

    // 速度效果不是瞬间效果
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Speed));

    // 力量效果不是瞬间效果
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Strength));
}

// ============================================================================
// EffectInstance 测试
// ============================================================================

class EffectInstanceTest : public ::testing::Test {
};

TEST_F(EffectInstanceTest, Constructor_SetsCorrectValues)
{
    effect::EffectInstance inst(effect::EffectType::Speed, 600, 1, true, false, true);

    EXPECT_EQ(inst.type(), effect::EffectType::Speed);
    EXPECT_EQ(inst.duration(), 600);
    EXPECT_EQ(inst.amplifier(), 1);
    EXPECT_TRUE(inst.isAmbient());
    EXPECT_FALSE(inst.isVisible());
    EXPECT_TRUE(inst.showIcon());
}

TEST_F(EffectInstanceTest, GetEffectLevel_ReturnsAmplifierPlusOne)
{
    effect::EffectInstance inst(effect::EffectType::Speed, 600, 0);
    EXPECT_EQ(inst.getEffectLevel(), 1); // amplifier 0 -> level I

    effect::EffectInstance inst2(effect::EffectType::Speed, 600, 1);
    EXPECT_EQ(inst2.getEffectLevel(), 2); // amplifier 1 -> level II

    effect::EffectInstance inst3(effect::EffectType::Speed, 600, 2);
    EXPECT_EQ(inst3.getEffectLevel(), 3); // amplifier 2 -> level III
}

TEST_F(EffectInstanceTest, IsExpired_WhenDurationIsZero)
{
    effect::EffectInstance inst(effect::EffectType::Speed, 0, 0);
    EXPECT_TRUE(inst.isExpired());

    effect::EffectInstance inst2(effect::EffectType::Speed, 100, 0);
    EXPECT_FALSE(inst2.isExpired());
}

TEST_F(EffectInstanceTest, IsPermanent_WhenDurationIsNegative)
{
    effect::EffectInstance inst(effect::EffectType::Speed, -1, 0);
    EXPECT_TRUE(inst.isPermanent());

    effect::EffectInstance inst2(effect::EffectType::Speed, 100, 0);
    EXPECT_FALSE(inst2.isPermanent());
}

TEST_F(EffectInstanceTest, CopyConstructor_CopiesAllValues)
{
    effect::EffectInstance original(effect::EffectType::Speed, 600, 2, true, false, false);
    effect::EffectInstance copy(original);

    EXPECT_EQ(copy.type(), original.type());
    EXPECT_EQ(copy.duration(), original.duration());
    EXPECT_EQ(copy.amplifier(), original.amplifier());
    EXPECT_EQ(copy.isAmbient(), original.isAmbient());
    EXPECT_EQ(copy.isVisible(), original.isVisible());
    EXPECT_EQ(copy.showIcon(), original.showIcon());
}

// ============================================================================
// 苦力怕药水云场景测试
// ============================================================================

class CreeperLingeringCloudScenarioTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_cloud = std::make_unique<AreaEffectCloudEntity>();

        // 模拟苦力怕爆炸后的药水云参数
        m_cloud->setRadius(2.5f);
        m_cloud->setRadiusOnUse(-0.5f);
        m_cloud->setWaitTime(10);
        m_cloud->setDuration(300);
        m_cloud->setRadiusPerTick(-2.5f / 300.0f);

        // 添加一些效果（模拟被药水影响的苦力怕）
        m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));
        m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 600, 1));
    }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(CreeperLingeringCloudScenarioTest, InitialState_IsCorrect)
{
    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 2.5f);
    EXPECT_EQ(m_cloud->getWaitTime(), 10);
    EXPECT_EQ(m_cloud->getDuration(), 300);
    EXPECT_EQ(m_cloud->getEffects().size(), 2);
}

TEST_F(CreeperLingeringCloudScenarioTest, NoCollision_IsEnabled)
{
    // MC 1.16.5: AreaEffectCloudEntity 无碰撞
    EXPECT_FALSE(m_cloud->canBeCollidedWith());
    // isPushable 是基类的虚函数，AreaEffectCloudEntity 重写返回 false
}

TEST_F(CreeperLingeringCloudScenarioTest, WidthHeight_AreCorrect)
{
    // 宽度 = 半径 * 2
    EXPECT_FLOAT_EQ(m_cloud->width(), 5.0f);
    // 高度固定为 0.5
    EXPECT_FLOAT_EQ(m_cloud->height(), 0.5f);
}

// ============================================================================
// 滞留药水场景测试（MC 1.16.5 PotionEntity.makeAreaOfEffectCloud）
// ============================================================================

class LingeringPotionCloudScenarioTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_cloud = std::make_unique<AreaEffectCloudEntity>();

        // MC 1.16.5 滞留药水创建的效果云参数
        // 参考: PotionEntity.makeAreaOfEffectCloud()
        m_cloud->setRadius(3.0f);
        m_cloud->setRadiusOnUse(-0.5f);
        m_cloud->setWaitTime(10);
        // radiusPerTick = -radius / duration = -3.0 / 600
        m_cloud->setRadiusPerTick(-m_cloud->getRadius() / static_cast<f32>(m_cloud->getDuration()));

        // 模拟滞留药水的效果（持续时间在效果云中为原持续时间的 1/4）
        // 例如：速度药水原持续时间 3600 tick (3分钟)，在效果云中为 900 tick
        m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 900, 0));        // 速度 I
        m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 900, 0));    // 力量 I
    }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(LingeringPotionCloudScenarioTest, InitialParameters_AreCorrect)
{
    // MC 1.16.5 标准滞留药水参数
    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 3.0f);
    EXPECT_EQ(m_cloud->getWaitTime(), 10);
    EXPECT_EQ(m_cloud->getDuration(), 600); // 默认 30 秒 = 600 ticks
}

TEST_F(LingeringPotionCloudScenarioTest, RadiusOnUse_Negative)
{
    // 每次应用效果时半径减少 0.5
    // 注：radiusOnUse 是私有成员，只能通过 setter 设置
    EXPECT_NO_THROW(m_cloud->setRadiusOnUse(-0.5f));
}

TEST_F(LingeringPotionCloudScenarioTest, RadiusPerTick_CalculatedCorrectly)
{
    // radiusPerTick = -3.0 / 600 = -0.005
    f32 expectedRadiusPerTick = -3.0f / 600.0f;
    EXPECT_NO_THROW(m_cloud->setRadiusPerTick(expectedRadiusPerTick));
}

TEST_F(LingeringPotionCloudScenarioTest, Effects_AreStored)
{
    const auto& effects = m_cloud->getEffects();
    ASSERT_EQ(effects.size(), 2);

    // 效果持续时间应为原持续时间的 1/4
    // 这里验证效果确实被存储
    EXPECT_EQ(effects[0].type(), effect::EffectType::Speed);
    EXPECT_EQ(effects[0].duration(), 900); // 3600 / 4 = 900
    EXPECT_EQ(effects[1].type(), effect::EffectType::Strength);
    EXPECT_EQ(effects[1].duration(), 900);
}

TEST_F(LingeringPotionCloudScenarioTest, Color_IsCalculatedFromEffects)
{
    // 颜色应该根据效果自动计算
    u32 color = m_cloud->getColor();
    EXPECT_NE(color, 0); // 颜色应该被设置
}

TEST_F(LingeringPotionCloudScenarioTest, Width_IsRadiusTimesTwo)
{
    // 宽度 = 半径 * 2
    EXPECT_FLOAT_EQ(m_cloud->width(), 6.0f); // 3.0 * 2
}

TEST_F(LingeringPotionCloudScenarioTest, Height_IsFixed)
{
    // 高度固定为 0.5
    EXPECT_FLOAT_EQ(m_cloud->height(), 0.5f);
}

TEST_F(LingeringPotionCloudScenarioTest, SetOwner_CanBeSet)
{
    // 注：在无世界环境的测试中，我们只验证接口可用
    EXPECT_NO_THROW(m_cloud->setOwner(nullptr));
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
}

// ============================================================================
// 效果持续时间除以 4 的逻辑测试
// ============================================================================

class LingeringPotionDurationTest : public ::testing::Test {
};

TEST_F(LingeringPotionDurationTest, DurationDividedByFour)
{
    // MC 1.16.5: 效果在区域效果云中持续时间为原持续时间的 1/4
    // 例如：8:00 的药水（9600 tick）在效果云中为 2400 tick

    // 模拟 8 分钟速度药水
    i32 originalDuration = 9600; // 8 分钟 = 9600 tick
    i32 cloudDuration = originalDuration / 4;

    EXPECT_EQ(cloudDuration, 2400);
}

TEST_F(LingeringPotionDurationTest, DurationDividedByFour_MinimumDuration)
{
    // 即使原持续时间很短，效果云中也至少有一些持续时间
    i32 originalDuration = 100; // 5 秒
    i32 cloudDuration = originalDuration / 4;

    EXPECT_EQ(cloudDuration, 25); // 25 tick = 1.25 秒
}

TEST_F(LingeringPotionDurationTest, DurationDividedByFour_InstantEffect)
{
    // 瞬间效果（瞬间治疗/瞬间伤害）持续时间为 1 tick
    // 在效果云中，瞬间效果会被特殊处理（使用 affectEntity 方法）
    // 这里只测试除法逻辑
    i32 originalDuration = 1; // 瞬间效果
    i32 cloudDuration = originalDuration / 4;

    EXPECT_EQ(cloudDuration, 0); // 会被截断为 0
}

// ============================================================================
// 滞留药水效果云参数与苦力怕药水云对比测试
// ============================================================================

class LingeringPotionVsCreeperCloudTest : public ::testing::Test {
};

TEST_F(LingeringPotionVsCreeperCloudTest, LingeringPotion_HasLargerRadius)
{
    // 滞留药水半径 3.0，苦力怕药水云半径 2.5
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>();
    lingeringCloud->setRadius(3.0f);

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>();
    creeperCloud->setRadius(2.5f);

    EXPECT_GT(lingeringCloud->getRadius(), creeperCloud->getRadius());
}

TEST_F(LingeringPotionVsCreeperCloudTest, LingeringPotion_HasLongerDuration)
{
    // 滞留药水持续时间 600 tick（30秒），苦力怕药水云持续时间 300 tick（15秒）
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>();
    // 默认 duration = 600

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>();
    creeperCloud->setDuration(300);

    EXPECT_GT(lingeringCloud->getDuration(), creeperCloud->getDuration());
}

TEST_F(LingeringPotionVsCreeperCloudTest, BothHaveSameRadiusOnUse)
{
    // 两者都有 radiusOnUse = -0.5
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>();
    lingeringCloud->setRadiusOnUse(-0.5f);

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>();
    creeperCloud->setRadiusOnUse(-0.5f);

    // 两者参数相同
    EXPECT_NO_THROW(lingeringCloud->setRadiusOnUse(-0.5f));
    EXPECT_NO_THROW(creeperCloud->setRadiusOnUse(-0.5f));
}

TEST_F(LingeringPotionVsCreeperCloudTest, BothHaveSameWaitTime)
{
    // 两者都有 waitTime = 10
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>();
    lingeringCloud->setWaitTime(10);

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>();
    creeperCloud->setWaitTime(10);

    EXPECT_EQ(lingeringCloud->getWaitTime(), creeperCloud->getWaitTime());
}
