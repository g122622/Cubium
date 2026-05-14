/**
 * @file GameRulesTest.cpp
 * @brief GameRules 系统单元测试
 */

#include "common/world/gamerule/GameRules.hpp"
#include "common/world/gamerule/GameRule.hpp"
#include <gtest/gtest.h>

using namespace mc::world::gamerule;

// ============================================================================
// GameRuleKey Tests
// ============================================================================

TEST(GameRuleKeyTest, Construction)
{
    BooleanGameRuleKey key("mobGriefing", GameRuleCategory::Mobs);
    EXPECT_EQ(key.getName(), "mobGriefing");
    EXPECT_EQ(key.getCategory(), GameRuleCategory::Mobs);
    EXPECT_EQ(key.getTranslationKey(), "gamerule.mobGriefing");
}

TEST(GameRuleKeyTest, Equality)
{
    BooleanGameRuleKey key1("testRule", GameRuleCategory::Misc);
    BooleanGameRuleKey key2("testRule", GameRuleCategory::Player);
    BooleanGameRuleKey key3("otherRule", GameRuleCategory::Misc);

    EXPECT_TRUE(key1 == key2);  // Same name = equal
    EXPECT_FALSE(key1 == key3); // Different name = not equal
}

TEST(GameRuleKeyTest, Hash)
{
    BooleanGameRuleKey key1("testRule", GameRuleCategory::Misc);
    BooleanGameRuleKey key2("testRule", GameRuleCategory::Player);

    EXPECT_EQ(key1.hashCode(), key2.hashCode()); // Same name = same hash
}

// ============================================================================
// GameRuleValue Tests
// ============================================================================

TEST(GameRuleValueTest, BooleanDefaultValue)
{
    BooleanGameRuleType type(true);
    BooleanGameRuleValue value(type);

    EXPECT_EQ(value.get(), true);
    EXPECT_TRUE(value.isDefault());
}

TEST(GameRuleValueTest, BooleanSetAndGet)
{
    BooleanGameRuleType type(true);
    BooleanGameRuleValue value(type);

    value.set(false, nullptr);
    EXPECT_EQ(value.get(), false);
    EXPECT_FALSE(value.isDefault());

    value.set(true, nullptr);
    EXPECT_EQ(value.get(), true);
    EXPECT_TRUE(value.isDefault());
}

TEST(GameRuleValueTest, BooleanReset)
{
    BooleanGameRuleType type(false);
    BooleanGameRuleValue value(type);

    value.set(true, nullptr);
    EXPECT_FALSE(value.isDefault());

    value.reset(nullptr);
    EXPECT_EQ(value.get(), false);
    EXPECT_TRUE(value.isDefault());
}

TEST(GameRuleValueTest, BooleanToString)
{
    BooleanGameRuleType type(true);
    BooleanGameRuleValue value(type);

    EXPECT_EQ(value.toString(), "true");

    value.set(false, nullptr);
    EXPECT_EQ(value.toString(), "false");
}

TEST(GameRuleValueTest, BooleanFromString)
{
    BooleanGameRuleType type(false);
    BooleanGameRuleValue value(type);

    EXPECT_TRUE(value.fromString("true"));
    EXPECT_EQ(value.get(), true);

    EXPECT_TRUE(value.fromString("false"));
    EXPECT_EQ(value.get(), false);

    EXPECT_TRUE(value.fromString("TRUE"));
    EXPECT_EQ(value.get(), true);

    EXPECT_TRUE(value.fromString("1"));
    EXPECT_EQ(value.get(), true);

    EXPECT_TRUE(value.fromString("0"));
    EXPECT_EQ(value.get(), false);
}

TEST(GameRuleValueTest, IntegerDefaultValue)
{
    IntegerGameRuleType type(10);
    IntegerGameRuleValue value(type);

    EXPECT_EQ(value.get(), 10);
    EXPECT_TRUE(value.isDefault());
}

TEST(GameRuleValueTest, IntegerSetAndGet)
{
    IntegerGameRuleType type(0);
    IntegerGameRuleValue value(type);

    value.set(100, nullptr);
    EXPECT_EQ(value.get(), 100);
    EXPECT_FALSE(value.isDefault());

    value.set(0, nullptr);
    EXPECT_EQ(value.get(), 0);
    EXPECT_TRUE(value.isDefault());
}

TEST(GameRuleValueTest, IntegerToString)
{
    IntegerGameRuleType type(0);
    IntegerGameRuleValue value(type);

    value.set(42, nullptr);
    EXPECT_EQ(value.toString(), "42");

    value.set(-10, nullptr);
    EXPECT_EQ(value.toString(), "-10");
}

