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

#include "entity/core/LivingEntity.hpp"
#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectType.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/potion/PotionUtils.hpp"
#include "world/IWorld.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试固定装置
// ============================================================================

class ArrowEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品注册表
        Items::initialize();
    }

    void TearDown() override {}
};

// ============================================================================
// ArrowEntity 药水效果测试
// ============================================================================

TEST_F(ArrowEntityTest, AddEffect_SingleEffect_StoredCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加单个药水效果
    entity::effect::EffectInstance poisonEffect(entity::effect::EffectType::Poison, 200, 1);
    arrow->addEffect(poisonEffect);

    // 验证效果已存储
    EXPECT_TRUE(arrow->hasEffects());
    const auto& effects = arrow->effects();
    ASSERT_EQ(effects.size(), 1);
    EXPECT_EQ(effects[0].type(), entity::effect::EffectType::Poison);
    EXPECT_EQ(effects[0].duration(), 200);
    EXPECT_EQ(effects[0].amplifier(), 1);
}

TEST_F(ArrowEntityTest, AddEffect_MultipleEffects_AllStored)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加多个药水效果
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 200, 1));
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Weakness, 150, 2));

    // 验证所有效果已存储
    EXPECT_TRUE(arrow->hasEffects());
    const auto& effects = arrow->effects();
    ASSERT_EQ(effects.size(), 3);
    EXPECT_EQ(effects[0].type(), entity::effect::EffectType::Poison);
    EXPECT_EQ(effects[1].type(), entity::effect::EffectType::Slowness);
    EXPECT_EQ(effects[2].type(), entity::effect::EffectType::Weakness);
}

TEST_F(ArrowEntityTest, SetEffects_ReplacesExistingEffects)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加初始效果
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));

    // 用新效果列表替换
    std::vector<entity::effect::EffectInstance> newEffects;
    newEffects.emplace_back(entity::effect::EffectType::Regeneration, 300, 0);
    newEffects.emplace_back(entity::effect::EffectType::Speed, 400, 1);
    arrow->setEffects(newEffects);

    // 验证只有新效果存在
    const auto& effects = arrow->effects();
    ASSERT_EQ(effects.size(), 2);
    EXPECT_EQ(effects[0].type(), entity::effect::EffectType::Regeneration);
    EXPECT_EQ(effects[1].type(), entity::effect::EffectType::Speed);
}

TEST_F(ArrowEntityTest, HasEffects_NoEffects_ReturnsFalse)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(arrow->hasEffects());
}

TEST_F(ArrowEntityTest, HasEffects_WithEffects_ReturnsTrue)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));
    EXPECT_TRUE(arrow->hasEffects());
}

// ============================================================================
// ArrowEntity 颜色测试
// ============================================================================

TEST_F(ArrowEntityTest, Color_DefaultIsWhite)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(arrow->color(), 0xFFFFFFFF);
}

TEST_F(ArrowEntityTest, SetColor_StoresCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置红色 (ARGB 格式)
    arrow->setColor(0xFFFF0000);
    EXPECT_EQ(arrow->color(), 0xFFFF0000);

    // 设置绿色
    arrow->setColor(0xFF00FF00);
    EXPECT_EQ(arrow->color(), 0xFF00FF00);

    // 设置蓝色
    arrow->setColor(0xFF0000FF);
    EXPECT_EQ(arrow->color(), 0xFF0000FF);
}

TEST_F(ArrowEntityTest, SetColor_CustomPotionColor)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置自定义颜色 (如中毒的绿色)
    u32 poisonColor = 0x4E9331FF;
    arrow->setColor(poisonColor);
    EXPECT_EQ(arrow->color(), poisonColor);
}

// ============================================================================
// ArrowEntity::getArrowStack NBT 标签测试
// ============================================================================

