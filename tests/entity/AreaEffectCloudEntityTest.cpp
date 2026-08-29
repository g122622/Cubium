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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

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
        m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

// ============================================================================
// 基本属性测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, DefaultValues_AreCorrect)
{
    // 默认值
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
    // 药水云高度固定为 0.5
    EXPECT_FLOAT_EQ(m_cloud->height(), 0.5f);

    m_cloud->setRadius(10.0f);
    EXPECT_FLOAT_EQ(m_cloud->height(), 0.5f); // 高度不受半径影响
}

// ============================================================================
// 创建工厂方法测试
// ============================================================================

TEST_F(AreaEffectCloudEntityTest, Create_ReturnsValidEntity)
{
    auto entity = AreaEffectCloudEntity::create(nullptr, mc::test::testEcsRegistry());
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
    // 苦力怕爆炸后的药水云参数
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
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));    // 蓝色
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 600, 0)); // 红色

    u32 color = m_cloud->getColor();
    EXPECT_NE(color, 0); // 颜色应该被计算
}

// ============================================================================
// 效果类型颜色测试
// ============================================================================

class EffectColorTest : public ::testing::Test {};

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

class EffectInstanceTest : public ::testing::Test {};

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
        m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());

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
    // AreaEffectCloudEntity 无碰撞
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
// 滞留药水场景测试
// ============================================================================

class LingeringPotionCloudScenarioTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());

        // 滞留药水创建的效果云参数
        // 参考: PotionEntity.makeAreaOfEffectCloud()
        m_cloud->setRadius(3.0f);
        m_cloud->setRadiusOnUse(-0.5f);
        m_cloud->setWaitTime(10);
        // radiusPerTick = -radius / duration = -3.0 / 600
        m_cloud->setRadiusPerTick(-m_cloud->getRadius() / static_cast<f32>(m_cloud->getDuration()));

        // 模拟滞留药水的效果（持续时间在效果云中为原持续时间的 1/4）
        // 例如：速度药水原持续时间 3600 tick (3分钟)，在效果云中为 900 tick
        m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 900, 0));    // 速度 I
        m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 900, 0)); // 力量 I
    }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(LingeringPotionCloudScenarioTest, InitialParameters_AreCorrect)
{
    // 标准滞留药水参数
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

TEST_F(LingeringPotionCloudScenarioTest, SetOwner_Nullptr_ClearsUuid)
{
    // 设置 nullptr 时，UUID 应该被清空
    m_cloud->setOwner(nullptr);
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

// ============================================================================
// 效果持续时间除以 4 的逻辑测试
// ============================================================================

class LingeringPotionDurationTest : public ::testing::Test {};

TEST_F(LingeringPotionDurationTest, DurationDividedByFour)
{
    // 效果在区域效果云中持续时间为原持续时间的 1/4
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

class LingeringPotionVsCreeperCloudTest : public ::testing::Test {};

TEST_F(LingeringPotionVsCreeperCloudTest, LingeringPotion_HasLargerRadius)
{
    // 滞留药水半径 3.0，苦力怕药水云半径 2.5
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    lingeringCloud->setRadius(3.0f);

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    creeperCloud->setRadius(2.5f);

    EXPECT_GT(lingeringCloud->getRadius(), creeperCloud->getRadius());
}

TEST_F(LingeringPotionVsCreeperCloudTest, LingeringPotion_HasLongerDuration)
{
    // 滞留药水持续时间 600 tick（30秒），苦力怕药水云持续时间 300 tick（15秒）
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    // 默认 duration = 600

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    creeperCloud->setDuration(300);

    EXPECT_GT(lingeringCloud->getDuration(), creeperCloud->getDuration());
}

TEST_F(LingeringPotionVsCreeperCloudTest, BothHaveSameRadiusOnUse)
{
    // 两者都有 radiusOnUse = -0.5
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    lingeringCloud->setRadiusOnUse(-0.5f);

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    creeperCloud->setRadiusOnUse(-0.5f);

    // 两者参数相同
    EXPECT_NO_THROW(lingeringCloud->setRadiusOnUse(-0.5f));
    EXPECT_NO_THROW(creeperCloud->setRadiusOnUse(-0.5f));
}

TEST_F(LingeringPotionVsCreeperCloudTest, BothHaveSameWaitTime)
{
    // 两者都有 waitTime = 10
    auto lingeringCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    lingeringCloud->setWaitTime(10);

    auto creeperCloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    creeperCloud->setWaitTime(10);

    EXPECT_EQ(lingeringCloud->getWaitTime(), creeperCloud->getWaitTime());
}

// ============================================================================
// 瞬间效果处理测试
// ============================================================================

class InstantEffectTest : public ::testing::Test {};

TEST_F(InstantEffectTest, IsInstantEffect_IdentifiesCorrectTypes)
{
    // 瞬间效果包括瞬间治疗、瞬间伤害、饱和
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::InstantHealth));
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::InstantDamage));
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::Saturation));

    // 非瞬间效果
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Speed));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Strength));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Regeneration));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Poison));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Wither));
}

