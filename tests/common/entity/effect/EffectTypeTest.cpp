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

#include "entity/effect/EffectType.hpp"
#include "resource/ResourceLocation.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::effect;

/**
 * @brief EffectType 工具函数测试
 *
 * 测试效果类型的 ID 转换、资源位置转换等功能
 */
class EffectTypeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// getEffectById 测试
// ============================================================================

TEST_F(EffectTypeTest, GetEffectById_ValidIds)
{
    // 测试有效ID
    EXPECT_EQ(EffectType::Speed, getEffectById(1).value());
    EXPECT_EQ(EffectType::Slowness, getEffectById(2).value());
    EXPECT_EQ(EffectType::Haste, getEffectById(3).value());
    EXPECT_EQ(EffectType::InstantHealth, getEffectById(6).value());
    EXPECT_EQ(EffectType::Poison, getEffectById(19).value());
    EXPECT_EQ(EffectType::HeroOfTheVillage, getEffectById(32).value());
}

TEST_F(EffectTypeTest, GetEffectById_InvalidIds)
{
    // 测试无效ID
    EXPECT_FALSE(getEffectById(0).has_value());
    EXPECT_FALSE(getEffectById(-1).has_value());
    EXPECT_FALSE(getEffectById(33).has_value());
    EXPECT_FALSE(getEffectById(100).has_value());
}

// ============================================================================
// getEffectByResourceLocation 测试
// ============================================================================

TEST_F(EffectTypeTest, GetEffectByResourceLocation_FullResourceLocation)
{
    // 测试完整资源位置格式
    EXPECT_EQ(EffectType::Speed, getEffectByResourceLocation(ResourceLocation("minecraft:speed")).value());
    EXPECT_EQ(EffectType::Poison, getEffectByResourceLocation(ResourceLocation("minecraft:poison")).value());
    EXPECT_EQ(
        EffectType::InstantHealth, getEffectByResourceLocation(ResourceLocation("minecraft:instant_health")).value());
    EXPECT_EQ(EffectType::HeroOfTheVillage,
        getEffectByResourceLocation(ResourceLocation("minecraft:hero_of_the_village")).value());
    EXPECT_EQ(EffectType::SlowFalling, getEffectByResourceLocation(ResourceLocation("minecraft:slow_falling")).value());
    EXPECT_EQ(
        EffectType::DolphinsGrace, getEffectByResourceLocation(ResourceLocation("minecraft:dolphins_grace")).value());
}

TEST_F(EffectTypeTest, GetEffectByResourceLocation_PathOnly)
{
    // 测试仅路径格式（不带命名空间）
    EXPECT_EQ(EffectType::Speed, getEffectByResourceLocation(ResourceLocation("speed")).value());
    EXPECT_EQ(EffectType::Poison, getEffectByResourceLocation(ResourceLocation("poison")).value());
    EXPECT_EQ(EffectType::Regeneration, getEffectByResourceLocation(ResourceLocation("regeneration")).value());
    EXPECT_EQ(EffectType::NightVision, getEffectByResourceLocation(ResourceLocation("night_vision")).value());
}

TEST_F(EffectTypeTest, GetEffectByResourceLocation_InvalidName)
{
    // 测试无效名称
    EXPECT_FALSE(getEffectByResourceLocation(ResourceLocation("minecraft:invalid_effect")).has_value());
    EXPECT_FALSE(getEffectByResourceLocation(ResourceLocation("invalid_effect")).has_value());
    EXPECT_FALSE(getEffectByResourceLocation(ResourceLocation("")).has_value());
}

// ============================================================================
// getEffectResourceLocation 测试
// ============================================================================

TEST_F(EffectTypeTest, GetEffectResourceLocation_CommonEffects)
{
    // 测试常见效果
    EXPECT_EQ(ResourceLocation("minecraft:speed"), getEffectResourceLocation(EffectType::Speed));
    EXPECT_EQ(ResourceLocation("minecraft:poison"), getEffectResourceLocation(EffectType::Poison));
    EXPECT_EQ(ResourceLocation("minecraft:instant_health"), getEffectResourceLocation(EffectType::InstantHealth));
    EXPECT_EQ(ResourceLocation("minecraft:regeneration"), getEffectResourceLocation(EffectType::Regeneration));
}

TEST_F(EffectTypeTest, GetEffectResourceLocation_AllEffects)
{
    // 验证所有效果都有有效的资源位置
    for (i32 id = 1; id <= 32; ++id) {
        auto type = getEffectById(id);
        ASSERT_TRUE(type.has_value()) << "Effect ID " << id << " should be valid";

        ResourceLocation loc = getEffectResourceLocation(type.value());
        EXPECT_EQ("minecraft", loc.namespace_()) << "Effect ID " << id << " should have minecraft namespace";
        EXPECT_FALSE(loc.path().empty()) << "Effect ID " << id << " should have non-empty path";
    }
}

// ============================================================================
// getEffectResourceName 测试
// ============================================================================