TEST_F(ArrowEntityTest, GetArrowStack_NoEffects_ReturnsRegularArrow)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 无效果的箭矢应返回普通箭矢
    ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), Items::ARROW);
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(ArrowEntityTest, GetArrowStack_WithEffects_ReturnsTippedArrow)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加药水效果
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));

    // 药水箭应返回 TIPPED_ARROW
    ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), Items::TIPPED_ARROW);
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(ArrowEntityTest, GetArrowStack_EffectsStoredInNBT)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 添加药水效果
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 200, 1));
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Slowness, 300, 0));

    // 获取物品堆
    ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), Items::TIPPED_ARROW);

    // 验证效果已存储到 NBT
    auto effects = potion::PotionUtils::getCustomEffects(stack);
    ASSERT_EQ(effects.size(), 2);
    EXPECT_EQ(effects[0].type(), entity::effect::EffectType::Poison);
    EXPECT_EQ(effects[0].duration(), 200);
    EXPECT_EQ(effects[0].amplifier(), 1);
    EXPECT_EQ(effects[1].type(), entity::effect::EffectType::Slowness);
    EXPECT_EQ(effects[1].duration(), 300);
    EXPECT_EQ(effects[1].amplifier(), 0);
}

TEST_F(ArrowEntityTest, GetArrowStack_ColorStoredInNBT)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置颜色和效果
    arrow->setColor(0xFF5733FF); // 自定义颜色
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));

    // 获取物品堆
    ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);

    // 验证颜色已存储
    auto customColor = potion::PotionUtils::getCustomPotionColor(stack);
    ASSERT_TRUE(customColor.has_value());
    EXPECT_EQ(customColor.value(), 0xFF5733FF);
}

TEST_F(ArrowEntityTest, GetArrowStack_NoColorDefaultNotStored)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 只添加效果，不设置颜色
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));

    // 获取物品堆
    ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);

    // 默认颜色不应存储
    auto customColor = potion::PotionUtils::getCustomPotionColor(stack);
    EXPECT_FALSE(customColor.has_value());
}

// ============================================================================
// ArrowEntity 效果颜色计算测试
// ============================================================================

TEST_F(ArrowEntityTest, EffectColor_PoisonIsGreen)
{
    // 验证中毒效果的颜色
    u32 poisonColor = potion::PotionUtils::getEffectColor(entity::effect::EffectType::Poison);
    EXPECT_EQ(poisonColor, 0x4E9331FF); // 绿色 (ARGB)
}

TEST_F(ArrowEntityTest, EffectColor_SpeedIsLightBlue)
{
    // 验证速度效果的颜色
    u32 speedColor = potion::PotionUtils::getEffectColor(entity::effect::EffectType::Speed);
    EXPECT_EQ(speedColor, 0x7CAFC6FF); // 淡蓝色 (ARGB)
}

TEST_F(ArrowEntityTest, EffectColor_RegenerationIsPink)
{
    // 验证生命恢复效果的颜色
    u32 regenColor = potion::PotionUtils::getEffectColor(entity::effect::EffectType::Regeneration);
    EXPECT_EQ(regenColor, 0xCD5CABFF); // 粉红色 (ARGB)
}

TEST_F(ArrowEntityTest, ColorCalculation_MultipleEffects_Average)
{
    // 创建多个效果，验证颜色计算
    std::vector<entity::effect::EffectInstance> effects;
    effects.emplace_back(entity::effect::EffectType::Poison, 100, 0);       // 0x4E9331FF
    effects.emplace_back(entity::effect::EffectType::Regeneration, 100, 0); // 0xCD5CABFF

    u32 calculatedColor = potion::PotionUtils::getColor(effects);
    // 颜色应该是平均值（简化验证：颜色存在且非零）
    EXPECT_NE(calculatedColor, 0);
    EXPECT_NE(calculatedColor, 0xFFFFFFFF);
}

// ============================================================================
// ArrowEntity 发光状态测试
// ============================================================================

TEST_F(ArrowEntityTest, Glowing_DefaultFalse)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(arrow->isGlowing());
}

TEST_F(ArrowEntityTest, SetGlowing_StoresCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());

    arrow->setGlowing(true);
    EXPECT_TRUE(arrow->isGlowing());

    arrow->setGlowing(false);
    EXPECT_FALSE(arrow->isGlowing());
}

// ============================================================================
// SpectralArrowEntity 测试
// ============================================================================