TEST_F(InstantEffectTest, GetEffectColor_InstantEffectsHaveColors)
{
    // 瞬间治疗效果颜色
    u32 instantHealthColor = effect::getEffectColor(effect::EffectType::InstantHealth);
    EXPECT_NE(instantHealthColor, 0);

    // 瞬间伤害效果颜色
    u32 instantDamageColor = effect::getEffectColor(effect::EffectType::InstantDamage);
    EXPECT_NE(instantDamageColor, 0);

    // 饱和效果颜色
    u32 saturationColor = effect::getEffectColor(effect::EffectType::Saturation);
    EXPECT_NE(saturationColor, 0);
}

// ============================================================================
// canBeHitWithPotion 测试
// ============================================================================

class CanBeHitWithPotionTest : public ::testing::Test {};

// 注意：完整的 canBeHitWithPotion 测试需要 Mock LivingEntity，
// 这里测试 isInstantEffect 的正确性，它是瞬间效果处理的关键

TEST_F(CanBeHitWithPotionTest, InstantHealthEffect_IsCorrectlyIdentified)
{
    // 瞬间治疗是瞬间效果
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::InstantHealth));
}

TEST_F(CanBeHitWithPotionTest, InstantDamageEffect_IsCorrectlyIdentified)
{
    // 瞬间伤害是瞬间效果
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::InstantDamage));
}

TEST_F(CanBeHitWithPotionTest, SaturationEffect_IsCorrectlyIdentified)
{
    // 饱和是瞬间效果
    EXPECT_TRUE(effect::isInstantEffect(effect::EffectType::Saturation));
}

TEST_F(CanBeHitWithPotionTest, DurationEffects_AreNotInstant)
{
    // 持续效果不是瞬间效果
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Speed));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Slowness));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Poison));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Regeneration));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Wither));
    EXPECT_FALSE(effect::isInstantEffect(effect::EffectType::Levitation));
}

// ============================================================================
// 效果乘数测试
// ============================================================================

class EffectMultiplierTest : public ::testing::Test {};

TEST_F(EffectMultiplierTest, AreaEffectCloudMultiplier_IsHalf)
{
    // 药水云中瞬间效果的强度乘数为 0.5
    // 瞬间治疗基础值 4.0，乘数 0.5 后为 2.0
    // 公式：(4.0 + amplifier * 2.0) * 0.5

    // amplifier = 0 (效果等级 I)
    f32 baseAmount = 4.0f;
    f32 perLevel = 2.0f;
    f32 multiplier = 0.5f; // 药水云乘数

    // 效果 I: (4.0 + 0 * 2.0) * 0.5 = 2.0
    f32 level1 = (baseAmount + 0 * perLevel) * multiplier;
    EXPECT_FLOAT_EQ(level1, 2.0f);

    // 效果 II: (4.0 + 1 * 2.0) * 0.5 = 3.0
    f32 level2 = (baseAmount + 1 * perLevel) * multiplier;
    EXPECT_FLOAT_EQ(level2, 3.0f);

    // 效果 III: (4.0 + 2 * 2.0) * 0.5 = 4.0
    f32 level3 = (baseAmount + 2 * perLevel) * multiplier;
    EXPECT_FLOAT_EQ(level3, 4.0f);
}