TEST(GameRuleValueTest, IntegerFromString)
{
    IntegerGameRuleType type(0);
    IntegerGameRuleValue value(type);

    EXPECT_TRUE(value.fromString("100"));
    EXPECT_EQ(value.get(), 100);

    EXPECT_TRUE(value.fromString("-50"));
    EXPECT_EQ(value.get(), -50);

    EXPECT_FALSE(value.fromString("not_a_number"));
}

TEST(GameRuleValueTest, Clone)
{
    IntegerGameRuleType type(10);
    IntegerGameRuleValue original(type);
    original.set(50, nullptr);

    IntegerGameRuleValue copy = original.clone();
    EXPECT_EQ(copy.get(), 50);
    EXPECT_EQ(copy.getType(), &type);

    // Changing copy shouldn't affect original
    copy.set(99, nullptr);
    EXPECT_EQ(original.get(), 50);
    EXPECT_EQ(copy.get(), 99);
}

// ============================================================================
// GameRules Tests
// ============================================================================

TEST(GameRulesTest, HasKnownRule)
{
    GameRules rules;

    EXPECT_TRUE(GameRules::hasRule("mobGriefing"));
    EXPECT_TRUE(GameRules::hasRule("naturalRegeneration"));
    EXPECT_TRUE(GameRules::hasRule("randomTickSpeed"));
    EXPECT_TRUE(GameRules::hasRule("doDaylightCycle"));
}

TEST(GameRulesTest, HasUnknownRule)
{
    EXPECT_FALSE(GameRules::hasRule("nonExistentRule"));
    EXPECT_FALSE(GameRules::hasRule(""));
}

TEST(GameRulesTest, GetRuleType)
{
    EXPECT_EQ(GameRules::getRuleType("mobGriefing"), GameRuleValueType::Boolean);
    EXPECT_EQ(GameRules::getRuleType("naturalRegeneration"), GameRuleValueType::Boolean);
    EXPECT_EQ(GameRules::getRuleType("randomTickSpeed"), GameRuleValueType::Integer);
    EXPECT_EQ(GameRules::getRuleType("maxEntityCramming"), GameRuleValueType::Integer);
}

TEST(GameRulesTest, GetRuleNames)
{
    auto names = GameRules::getRuleNames();

    // Should have many rules
    EXPECT_GT(names.size(), 20u);

    // Should contain known rules
    bool hasMobGriefing = false;
    bool hasRandomTickSpeed = false;
    for (const auto& name : names) {
        if (name == "mobGriefing") hasMobGriefing = true;
        if (name == "randomTickSpeed") hasRandomTickSpeed = true;
    }
    EXPECT_TRUE(hasMobGriefing);
    EXPECT_TRUE(hasRandomTickSpeed);
}

TEST(GameRulesTest, BooleanRuleDefaultValues)
{
    GameRules rules;

    // Test known default values (from MC 1.16.5)
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::NATURAL_REGENERATION));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_DAYLIGHT_CYCLE));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_WEATHER_CYCLE));
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::KEEP_INVENTORY)); // Default false in 1.16.5
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_MOB_SPAWNING)); // Default true in 1.16.5
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_TILE_DROPS));   // Default true in 1.16.5
}

TEST(GameRulesTest, IntegerRuleDefaultValues)
{
    GameRules rules;

    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 3);
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_ENTITY_CRAMMING), 24);
    EXPECT_EQ(rules.getInt(GameRuleKeys::SPAWN_RADIUS), 10);
    EXPECT_EQ(rules.getInt(GameRuleKeys::MAX_COMMAND_CHAIN_LENGTH), 65536);
}

TEST(GameRulesTest, SetBooleanRule)
{
    GameRules rules;

    // Change value
    rules.setBoolean(GameRuleKeys::MOB_GRIEFING, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING));

    // Change back
    rules.setBoolean(GameRuleKeys::MOB_GRIEFING, true, nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING));
}

TEST(GameRulesTest, SetIntegerRule)
{
    GameRules rules;

    rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 100, nullptr);
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 100);

    rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 0, nullptr);
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 0);

    rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, -1, nullptr); // Negative is valid
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), -1);
}

