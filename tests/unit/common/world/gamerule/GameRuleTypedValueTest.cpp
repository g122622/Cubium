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
 * @file GameRuleTypedValueTest.cpp
 * @brief 游戏规则的注册表完整性、取值范围与按类名设置单元测试
 */

#include "common/world/gamerule/GameRule.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <gtest/gtest.h>

using namespace mc::world::gamerule;

namespace {

/// 判定规则名在当前版本 vanilla 中是否存在（含 1.21.11 新增规则）
bool _isKnownRuleName(const std::string& name)
{
    return name == "playersNetherPortalDefaultDelay" || name == "playersNetherPortalCreativeDelay" ||
        name == "fire_spread_radius_around_player" || name == "snowAccumulationHeight" || name == "randomTickSpeed" ||
        name == "max_minecart_speed" || name == "spawnRadius";
}

} // namespace

// ============================================================================
// 规则名必须与 vanilla 完全一致
// ============================================================================

TEST(GameRuleTypedValueTest, VanillaRuleNamesAreExact)
{
    EXPECT_EQ(GameRuleKeys::LOCATOR_BAR.getName(), "locator_bar");
    EXPECT_EQ(GameRuleKeys::MAX_MINECART_SPEED.getName(), "max_minecart_speed");
    EXPECT_EQ(GameRuleKeys::PLAYERS_NETHER_PORTAL_DEFAULT_DELAY.getName(), "playersNetherPortalDefaultDelay");
    EXPECT_EQ(GameRuleKeys::PLAYERS_NETHER_PORTAL_CREATIVE_DELAY.getName(), "playersNetherPortalCreativeDelay");
    EXPECT_EQ(GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER.getName(), "fire_spread_radius_around_player");
}

// ============================================================================
// 整型规则必须注册在整数表中，读取才能拿到真实默认值
// ============================================================================

TEST(GameRuleTypedValueTest, IntegerRulesResolveToRegisteredDefaults)
{
    GameRules rules;

    for (const auto& name : GameRules::getRuleNames()) {
        if (!_isKnownRuleName(name)) {
            continue;
        }

        // std::optional 的断言检查不会被静态分析识别，故先取出再比较（已用 gtest 断言兜底）
        const std::optional<std::string_view> ruleClass = GameRules::getRuleClass(name);
        ASSERT_TRUE(ruleClass.has_value()) << name;
        if (ruleClass.value_or(std::string_view{}) != GameRuleClassNames::INTEGER) {
            continue;
        }

        const IntegerGameRuleKey key(name, GameRuleCategory::Misc);
        const std::string defaultValue = GameRules::getDefaultValueAsString(name);
        EXPECT_EQ(std::to_string(rules.getInt(key)), defaultValue) << name;

        // 注册表默认值非空字符串证明规则确实存在，而不是 getInt 的 0 兜底
        EXPECT_FALSE(defaultValue.empty()) << name;
        EXPECT_EQ(rules.getValueAsString(name), defaultValue) << name;
    }
}

TEST(GameRuleTypedValueTest, RandomTickSpeedAndSnowAccumulationAreWired)
{
    GameRules rules;

    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 3);
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT), 1);

    // 修改后必须回读为新值，证明 setInt 命中了整数表而非被静默丢弃
    rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 7, nullptr);
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 7);

    rules.setInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT, 4, nullptr);
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT), 4);
}

// ============================================================================
// 默认值与取值范围
// ============================================================================

TEST(GameRuleTypedValueTest, FireSpreadRadiusDefaultsAndAcceptsMinusOne)
{
    GameRules rules;

    EXPECT_EQ(GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER.getName(), "fire_spread_radius_around_player");
    EXPECT_EQ(rules.getInt(GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER), 128);

    EXPECT_TRUE(rules.setTypedFromString(GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER.getName(),
        std::string(GameRuleClassNames::INTEGER),
        "-1",
        nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER), -1);

    // 低于下界 -1 的值无效，且不得改动既有值
    EXPECT_FALSE(rules.setTypedFromString(GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER.getName(),
        std::string(GameRuleClassNames::INTEGER),
        "-2",
        nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::FIRE_SPREAD_RADIUS_AROUND_PLAYER), -1);
}

TEST(GameRuleTypedValueTest, SnowAccumulationHeightIsCappedAtEight)
{
    GameRules rules;

    EXPECT_TRUE(rules.setTypedFromString(
        GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT.getName(), std::string(GameRuleClassNames::INTEGER), "8", nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT), 8);

    EXPECT_FALSE(rules.setTypedFromString(
        GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT.getName(), std::string(GameRuleClassNames::INTEGER), "9", nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT), 8);

    EXPECT_FALSE(rules.setTypedFromString(
        GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT.getName(), std::string(GameRuleClassNames::INTEGER), "-1", nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT), 8);
}