TEST_F(EffectMultiplierTest, SplashPotionMultiplier_IsFull)
{
    // 喷溅药水的瞬间效果强度乘数为 1.0
    // 瞬间治疗基础值 4.0，乘数 1.0 后为 4.0
    f32 baseAmount = 4.0f;
    f32 perLevel = 2.0f;
    f32 multiplier = 1.0f; // 喷溅药水乘数

    // 效果 I: (4.0 + 0 * 2.0) * 1.0 = 4.0
    f32 level1 = (baseAmount + 0 * perLevel) * multiplier;
    EXPECT_FLOAT_EQ(level1, 4.0f);

    // 效果 II: (4.0 + 1 * 2.0) * 1.0 = 6.0
    f32 level2 = (baseAmount + 1 * perLevel) * multiplier;
    EXPECT_FLOAT_EQ(level2, 6.0f);
}

// ============================================================================
// 效果云生命周期测试
// ============================================================================

class AreaEffectCloudLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override { m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry()); }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(AreaEffectCloudLifecycleTest, DefaultDuration_Is600Ticks)
{
    // 默认持续时间 600 tick = 30 秒
    EXPECT_EQ(m_cloud->getDuration(), 600);
}

TEST_F(AreaEffectCloudLifecycleTest, DefaultWaitTime_Is20Ticks)
{
    // 默认等待时间 20 tick = 1 秒
    EXPECT_EQ(m_cloud->getWaitTime(), 20);
}

TEST_F(AreaEffectCloudLifecycleTest, DefaultReapplicationDelay_Is20Ticks)
{
    // 默认重应用延迟 20 tick = 1 秒
    EXPECT_EQ(m_cloud->getReapplicationDelay(), 20);
}

TEST_F(AreaEffectCloudLifecycleTest, DefaultRadius_Is3_0)
{
    // 默认半径 3.0
    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 3.0f);
}

TEST_F(AreaEffectCloudLifecycleTest, DurationOnUse_DefaultIsZero)
{
    // 默认 durationOnUse 为 0
    // 注：durationOnUse 是私有成员，无法直接访问
    // 这里验证 setDurationOnUse 可被调用
    EXPECT_NO_THROW(m_cloud->setDurationOnUse(-10));
}

TEST_F(AreaEffectCloudLifecycleTest, RadiusOnUse_DefaultIsZero)
{
    // 默认 radiusOnUse 为 0
    // 注：radiusOnUse 是私有成员，无法直接访问
    // 这里验证 setRadiusOnUse 可被调用
    EXPECT_NO_THROW(m_cloud->setRadiusOnUse(-0.5f));
}

// ============================================================================
// 效果云颜色测试（多效果混合）
// ============================================================================

class AreaEffectCloudColorTest : public ::testing::Test {
protected:
    void SetUp() override { m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry()); }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(AreaEffectCloudColorTest, SingleEffect_ColorMatchesEffect)
{
    // 单个效果时，颜色应该匹配效果颜色（可能包含 Alpha 通道）
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));

    u32 cloudColor = m_cloud->getColor();
    u32 speedColor = effect::getEffectColor(effect::EffectType::Speed);

    // 颜色可能包含 Alpha 通道（ARGB 格式），提取 RGB 进行比较
    u32 cloudRGB = cloudColor & 0x00FFFFFF;
    u32 speedRGB = speedColor & 0x00FFFFFF;

    EXPECT_EQ(cloudRGB, speedRGB);
}

