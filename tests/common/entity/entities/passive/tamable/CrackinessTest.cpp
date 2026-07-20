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

#include "common/entity/entities/passive/tamable/Crackiness.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// Crackiness 通用测试
// ============================================================================

TEST(CrackinessTest, ByFraction_NoneWhenHighDurability)
{
    // 剩余耐久 100% → None
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    EXPECT_EQ(crackiness.byFraction(1.0f), Crackiness::Level::None);
    EXPECT_EQ(crackiness.byFraction(0.8f), Crackiness::Level::None);
}

TEST(CrackinessTest, ByFraction_LowLevel)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    EXPECT_EQ(crackiness.byFraction(0.74f), Crackiness::Level::Low);
    EXPECT_EQ(crackiness.byFraction(0.6f), Crackiness::Level::Low);
    EXPECT_EQ(crackiness.byFraction(0.51f), Crackiness::Level::Low);
}

TEST(CrackinessTest, ByFraction_MediumLevel)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    EXPECT_EQ(crackiness.byFraction(0.49f), Crackiness::Level::Medium);
    EXPECT_EQ(crackiness.byFraction(0.4f), Crackiness::Level::Medium);
    EXPECT_EQ(crackiness.byFraction(0.26f), Crackiness::Level::Medium);
}

TEST(CrackinessTest, ByFraction_HighLevel)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    EXPECT_EQ(crackiness.byFraction(0.24f), Crackiness::Level::High);
    EXPECT_EQ(crackiness.byFraction(0.1f), Crackiness::Level::High);
    EXPECT_EQ(crackiness.byFraction(0.0f), Crackiness::Level::High);
}

TEST(CrackinessTest, ByDamage_NoneWhenNoDamage)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    // 0 damage / 100 max = 100% remaining → None
    EXPECT_EQ(crackiness.byDamage(0, 100), Crackiness::Level::None);
}

TEST(CrackinessTest, ByDamage_ZeroMaxDamage_ReturnsNone)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    // maxDamage <= 0 时返回 None
    EXPECT_EQ(crackiness.byDamage(10, 0), Crackiness::Level::None);
    EXPECT_EQ(crackiness.byDamage(10, -1), Crackiness::Level::None);
}

TEST(CrackinessTest, ByDamage_FullDamage_HighCrack)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    // 100 damage / 100 max = 0% remaining → High
    EXPECT_EQ(crackiness.byDamage(100, 100), Crackiness::Level::High);
}

TEST(CrackinessTest, ByDamage_HalfDamage_MediumCrack)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    // 51 damage / 100 max = 49% remaining → Medium (49% < 50%)
    EXPECT_EQ(crackiness.byDamage(51, 100), Crackiness::Level::Medium);
}

TEST(CrackinessTest, ByDamage_SlightDamage_LowCrack)
{
    Crackiness crackiness(0.75f, 0.5f, 0.25f);
    // 30 damage / 100 max = 70% remaining → Low
    EXPECT_EQ(crackiness.byDamage(30, 100), Crackiness::Level::Low);
}

// ============================================================================
// Golem 裂纹阈值测试
// ============================================================================

TEST(CrackinessTest, GolemThresholds)
{
    // 铁傀儡：剩余 < 75% → Low, < 50% → Medium, < 25% → High
    EXPECT_EQ(Crackiness::GOLEM.byFraction(1.0f), Crackiness::Level::None);
    EXPECT_EQ(Crackiness::GOLEM.byFraction(0.76f), Crackiness::Level::None);
    EXPECT_EQ(Crackiness::GOLEM.byFraction(0.74f), Crackiness::Level::Low);
    EXPECT_EQ(Crackiness::GOLEM.byFraction(0.49f), Crackiness::Level::Medium);
    EXPECT_EQ(Crackiness::GOLEM.byFraction(0.24f), Crackiness::Level::High);
    EXPECT_EQ(Crackiness::GOLEM.byFraction(0.0f), Crackiness::Level::High);
}

// ============================================================================
// WOLF_ARMOR 裂纹阈值测试
// ============================================================================

TEST(CrackinessTest, WolfArmorThresholds)
{
    // 狼铠：剩余 < 95% → Low, < 69% → Medium, < 32% → High
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(1.0f), Crackiness::Level::None);
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(0.96f), Crackiness::Level::None);
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(0.94f), Crackiness::Level::Low);
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(0.7f), Crackiness::Level::Low);
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(0.68f), Crackiness::Level::Medium);
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(0.5f), Crackiness::Level::Medium);
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(0.31f), Crackiness::Level::High);
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byFraction(0.0f), Crackiness::Level::High);
}

TEST(CrackinessTest, WolfArmor_DamageThresholds)
{
    // 狼铠耐久 64 点
    // 无损伤 → None
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(0, 64), Crackiness::Level::None);
    // 3 点损伤 → 61/64 = 95.3% → None（>= 95%）
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(3, 64), Crackiness::Level::None);
    // 4 点损伤 → 60/64 = 93.75% → Low（< 95%）
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(4, 64), Crackiness::Level::Low);
    // 19 点损伤 → 45/64 = 70.3% → Low（>= 69%）
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(19, 64), Crackiness::Level::Low);
    // 20 点损伤 → 44/64 = 68.75% → Medium（< 69%）
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(20, 64), Crackiness::Level::Medium);
    // 43 点损伤 → 21/64 = 32.8% → Medium（>= 32%）
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(43, 64), Crackiness::Level::Medium);
    // 44 点损伤 → 20/64 = 31.25% → High（< 32%）
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(44, 64), Crackiness::Level::High);
    // 64 点损伤 → 0/64 = 0% → High
    EXPECT_EQ(Crackiness::WOLF_ARMOR.byDamage(64, 64), Crackiness::Level::High);
}