TEST_F(EffectTypeTest, GetEffectResourceName_CommonEffects)
{
    EXPECT_STREQ("speed", getEffectResourceName(EffectType::Speed));
    EXPECT_STREQ("poison", getEffectResourceName(EffectType::Poison));
    EXPECT_STREQ("instant_health", getEffectResourceName(EffectType::InstantHealth));
    EXPECT_STREQ("night_vision", getEffectResourceName(EffectType::NightVision));
    EXPECT_STREQ("slow_falling", getEffectResourceName(EffectType::SlowFalling));
}

TEST_F(EffectTypeTest, GetEffectResourceName_InvalidEffect)
{
    // 无效效果应该返回 "unknown"
    EXPECT_STREQ("unknown", getEffectResourceName(static_cast<EffectType>(0)));
    EXPECT_STREQ("unknown", getEffectResourceName(static_cast<EffectType>(100)));
}

// ============================================================================
// isInstantEffect 测试
// ============================================================================

TEST_F(EffectTypeTest, IsInstantEffect_InstantEffects)
{
    // 瞬间效果
    EXPECT_TRUE(isInstantEffect(EffectType::InstantHealth));
    EXPECT_TRUE(isInstantEffect(EffectType::InstantDamage));
    EXPECT_TRUE(isInstantEffect(EffectType::Saturation));
}

TEST_F(EffectTypeTest, IsInstantEffect_NonInstantEffects)
{
    // 非瞬间效果
    EXPECT_FALSE(isInstantEffect(EffectType::Speed));
    EXPECT_FALSE(isInstantEffect(EffectType::Poison));
    EXPECT_FALSE(isInstantEffect(EffectType::Regeneration));
    EXPECT_FALSE(isInstantEffect(EffectType::NightVision));
    EXPECT_FALSE(isInstantEffect(EffectType::SlowFalling));
}

// ============================================================================
// 往返转换测试
// ============================================================================

TEST_F(EffectTypeTest, RoundTrip_ResourceLocation)
{
    // 验证从类型到资源位置再转回类型的一致性
    for (i32 id = 1; id <= 32; ++id) {
        auto originalType = getEffectById(id);
        ASSERT_TRUE(originalType.has_value()) << "Effect ID " << id << " should be valid";

        ResourceLocation loc = getEffectResourceLocation(originalType.value());
        auto recoveredType = getEffectByResourceLocation(loc);

        ASSERT_TRUE(recoveredType.has_value()) << "Resource location " << loc.toString() << " should be valid";
        EXPECT_EQ(originalType.value(), recoveredType.value())
            << "Effect ID " << id << " should round-trip through ResourceLocation";
    }
}

TEST_F(EffectTypeTest, RoundTrip_NumericId)
{
    // 验证从类型到数值ID再转回类型的一致性
    for (i32 id = 1; id <= 32; ++id) {
        auto originalType = getEffectById(id);
        ASSERT_TRUE(originalType.has_value()) << "Effect ID " << id << " should be valid";

        i32 recoveredId = static_cast<i32>(originalType.value());
        EXPECT_EQ(id, recoveredId) << "Effect ID " << id << " should round-trip through numeric ID";
    }
}

// ============================================================================
// getEffectName 测试（已有函数，确保未被破坏）
// ============================================================================

TEST_F(EffectTypeTest, GetEffectName_CommonEffects)
{
    // 确保显示名称函数正常工作
    EXPECT_STREQ("Speed", getEffectName(EffectType::Speed));
    EXPECT_STREQ("Poison", getEffectName(EffectType::Poison));
    EXPECT_STREQ("Instant Health", getEffectName(EffectType::InstantHealth));
    EXPECT_STREQ("Night Vision", getEffectName(EffectType::NightVision));
    EXPECT_STREQ("Dolphin's Grace", getEffectName(EffectType::DolphinsGrace));
}

// ============================================================================
// isBeneficialEffect 测试（已有函数，确保未被破坏）
// ============================================================================

TEST_F(EffectTypeTest, IsBeneficialEffect_BeneficialEffects)
{
    // 有益效果
    EXPECT_TRUE(isBeneficialEffect(EffectType::Speed));
    EXPECT_TRUE(isBeneficialEffect(EffectType::Regeneration));
    EXPECT_TRUE(isBeneficialEffect(EffectType::FireResistance));
    EXPECT_TRUE(isBeneficialEffect(EffectType::NightVision));
    EXPECT_TRUE(isBeneficialEffect(EffectType::HeroOfTheVillage));
}

TEST_F(EffectTypeTest, IsBeneficialEffect_NonBeneficialEffects)
{
    // 无益/有害效果
    EXPECT_FALSE(isBeneficialEffect(EffectType::Poison));
    EXPECT_FALSE(isBeneficialEffect(EffectType::Wither));
    EXPECT_FALSE(isBeneficialEffect(EffectType::Nausea));
    EXPECT_FALSE(isBeneficialEffect(EffectType::BadOmen));
}