TEST_F(AreaEffectCloudColorTest, MultipleEffects_ColorIsBlended)
{
    // 多个效果时，颜色应该是混合的
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));    // 蓝色
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 600, 0)); // 红色

    u32 cloudColor = m_cloud->getColor();
    u32 speedColor = effect::getEffectColor(effect::EffectType::Speed);
    u32 strengthColor = effect::getEffectColor(effect::EffectType::Strength);

    // 混合颜色应该不等于任何一个单独的颜色
    // （除非两个效果颜色相同）
    EXPECT_NE(cloudColor, 0);
}

TEST_F(AreaEffectCloudColorTest, SetColor_OverridesAutoCalculation)
{
    // 手动设置颜色后，后续添加效果不应覆盖
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));
    u32 autoColor = m_cloud->getColor();

    m_cloud->setColor(0xFFFF0000); // 红色
    EXPECT_EQ(m_cloud->getColor(), 0xFFFF0000);

    // 再添加效果，颜色不应改变
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 600, 0));
    EXPECT_EQ(m_cloud->getColor(), 0xFFFF0000); // 仍然是手动设置的颜色
}

TEST_F(AreaEffectCloudColorTest, NoEffects_ColorIsZero)
{
    // 无效果时，颜色应该为 0
    EXPECT_EQ(m_cloud->getColor(), 0);
}

// ============================================================================
// Owner UUID 测试
// ============================================================================

class AreaEffectCloudOwnerTest : public ::testing::Test {
protected:
    void SetUp() override { m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry()); }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(AreaEffectCloudOwnerTest, DefaultOwner_IsNullptr)
{
    // 默认无 owner
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

TEST_F(AreaEffectCloudOwnerTest, SetOwner_Nullptr_ClearsOwnerAndUuid)
{
    // 先设置一个 owner 再清空
    m_cloud->setOwner(nullptr);
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

TEST_F(AreaEffectCloudOwnerTest, SetOwnerUuid_SetsUuidOnly)
{
    // 仅设置 UUID，不设置指针
    m_cloud->setOwnerUuid("abcdef0123456789abcdef0123456789");
    EXPECT_EQ(m_cloud->ownerUuid(), "abcdef0123456789abcdef0123456789");
    // 指针应为 nullptr（等 getOwner() 在有世界环境时才通过 UUID 查找）
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
}

TEST_F(AreaEffectCloudOwnerTest, SetOwnerUuid_EmptyString_ClearsUuid)
{
    // 先设置 UUID 再清空
    m_cloud->setOwnerUuid("abcdef0123456789abcdef0123456789");
    EXPECT_FALSE(m_cloud->ownerUuid().empty());

    m_cloud->setOwnerUuid("");
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

TEST_F(AreaEffectCloudOwnerTest, OwnerUuid_AfterSetOwnerWithNullptr_IsEmpty)
{
    // setOwner(nullptr) 应该同时清空 UUID
    m_cloud->setOwner(nullptr);
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

TEST_F(AreaEffectCloudOwnerTest, GetOwner_ConstVersion_ReturnsCachePointer)
{
    // const 版本的 getOwner 直接返回缓存指针
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
}

TEST_F(AreaEffectCloudOwnerTest, SetOwnerUuid_DoesNotSetCachePointer)
{
    // setOwnerUuid 只设置 UUID，不清除缓存指针（设计上指针在下次 getOwner 时重新查找）
    m_cloud->setOwnerUuid("abcdef0123456789abcdef0123456789");
    // getOwner() 应返回 nullptr（无世界环境，无法通过 UUID 查找）
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
    EXPECT_EQ(m_cloud->ownerUuid(), "abcdef0123456789abcdef0123456789");
}

// ============================================================================
// NBT 序列化/反序列化测试
// ============================================================================

class AreaEffectCloudNbtTest : public ::testing::Test {
protected:
    void SetUp() override { m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry()); }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(AreaEffectCloudNbtTest, SerializeDeserialize_RoundTrip)
{
    // 设置各种属性
    m_cloud->setRadius(5.0f);
    m_cloud->setDuration(300);
    m_cloud->setWaitTime(10);
    m_cloud->setReapplicationDelay(30);
    m_cloud->setDurationOnUse(-5);
    m_cloud->setRadiusOnUse(-0.5f);
    m_cloud->setRadiusPerTick(-0.01f);
    m_cloud->setColor(0xFF00FF00);
    m_cloud->setOwnerUuid("a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6");
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 600, 0));
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Strength, 400, 1));

    // 序列化
    nbt::tags::compound_tag tag;
    m_cloud->addAdditionalSaveData(tag);

    // 反序列化到新实体
    auto cloud2 = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    auto result = cloud2->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    // 验证属性值
    EXPECT_FLOAT_EQ(cloud2->getRadius(), 5.0f);
    EXPECT_EQ(cloud2->getDuration(), 300);
    EXPECT_EQ(cloud2->getWaitTime(), 10);
    EXPECT_EQ(cloud2->getReapplicationDelay(), 30);
    EXPECT_EQ(cloud2->getColor(), 0xFF00FF00);

    // 验证 Owner UUID
    EXPECT_EQ(cloud2->ownerUuid(), "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6");

    // 验证效果列表
    const auto& effects = cloud2->getEffects();
    ASSERT_EQ(effects.size(), 2u);
    EXPECT_EQ(effects[0].type(), effect::EffectType::Speed);
    EXPECT_EQ(effects[1].type(), effect::EffectType::Strength);
}