TEST(GameRulesTest, SetFromString)
{
    GameRules rules;

    // Boolean from string
    EXPECT_TRUE(rules.setFromString("mobGriefing", "false", nullptr));
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING));

    EXPECT_TRUE(rules.setFromString("mobGriefing", "true", nullptr));
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::MOB_GRIEFING));

    // Integer from string
    EXPECT_TRUE(rules.setFromString("randomTickSpeed", "50", nullptr));
    EXPECT_EQ(rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 50);

    // Invalid boolean value - only "true"/"false"/"1"/"0" are valid
    EXPECT_FALSE(rules.setFromString("mobGriefing", "yes", nullptr)); // "yes" is not valid

    // Invalid integer value
    EXPECT_FALSE(rules.setFromString("randomTickSpeed", "not_a_number", nullptr));

    // Unknown rule
    EXPECT_FALSE(rules.setFromString("unknownRule", "value", nullptr));
}

TEST(GameRulesTest, WriteAndReadNBT)
{
    GameRules original;

    // Set some custom values
    original.setBoolean(GameRuleKeys::MOB_GRIEFING, false, nullptr);
    original.setBoolean(GameRuleKeys::KEEP_INVENTORY, true, nullptr);
    original.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 100, nullptr);
    original.setInt(GameRuleKeys::SPAWN_RADIUS, 5, nullptr);

    // Write to NBT
    auto nbt = original.write();
    ASSERT_NE(nbt, nullptr);

    // Read into new GameRules
    GameRules loaded;
    loaded.read(*nbt);

    // Verify values
    EXPECT_FALSE(loaded.getBoolean(GameRuleKeys::MOB_GRIEFING));
    EXPECT_TRUE(loaded.getBoolean(GameRuleKeys::KEEP_INVENTORY));
    EXPECT_EQ(loaded.getInt(GameRuleKeys::RANDOM_TICK_SPEED), 100);
    EXPECT_EQ(loaded.getInt(GameRuleKeys::SPAWN_RADIUS), 5);

    // Default values should be preserved
    EXPECT_TRUE(loaded.getBoolean(GameRuleKeys::NATURAL_REGENERATION));
    EXPECT_EQ(loaded.getInt(GameRuleKeys::MAX_ENTITY_CRAMMING), 24);
}

TEST(GameRulesTest, WriteOnlyNonDefaultValues)
{
    GameRules rules;

    // Don't change anything - NBT should be minimal
    auto nbt = rules.write();
    ASSERT_NE(nbt, nullptr);

    // Default GameRules should write empty or minimal NBT
    // (implementation detail - just ensure it doesn't crash)
}

TEST(GameRulesTest, Categories)
{
    // Verify that all keys have correct categories
    EXPECT_EQ(GameRuleKeys::MOB_GRIEFING.getCategory(), GameRuleCategory::Mobs);
    EXPECT_EQ(GameRuleKeys::NATURAL_REGENERATION.getCategory(), GameRuleCategory::Player);
    EXPECT_EQ(GameRuleKeys::RANDOM_TICK_SPEED.getCategory(), GameRuleCategory::Updates);
    EXPECT_EQ(GameRuleKeys::COMMAND_BLOCK_OUTPUT.getCategory(), GameRuleCategory::Chat);
}

// ============================================================================
// GameRuleType Tests
// ============================================================================

TEST(GameRuleTypeTest, CreateValue)
{
    BooleanGameRuleType boolType(true);
    auto boolValue = boolType.createValue();
    EXPECT_EQ(boolValue.get(), true);

    IntegerGameRuleType intType(42);
    auto intValue = intType.createValue();
    EXPECT_EQ(intValue.get(), 42);
}

TEST(GameRuleTypeTest, GetDefaultValue)
{
    BooleanGameRuleType boolType(false);
    EXPECT_EQ(boolType.getDefaultValue(), false);

    IntegerGameRuleType intType(100);
    EXPECT_EQ(intType.getDefaultValue(), 100);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(GameRulesIntegrationTest, AllRulesAreAccessible)
{
    GameRules rules;
    auto names = GameRules::getRuleNames();

    // Every rule name should be queryable
    for (const auto& name : names) {
        EXPECT_TRUE(GameRules::hasRule(name)) << "Rule " << name << " should exist";
        EXPECT_NE(GameRules::getRuleType(name), std::nullopt) << "Rule " << name << " should have a type";
    }
}

TEST(GameRulesIntegrationTest, AllKeysWorkWithGameRules)
{
    GameRules rules;

    // All predefined keys should work
    rules.getBoolean(GameRuleKeys::MOB_GRIEFING);
    rules.getBoolean(GameRuleKeys::NATURAL_REGENERATION);
    rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED);
    rules.getInt(GameRuleKeys::MAX_ENTITY_CRAMMING);
    // No exceptions should be thrown
}
