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

// 箭矢 doPostHurtEffects 单元测试（任务 #285）。
//
// 验证 ArrowEntity::doPostHurtEffects / SpectralArrowEntity::doPostHurtEffects 在被调用时
// 正确施加药水/发光效果到目标 LivingEntity。对齐 vanilla Arrow.doPostHurtEffects:113-119
// （potioncontents.forEachEffect addEffect）与 SpectralArrow.doPostHurtEffects:41-45
// （addEffect Glowing duration amplifier=0）。
//
// 任务 #285 修复引入 doPostHurtEffects 虚函数范式：父类 AbstractArrowEntity::onEntityHit 在
// hurt 成功后调 doPostHurtEffects，子类重写施加效果。
//
// 测试访问方式：doPostHurtEffects 在 AbstractArrowEntity 为 protected virtual，子类重写为
// protected。测试经 TestArrowEntity/TestSpectralArrowEntity 公有派生暴露：
// - ArrowEntity 测试经 callDoPostHurtEffectsDirect() 内部 ArrowEntity::doPostHurtEffects
//   限定名非虚调用，验证重写实现逻辑正确（施加 effects() 列表中的药水效果）。
// - SpectralArrowEntity 测试经 using 暴露 doPostHurtEffects 为 public，虚派发调用，验证
//   重写实现 + 虚派发链路（施加 Glowing）。
//
// hurt 门控逻辑（hurt 失败不调 doPostHurtEffects）由 onEntityHit 内 if(hurt) 保证，
// 集成测试 spectral_arrow_inflicts_glowing 端到端覆盖 hurt 成功→施加 Glowing 全链路。

#include "entity/core/LivingEntity.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectType.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "item/Items.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试用公有派生类：暴露 protected doPostHurtEffects 为 public，供单元测试调用。
// ============================================================================

class TestArrowEntity : public ArrowEntity {
public:
    using ArrowEntity::ArrowEntity;
    // 暴露 protected override 为 public，并提供限定名非虚调用入口（验证重写实现逻辑）。
    using ArrowEntity::doPostHurtEffects;
    void callDoPostHurtEffectsDirect(LivingEntity& target) { ArrowEntity::doPostHurtEffects(target); }
};

class TestSpectralArrowEntity : public SpectralArrowEntity {
public:
    using SpectralArrowEntity::doPostHurtEffects; // 暴露 protected override 为 public
    using SpectralArrowEntity::SpectralArrowEntity;
};

// ============================================================================
// 测试固定装置
// ============================================================================

class ArrowPostHurtEffectsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品注册表（部分实体构造/效果系统间接依赖）
        Items::initialize();
    }

    void TearDown() override {}
};

// ============================================================================
// ArrowEntity::doPostHurtEffects 测试
// ============================================================================

// 药水箭 doPostHurtEffects 施加携带的药水效果到目标（对齐 vanilla Arrow.doPostHurtEffects）。
//
// ArrowEntity 携带 Poison 效果（addEffect），doPostHurtEffects(target) 应将 Poison 施加到 target。
// 验证 target.hasEffect(Poison) 为 true，且效果参数（amplifier）与箭矢携带一致。
TEST_F(ArrowPostHurtEffectsTest, ArrowDoPostHurtEffects_AppliesPotionEffectsToTarget)
{
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 箭矢携带 Poison II（amplifier=1），duration=200
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 200, 1));

    auto target = std::make_unique<LivingEntity>(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());

    // 调用 doPostHurtEffects（模拟 onEntityHit 内 hurt 成功后的后置效果施加）
    arrow->callDoPostHurtEffectsDirect(*target);

    // 验证 Poison 已施加到 target
    EXPECT_TRUE(target->hasEffect(entity::effect::EffectType::Poison))
        << "ArrowEntity::doPostHurtEffects should apply carried Poison effect to target";
    const auto* inst = target->getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->amplifier(), 1) << "Poison amplifier should match arrow's carried effect (II)";
}