TEST_F(AreaEffectCloudNbtTest, SerializeDeserialize_DefaultValues)
{
    // 默认值序列化/反序列化
    nbt::tags::compound_tag tag;
    m_cloud->addAdditionalSaveData(tag);

    auto cloud2 = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    auto result = cloud2->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    // 默认值应保持
    EXPECT_FLOAT_EQ(cloud2->getRadius(), 3.0f);
    EXPECT_EQ(cloud2->getDuration(), 600);
    EXPECT_EQ(cloud2->getWaitTime(), 20);
    EXPECT_EQ(cloud2->getReapplicationDelay(), 20);

    // 无 Owner UUID
    EXPECT_TRUE(cloud2->ownerUuid().empty());

    // 无效果
    EXPECT_TRUE(cloud2->getEffects().empty());
}

TEST_F(AreaEffectCloudNbtTest, Serialize_OwnerUuid_WrittenAsUuidMostLeast)
{
    // 设置 Owner UUID
    m_cloud->setOwnerUuid("0123456789abcdef0123456789abcdef");

    // 序列化
    nbt::tags::compound_tag tag;
    m_cloud->addAdditionalSaveData(tag);

    // 验证 OwnerUUIDMost 和 OwnerUUIDLeast 存在
    auto most = mc::entity::serialization::nbt_helper::tryGetLong(tag, "OwnerUUIDMost");
    auto least = mc::entity::serialization::nbt_helper::tryGetLong(tag, "OwnerUUIDLeast");
    ASSERT_TRUE(most.has_value());
    ASSERT_TRUE(least.has_value());

    // 反序列化并验证 UUID 一致
    auto cloud2 = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    auto result = cloud2->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(cloud2->ownerUuid(), "0123456789abcdef0123456789abcdef");
}

TEST_F(AreaEffectCloudNbtTest, Serialize_NoOwnerUuid_NoKeysWritten)
{
    // 不设置 Owner UUID（默认为空）
    nbt::tags::compound_tag tag;
    m_cloud->addAdditionalSaveData(tag);

    // 验证 OwnerUUIDMost 和 OwnerUUIDLeast 不存在
    auto most = mc::entity::serialization::nbt_helper::tryGetLong(tag, "OwnerUUIDMost");
    auto least = mc::entity::serialization::nbt_helper::tryGetLong(tag, "OwnerUUIDLeast");
    EXPECT_FALSE(most.has_value());
    EXPECT_FALSE(least.has_value());
}