TEST_F(ArrowEntityTest, SpectralArrow_DefaultDamage)
{
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(spectralArrow->damage(), 2.0f);
}

TEST_F(ArrowEntityTest, SpectralArrow_GetArrowStack_ReturnsSpectralArrow)
{
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ItemStack stack = spectralArrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), Items::SPECTRAL_ARROW);
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// 颜色 RGB 分量提取测试 (用于粒子效果)
// ============================================================================

TEST_F(ArrowEntityTest, ColorRGBExtraction_RedChannel)
{
    u32 color = 0xFFFF0000; // 红色
    f32 r = static_cast<f32>((color >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((color >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(color & 0xFF) / 255.0f;

    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
    EXPECT_FLOAT_EQ(b, 0.0f);
}

TEST_F(ArrowEntityTest, ColorRGBExtraction_GreenChannel)
{
    u32 color = 0xFF00FF00; // 绿色
    f32 r = static_cast<f32>((color >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((color >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(color & 0xFF) / 255.0f;

    EXPECT_FLOAT_EQ(r, 0.0f);
    EXPECT_FLOAT_EQ(g, 1.0f);
    EXPECT_FLOAT_EQ(b, 0.0f);
}

TEST_F(ArrowEntityTest, ColorRGBExtraction_BlueChannel)
{
    u32 color = 0xFF0000FF; // 蓝色
    f32 r = static_cast<f32>((color >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((color >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(color & 0xFF) / 255.0f;

    EXPECT_FLOAT_EQ(r, 0.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
    EXPECT_FLOAT_EQ(b, 1.0f);
}

TEST_F(ArrowEntityTest, ColorRGBExtraction_PoisonGreen)
{
    // 中毒效果的颜色 (ARGB 格式): 0x4E9331FF
    // A=0x4E, R=0x93, G=0x31, B=0xFF
    u32 color = 0x4E9331FF;
    u8 r = (color >> 16) & 0xFF;
    u8 g = (color >> 8) & 0xFF;
    u8 b = color & 0xFF;

    // 验证 RGB 分量提取正确
    EXPECT_EQ(r, 0x93); // 红色分量
    EXPECT_EQ(g, 0x31); // 绿色分量
    EXPECT_EQ(b, 0xFF); // 蓝色分量
}

TEST_F(ArrowEntityTest, ColorRGBExtraction_DefaultColorIsWhite)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    u32 color = arrow->color();

    // 默认颜色是白色 (0xFFFFFFFF)
    EXPECT_EQ(color, 0xFFFFFFFF);

    // RGB 分量验证
    f32 r = static_cast<f32>((color >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((color >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(color & 0xFF) / 255.0f;

    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 1.0f);
    EXPECT_FLOAT_EQ(b, 1.0f);
}

// ============================================================================
// 效果类型验证测试
// ============================================================================

TEST_F(ArrowEntityTest, EffectType_PoisonExists)
{
    // 验证中毒效果类型存在
    entity::effect::EffectInstance poison(entity::effect::EffectType::Poison, 100, 0);
    EXPECT_EQ(poison.type(), entity::effect::EffectType::Poison);
    EXPECT_EQ(poison.duration(), 100);
    EXPECT_EQ(poison.amplifier(), 0);
}

TEST_F(ArrowEntityTest, EffectType_RegenerationExists)
{
    // 验证生命恢复效果类型存在
    entity::effect::EffectInstance regen(entity::effect::EffectType::Regeneration, 200, 2);
    EXPECT_EQ(regen.type(), entity::effect::EffectType::Regeneration);
    EXPECT_EQ(regen.duration(), 200);
    EXPECT_EQ(regen.amplifier(), 2);
}

TEST_F(ArrowEntityTest, EffectType_SpeedExists)
{
    // 验证速度效果类型存在
    entity::effect::EffectInstance speed(entity::effect::EffectType::Speed, 300, 1);
    EXPECT_EQ(speed.type(), entity::effect::EffectType::Speed);
    EXPECT_EQ(speed.duration(), 300);
    EXPECT_EQ(speed.amplifier(), 1);
}