// 无药水效果的普通箭 doPostHurtEffects 不施加任何效果（对齐 vanilla Arrow 空 PotionContents）。
//
// 普通箭（无 addEffect）doPostHurtEffects 应为 no-op，target 无任何效果。
TEST_F(ArrowPostHurtEffectsTest, ArrowDoPostHurtEffects_NoEffects_DoesNothing)
{
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(3), mc::test::testEcsRegistry());
    // 不添加任何药水效果（普通箭）

    auto target = std::make_unique<LivingEntity>(EntityInstanceId(4), nullptr, mc::test::testEcsRegistry());

    arrow->callDoPostHurtEffectsDirect(*target);

    // 普通箭无效果，target 不应有 Poison/Glowing 等
    EXPECT_FALSE(target->hasEffect(entity::effect::EffectType::Poison));
    EXPECT_FALSE(target->hasEffect(entity::effect::EffectType::Glowing));
}

// 药水箭 doPostHurtEffects 施加多个药水效果（对齐 vanilla forEachEffect 遍历全部效果）。
TEST_F(ArrowPostHurtEffectsTest, ArrowDoPostHurtEffects_AppliesMultipleEffects)
{
    auto arrow = std::make_unique<TestArrowEntity>(EntityInstanceId(5), mc::test::testEcsRegistry());
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 200, 1));

    auto target = std::make_unique<LivingEntity>(EntityInstanceId(6), nullptr, mc::test::testEcsRegistry());

    arrow->callDoPostHurtEffectsDirect(*target);

    EXPECT_TRUE(target->hasEffect(entity::effect::EffectType::Poison));
    EXPECT_TRUE(target->hasEffect(entity::effect::EffectType::Slowness));
}

// ============================================================================
// SpectralArrowEntity::doPostHurtEffects 测试
// ============================================================================

// 光灵箭 doPostHurtEffects 施加 Glowing 效果到目标（对齐 vanilla SpectralArrow.doPostHurtEffects:41-45）。
//
// SpectralArrowEntity::doPostHurtEffects 应施加 Glowing（amplifier=0，duration=glowDuration()=200）。
// 验证 target.hasEffect(Glowing) 为 true，amplifier===0（Glowing I），duration===200。
TEST_F(ArrowPostHurtEffectsTest, SpectralDoPostHurtEffects_AppliesGlowingToTarget)
{
    auto spectral = std::make_unique<TestSpectralArrowEntity>(EntityInstanceId(7), mc::test::testEcsRegistry());

    auto target = std::make_unique<LivingEntity>(EntityInstanceId(8), nullptr, mc::test::testEcsRegistry());

    spectral->doPostHurtEffects(*target);

    // 验证 Glowing 已施加
    EXPECT_TRUE(target->hasEffect(entity::effect::EffectType::Glowing))
        << "SpectralArrowEntity::doPostHurtEffects should apply Glowing to target";
    const auto* inst = target->getEffect(entity::effect::EffectType::Glowing);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->amplifier(), 0) << "Glowing amplifier should be 0 (Glowing I)";
    // duration 应为 glowDuration() 默认 200（对齐 vanilla SpectralArrow DEFAULT_DURATION=200）
    EXPECT_EQ(inst->duration(), 200) << "Glowing duration should be 200 (DEFAULT_DURATION)";
}

// 光灵箭 glowDuration 可定制：setGlowDuration 后 doPostHurtEffects 施加的 Glowing duration 跟随。
//
// 验证 setGlowDuration 修改后 doPostHurtEffects 施加的 Glowing duration 与设置值一致。
TEST_F(ArrowPostHurtEffectsTest, SpectralDoPostHurtEffects_CustomGlowDuration)
{
    auto spectral = std::make_unique<TestSpectralArrowEntity>(EntityInstanceId(9), mc::test::testEcsRegistry());
    spectral->setGlowDuration(400); // 自定义 400 tick

    auto target = std::make_unique<LivingEntity>(EntityInstanceId(10), nullptr, mc::test::testEcsRegistry());

    spectral->doPostHurtEffects(*target);

    const auto* inst = target->getEffect(entity::effect::EffectType::Glowing);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->duration(), 400) << "Glowing duration should follow custom glowDuration";
}
