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
 * @file PvpGameRuleTest.cpp
 * @brief PVP 游戏规则单元测试
 *
 * 测试 GameRuleKeys::PVP 的默认值、设置、获取、重置、序列化等行为。
 */

#include "common/world/gamerule/GameRule.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <gtest/gtest.h>

using namespace mc::world::gamerule;

// ============================================================================
// PVP 游戏规则基本测试
// ============================================================================

TEST(PvpGameRuleTest, DefaultIsTrue)
{
    GameRules rules;
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, SetToFalse)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, SetToTrue)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));

    rules.setBoolean(GameRuleKeys::PVP, true, nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, ResetRestoresDefault)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));

    rules.reset("pvp", nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, ResetAllRestoresDefault)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));

    rules.resetAll();
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, KeyName)
{
    EXPECT_EQ(GameRuleKeys::PVP.getName(), "pvp");
}

TEST(PvpGameRuleTest, KeyCategory)
{
    EXPECT_EQ(GameRuleKeys::PVP.getCategory(), GameRuleCategory::Player);
}

TEST(PvpGameRuleTest, KeyTranslationKey)
{
    EXPECT_EQ(GameRuleKeys::PVP.getTranslationKey(), "gamerule.pvp");
}

TEST(PvpGameRuleTest, HasRule)
{
    EXPECT_TRUE(GameRules::hasRule("pvp"));
}

TEST(PvpGameRuleTest, GetRuleType)
{
    auto type = GameRules::getRuleType("pvp");
    EXPECT_TRUE(type.has_value());
    EXPECT_EQ(type.value(), GameRuleValueType::Boolean);
}

TEST(PvpGameRuleTest, SetFromStringTrue)
{
    GameRules rules;
    EXPECT_TRUE(rules.setFromString("pvp", "true"));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, SetFromStringFalse)
{
    GameRules rules;
    EXPECT_TRUE(rules.setFromString("pvp", "false"));
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, NbtRoundTrip)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);

    auto nbt = rules.write();
    ASSERT_NE(nbt, nullptr);

    GameRules loadedRules(*nbt);
    EXPECT_FALSE(loadedRules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, NbtRoundTripDefault)
{
    GameRules rules;
    auto nbt = rules.write();
    ASSERT_NE(nbt, nullptr);

    GameRules loadedRules(*nbt);
    EXPECT_TRUE(loadedRules.getBoolean(GameRuleKeys::PVP));
}

TEST(PvpGameRuleTest, IndependenceFromOtherRules)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);
    rules.setBoolean(GameRuleKeys::KEEP_INVENTORY, true, nullptr);

    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::KEEP_INVENTORY));

    // 修改 KEEP_INVENTORY 不影响 PVP
    rules.setBoolean(GameRuleKeys::KEEP_INVENTORY, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::PVP));
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::KEEP_INVENTORY));
}

TEST(PvpGameRuleTest, GetBooleanValueObject)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::PVP, false, nullptr);

    const auto& value = rules.getBooleanValue(GameRuleKeys::PVP);
    EXPECT_FALSE(value.get());
    EXPECT_FALSE(value.isDefault());
}

TEST(PvpGameRuleTest, IsInRuleNamesList)
{
    auto names = GameRules::getRuleNames();
    auto it = std::find(names.begin(), names.end(), "pvp");
    EXPECT_NE(it, names.end());
}