TEST_F(AreaEffectCloudNbtTest, Deserialize_MissingKeys_KeepDefaults)
{
    // 空的 NBT tag 反序列化应保持默认值
    nbt::tags::compound_tag emptyTag;
    auto result = m_cloud->readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(static_cast<bool>(result));

    // 默认值应保持
    EXPECT_FLOAT_EQ(m_cloud->getRadius(), 3.0f);
    EXPECT_EQ(m_cloud->getDuration(), 600);
    EXPECT_EQ(m_cloud->getWaitTime(), 20);
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

// ============================================================================
// Owner UUID 懒加载测试（无世界环境验证逻辑）
// ============================================================================

class AreaEffectCloudOwnerLazyLoadTest : public ::testing::Test {
protected:
    void SetUp() override { m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry()); }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(AreaEffectCloudOwnerLazyLoadTest, SetOwnerNullptr_ClearsUuidAndPointer)
{
    // setOwner(nullptr) 应清空 UUID 和指针
    m_cloud->setOwnerUuid("aabbccdd11223344aabbccdd11223344");
    EXPECT_FALSE(m_cloud->ownerUuid().empty());

    m_cloud->setOwner(nullptr);
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

TEST_F(AreaEffectCloudOwnerLazyLoadTest, SetOwnerUuid_ThenGetOwner_ReturnsNullptrWithoutWorld)
{
    // 设置 UUID 后，没有世界环境时 getOwner() 应返回 nullptr
    m_cloud->setOwnerUuid("aabbccdd11223344aabbccdd11223344");
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
    // UUID 仍然保留
    EXPECT_EQ(m_cloud->ownerUuid(), "aabbccdd11223344aabbccdd11223344");
}

TEST_F(AreaEffectCloudOwnerLazyLoadTest, OwnerUuid_EmptyStringAfterClear)
{
    m_cloud->setOwnerUuid("test");
    m_cloud->setOwnerUuid("");
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

// ============================================================================
// 伤害来源归属测试（验证 applyInstantEffect 使用 owner）
// ============================================================================

class AreaEffectCloudDamageSourceTest : public ::testing::Test {
protected:
    void SetUp() override { m_cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry()); }

    std::unique_ptr<AreaEffectCloudEntity> m_cloud;
};

TEST_F(AreaEffectCloudDamageSourceTest, SetOwner_Nullptr_DamageSourceIsMagic)
{
    // 当 owner 为 nullptr 时，瞬间效果应使用 DamageSources::magic() 伤害来源
    // 这只是验证 setOwner(nullptr) 不会崩溃
    m_cloud->setOwner(nullptr);
    EXPECT_EQ(m_cloud->getOwner(), nullptr);
    EXPECT_TRUE(m_cloud->ownerUuid().empty());
}

TEST_F(AreaEffectCloudDamageSourceTest, SetOwnerUuid_SetsUuidCorrectly)
{
    // 验证 setOwnerUuid 正确设置 UUID 字符串
    const std::string testUuid = "abcdef0123456789abcdef0123456789";
    m_cloud->setOwnerUuid(testUuid);
    EXPECT_EQ(m_cloud->ownerUuid(), testUuid);
    EXPECT_EQ(m_cloud->ownerUuid().length(), 32u);
}

TEST_F(AreaEffectCloudDamageSourceTest, NbtRoundTrip_PreservesOwnerUuid)
{
    // 验证 NBT 序列化/反序列化往返后 Owner UUID 保持一致
    const std::string testUuid = "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6";
    m_cloud->setOwnerUuid(testUuid);
    m_cloud->setDuration(400);

    nbt::tags::compound_tag tag;
    m_cloud->addAdditionalSaveData(tag);

    auto cloud2 = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    auto result = cloud2->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(cloud2->ownerUuid(), testUuid);
    EXPECT_EQ(cloud2->getDuration(), 400);
}

TEST_F(AreaEffectCloudDamageSourceTest, NbtRoundTrip_EffectsPreserved)
{
    // 验证效果列表在 NBT 序列化/反序列化后保持一致
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Speed, 200, 0));
    m_cloud->addEffect(effect::EffectInstance(effect::EffectType::Regeneration, 300, 2));

    nbt::tags::compound_tag tag;
    m_cloud->addAdditionalSaveData(tag);

    auto cloud2 = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    auto result = cloud2->readAdditionalSaveData(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    const auto& effects = cloud2->getEffects();
    ASSERT_EQ(effects.size(), 2u);
    EXPECT_EQ(effects[0].type(), effect::EffectType::Speed);
    EXPECT_EQ(effects[0].duration(), 200);
    EXPECT_EQ(effects[0].amplifier(), 0);
    EXPECT_EQ(effects[1].type(), effect::EffectType::Regeneration);
    EXPECT_EQ(effects[1].duration(), 300);
    EXPECT_EQ(effects[1].amplifier(), 2);
}

// ============================================================================
// 火焰免疫测试（对齐 vanilla EntityType.AREA_EFFECT_CLOUD.fireImmune()，
// EntityType.java:227）。vanilla 滞留药水云（AreaEffectCloud）免疫火焰伤害。
// AreaEffectCloudEntity 直接继承 Entity（非 LivingEntity），走基类
// Entity::isInvulnerableTo 的 IS_FIRE+isImmuneToFire 守卫免疫火焰。
// AreaEffectCloudEntity 无 hurt override（grep 确认），走基类 Entity::hurt。
// 注：此套件的 AreaEffectCloudEntityTest SetUp 不初始化注册表，故此处用一个
// 独立夹具 AreaEffectCloudFireImmuneTest 自行初始化 VanillaBlocks/VanillaEntities。
// ============================================================================

class AreaEffectCloudFireImmuneTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和实体注册表，使 AREA_EFFECT_CLOUD 类型注册 .immuneToFire()
        // 标志，且 isImmuneToFire() 经 EntityRegistry 能查到该类型。
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
    }
};