TEST(GameRuleTypedValueTest, MinecartSpeedRangeIsOneToThousand)
{
    GameRules rules;

    EXPECT_FALSE(rules.setTypedFromString(
        GameRuleKeys::MAX_MINECART_SPEED.getName(), std::string(GameRuleClassNames::INTEGER), "0", nullptr));
    EXPECT_FALSE(rules.setTypedFromString(
        GameRuleKeys::MAX_MINECART_SPEED.getName(), std::string(GameRuleClassNames::INTEGER), "1001", nullptr));
    EXPECT_TRUE(rules.setTypedFromString(
        GameRuleKeys::MAX_MINECART_SPEED.getName(), std::string(GameRuleClassNames::INTEGER), "1000", nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_MINECART_SPEED), 1000);
}

// ============================================================================
// 按类名设置：类名与规则类型必须匹配
// ============================================================================

TEST(GameRuleTypedValueTest, ClassNameMustMatchRuleType)
{
    GameRules rules;
    rules.setBoolean(GameRuleKeys::MOB_GRIEFING, true, nullptr);
    rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 3, nullptr);

    // 布尔规则不接受 integer 类名，反之亦然
    EXPECT_FALSE(rules.setTypedFromString("mobGriefing", std::string(GameRuleClassNames::INTEGER), "0", nullptr));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING));

    EXPECT_FALSE(rules.setTypedFromString("randomTickSpeed", std::string(GameRuleClassNames::BOOLEAN), "1", nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 3);

    // 不存在的规则
    EXPECT_FALSE(rules.setTypedFromString("noSuchRule", std::string(GameRuleClassNames::INTEGER), "1", nullptr));
}

TEST(GameRuleTypedValueTest, IntegerValueMustBeCompleteDecimal)
{
    GameRules rules;
    rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 3, nullptr);

    // std::stoi 会静默接受这些输入，此处必须拒绝且不污染既有值
    for (const char* invalid : {"1.5", "3abc", "", " ", "+3", "0x10"}) {
        EXPECT_FALSE(
            rules.setTypedFromString("randomTickSpeed", std::string(GameRuleClassNames::INTEGER), invalid, nullptr))
            << invalid;
        EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 3) << invalid;
    }

    EXPECT_TRUE(rules.setTypedFromString("randomTickSpeed", std::string(GameRuleClassNames::INTEGER), "0007", nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 7);
}

TEST(GameRuleTypedValueTest, BooleanValueAcceptsCanonicalForms)
{
    GameRules rules;

    for (const char* truthy : {"true", "TRUE", "1"}) {
        EXPECT_TRUE(rules.setTypedFromString("mobGriefing", std::string(GameRuleClassNames::BOOLEAN), truthy, nullptr));
        EXPECT_TRUE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING)) << truthy;
    }

    for (const char* falsy : {"false", "FALSE", "0"}) {
        EXPECT_TRUE(rules.setTypedFromString("mobGriefing", std::string(GameRuleClassNames::BOOLEAN), falsy, nullptr));
        EXPECT_FALSE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING)) << falsy;
    }

    // 非法布尔值不得把既有值改为 false
    rules.setBoolean(GameRuleKeys::MOB_GRIEFING, true, nullptr);
    EXPECT_FALSE(rules.setTypedFromString("mobGriefing", std::string(GameRuleClassNames::BOOLEAN), "yes", nullptr));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING));
}

TEST(GameRuleTypedValueTest, QueryStringIncludesDefaultWhenChanged)
{
    GameRules rules;

    const std::string defaultValue = GameRules::getDefaultValueAsString("randomTickSpeed");
    EXPECT_EQ(defaultValue, "3");

    rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 10, nullptr);
    EXPECT_EQ(rules.getValueAsString("randomTickSpeed"), "10");
    EXPECT_EQ(rules.reset("randomTickSpeed"), true);
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 3);
}

TEST(GameRuleTypedValueTest, RuleClassLookup)
{
    EXPECT_EQ(GameRules::getRuleClass("fire_spread_radius_around_player"),
        std::optional<std::string_view>(GameRuleClassNames::INTEGER));
    EXPECT_EQ(GameRules::getRuleClass("doFireTick"), std::optional<std::string_view>(GameRuleClassNames::BOOLEAN));
    EXPECT_FALSE(GameRules::getRuleClass("noSuchRule").has_value());
}