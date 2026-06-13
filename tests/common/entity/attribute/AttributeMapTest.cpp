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

#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attribute.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::attribute;

// ============================================================================
// AttributeMap::resetBaseValue 测试
// ============================================================================

TEST(AttributeMapResetBaseValueTest, ResetExistingAttribute)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::maxHealth());

    // 修改基础值
    map.setBaseValue(Attributes::MAX_HEALTH, 30.0);
    EXPECT_DOUBLE_EQ(map.getBaseValue(Attributes::MAX_HEALTH), 30.0);

    // 重置为默认值（maxHealth 默认为 20.0）
    bool result = map.resetBaseValue(Attributes::MAX_HEALTH);
    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(map.getBaseValue(Attributes::MAX_HEALTH), 20.0);
}

TEST(AttributeMapResetBaseValueTest, ResetNonExistentAttributeReturnsFalse)
{
    AttributeMap map;
    // 不存在的属性
    bool result = map.resetBaseValue("generic.nonexistent");
    EXPECT_FALSE(result);
}

TEST(AttributeMapResetBaseValueTest, ResetPreservesModifiers)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::movementSpeed());
    map.setBaseValue(Attributes::MOVEMENT_SPEED, 0.2);

    // 添加修饰符
    AttributeModifier modifier("test_modifier", "Test", 0.1, Operation::Addition);
    map.addModifier(Attributes::MOVEMENT_SPEED, modifier);

    // 重置基础值
    bool result = map.resetBaseValue(Attributes::MOVEMENT_SPEED);
    EXPECT_TRUE(result);

    // 基础值应恢复默认（0.7），但修饰符仍应存在
    EXPECT_DOUBLE_EQ(map.getBaseValue(Attributes::MOVEMENT_SPEED), 0.7);
    EXPECT_TRUE(map.hasModifier(Attributes::MOVEMENT_SPEED, "test_modifier"));

    // 最终值应包含修饰符的效果
    f64 value = map.getValue(Attributes::MOVEMENT_SPEED);
    EXPECT_DOUBLE_EQ(value, 0.8); // 0.7 + 0.1
}

// ============================================================================
// AttributeMap::hasModifier 测试
// ============================================================================

TEST(AttributeMapHasModifierTest, HasModifierReturnsTrueForExistingModifier)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::maxHealth());
    AttributeModifier modifier("mod1", "Modifier1", 5.0, Operation::Addition);
    map.addModifier(Attributes::MAX_HEALTH, modifier);

    EXPECT_TRUE(map.hasModifier(Attributes::MAX_HEALTH, "mod1"));
}

TEST(AttributeMapHasModifierTest, HasModifierReturnsFalseForNonExistentModifier)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::maxHealth());

    EXPECT_FALSE(map.hasModifier(Attributes::MAX_HEALTH, "nonexistent"));
}

TEST(AttributeMapHasModifierTest, HasModifierReturnsFalseForNonExistentAttribute)
{
    AttributeMap map;

    EXPECT_FALSE(map.hasModifier("generic.nonexistent", "mod1"));
}

// ============================================================================
// AttributeMap::getModifierValue 测试
// ============================================================================

TEST(AttributeMapGetModifierValueTest, GetModifierValueReturnsCorrectAmount)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::attackDamage());
    AttributeModifier modifier("atk_mod", "Attack Boost", 3.5, Operation::Addition);
    map.addModifier(Attributes::ATTACK_DAMAGE, modifier);

    f64 value = map.getModifierValue(Attributes::ATTACK_DAMAGE, "atk_mod");
    EXPECT_DOUBLE_EQ(value, 3.5);
}

TEST(AttributeMapGetModifierValueTest, GetModifierValueReturnsDefaultForNonExistent)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::attackDamage());

    f64 value = map.getModifierValue(Attributes::ATTACK_DAMAGE, "nonexistent", -1.0);
    EXPECT_DOUBLE_EQ(value, -1.0);
}

TEST(AttributeMapGetModifierValueTest, GetModifierValueReturnsDefaultForNonExistentAttribute)
{
    AttributeMap map;

    f64 value = map.getModifierValue("generic.nonexistent", "mod1", 42.0);
    EXPECT_DOUBLE_EQ(value, 42.0);
}

TEST(AttributeMapGetModifierValueTest, GetModifierValueDefaultParameterIsZero)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::attackDamage());

    f64 value = map.getModifierValue(Attributes::ATTACK_DAMAGE, "nonexistent");
    EXPECT_DOUBLE_EQ(value, 0.0);
}

// ============================================================================
// AttributeMap 综合测试
// ============================================================================

TEST(AttributeMapComprehensiveTest, AddAndRemoveModifierViaMap)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::maxHealth());

    // 初始基础值为默认值
    EXPECT_DOUBLE_EQ(map.getBaseValue(Attributes::MAX_HEALTH), 20.0);

    // 添加修饰符
    AttributeModifier mod1("mod1", "Health Boost", 10.0, Operation::Addition);
    map.addModifier(Attributes::MAX_HEALTH, mod1);
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 30.0); // 20 + 10

    // 添加第二个修饰符
    AttributeModifier mod2("mod2", "Health Multiplier", 0.5, Operation::MultiplyBase);
    map.addModifier(Attributes::MAX_HEALTH, mod2);
    // 20 + 10 + 20*0.5 = 40.0
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 40.0);

    // 移除第一个修饰符
    bool removed = map.removeModifier(Attributes::MAX_HEALTH, "mod1");
    EXPECT_TRUE(removed);
    // 20 + 20*0.5 = 30.0
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 30.0);

    // 确认修饰符已移除
    EXPECT_FALSE(map.hasModifier(Attributes::MAX_HEALTH, "mod1"));
    EXPECT_TRUE(map.hasModifier(Attributes::MAX_HEALTH, "mod2"));
}

TEST(AttributeMapComprehensiveTest, MultiplyTotalOperation)
{
    AttributeMap map;
    map.registerAttribute(*Attributes::movementSpeed());

    AttributeModifier addition("add1", "Speed Boost", 0.1, Operation::Addition);
    AttributeModifier multiply("mul1", "Speed Multiplier", 0.5, Operation::MultiplyTotal);
    map.addModifier(Attributes::MOVEMENT_SPEED, addition);
    map.addModifier(Attributes::MOVEMENT_SPEED, multiply);

    // 基础 0.7 + 0.1 = 0.8, 然后 0.8 * (1 + 0.5) = 1.2
    f64 value = map.getValue(Attributes::MOVEMENT_SPEED);
    EXPECT_DOUBLE_EQ(value, 1.2);
}