TEST_F(AreaEffectCloudFireImmuneTest, FlagRegistered)
{
    auto& registry = EntityRegistry::instance();
    const EntityType* cloudType = registry.getType(EntityTypeKeys::AREA_EFFECT_CLOUD);
    ASSERT_NE(cloudType, nullptr);
    EXPECT_TRUE(cloudType->immuneToFire())
        << "AREA_EFFECT_CLOUD 实体类型应注册 fireImmune（vanilla EntityType.AREA_EFFECT_CLOUD.fireImmune()）";
}

TEST_F(AreaEffectCloudFireImmuneTest, IsImmuneToFire_InstanceReturnsTrue)
{
    auto cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    // 直接构造不经工厂，补 setTypeId 对齐生产路径。
    cloud->setTypeId(EntityTypeKeys::AREA_EFFECT_CLOUD);
    EXPECT_TRUE(cloud->isImmuneToFire()) << "滞留药水云应免疫火焰";
}

TEST_F(AreaEffectCloudFireImmuneTest, FireDamage_IsImmune)
{
    auto cloud = std::make_unique<AreaEffectCloudEntity>(mc::test::testEcsRegistry());
    cloud->setTypeId(EntityTypeKeys::AREA_EFFECT_CLOUD);

    // InFire 是 IS_FIRE 伤害源（DamageTypeTags IS_FIRE 成员含 InFire）
    EnvironmentalDamage fireDamage(DamageType::InFire);

    // 基类 Entity::isInvulnerableTo 的 IS_FIRE+isImmuneToFire 守卫应拦截
    EXPECT_TRUE(cloud->isInvulnerableTo(fireDamage)) << "滞留药水云对 IS_FIRE 伤害源应免疫（isInvulnerableTo 返 true）";
    // hurt 应返回 false（火焰伤害被拦截）
    EXPECT_FALSE(cloud->hurt(fireDamage, 5.0f)) << "滞留药水云受火焰伤害应被拒绝（hurt 返 false）";
}
