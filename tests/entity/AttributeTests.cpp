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

#include "common/entity/attribute/Attribute.hpp"
#include "common/entity/attribute/AttributeInstance.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"

using namespace mc::entity::attribute;

// ============================================================================
// Attribute 测试
// ============================================================================

TEST(Attribute, Construction)
{
    Attribute attr("test.attribute", 10.0, 0.0, 100.0);

    EXPECT_EQ(attr.registryName(), "test.attribute");
    EXPECT_DOUBLE_EQ(attr.defaultValue(), 10.0);
    EXPECT_DOUBLE_EQ(attr.minValue(), 0.0);
    EXPECT_DOUBLE_EQ(attr.maxValue(), 100.0);
}

TEST(Attribute, Comparison)
{
    Attribute attr1("test.attribute", 10.0, 0.0, 100.0);
    Attribute attr2("test.attribute", 20.0, 5.0, 50.0);
    Attribute attr3("other.attribute", 10.0, 0.0, 100.0);

    EXPECT_TRUE(attr1 == attr2);  // 相同名称
    EXPECT_FALSE(attr1 == attr3); // 不同名称
}

TEST(Attribute, Clone)
{
    Attribute attr("test.attribute", 10.0, 0.0, 100.0);
    auto clone = attr.clone();

    EXPECT_EQ(clone->registryName(), attr.registryName());
    EXPECT_DOUBLE_EQ(clone->defaultValue(), attr.defaultValue());
    EXPECT_DOUBLE_EQ(clone->minValue(), attr.minValue());
    EXPECT_DOUBLE_EQ(clone->maxValue(), attr.maxValue());
}

// ============================================================================
// AttributeModifier 测试
// ============================================================================

TEST(AttributeModifier, Construction)
{
    AttributeModifier mod("modifier-1", "Test Modifier", 5.0, Operation::Addition);

    EXPECT_EQ(mod.id(), "modifier-1");
    EXPECT_EQ(mod.name(), "Test Modifier");
    EXPECT_DOUBLE_EQ(mod.amount(), 5.0);
    EXPECT_EQ(mod.operation(), Operation::Addition);
}

TEST(AttributeModifier, SetAmount)
{
    AttributeModifier mod("modifier-1", "Test", 5.0, Operation::Addition);

    mod.setAmount(10.0);
    EXPECT_DOUBLE_EQ(mod.amount(), 10.0);
}

TEST(AttributeModifier, Comparison)
{
    AttributeModifier mod1("id-1", "A", 1.0, Operation::Addition);
    AttributeModifier mod2("id-1", "B", 2.0, Operation::MultiplyBase);
    AttributeModifier mod3("id-2", "A", 1.0, Operation::Addition);

    EXPECT_TRUE(mod1 == mod2);  // 相同ID
    EXPECT_FALSE(mod1 == mod3); // 不同ID
}

// ============================================================================
// AttributeInstance 测试
// ============================================================================

TEST(AttributeInstance, Construction)
{
    auto attr = Attributes::maxHealth();
    AttributeInstance instance(*attr);

    EXPECT_DOUBLE_EQ(instance.baseValue(), attr->defaultValue());
    EXPECT_DOUBLE_EQ(instance.getValue(), attr->defaultValue());
}

TEST(AttributeInstance, SetBaseValue)
{
    auto attr = Attributes::maxHealth();
    AttributeInstance instance(*attr);

    instance.setBaseValue(30.0);
    EXPECT_DOUBLE_EQ(instance.baseValue(), 30.0);
    EXPECT_DOUBLE_EQ(instance.getValue(), 30.0);
}

TEST(AttributeInstance, BaseValueClamping)
{
    Attribute attr("test", 50.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    instance.setBaseValue(150.0);
    EXPECT_DOUBLE_EQ(instance.baseValue(), 100.0); // 限制到最大值

    instance.setBaseValue(-10.0);
    EXPECT_DOUBLE_EQ(instance.baseValue(), 0.0); // 限制到最小值
}

TEST(AttributeInstance, AdditionModifier)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    AttributeModifier mod("mod-1", "Add 5", 5.0, Operation::Addition);
    instance.addModifier(mod);

    EXPECT_DOUBLE_EQ(instance.getValue(), 15.0); // 10 + 5
}

TEST(AttributeInstance, MultipleAdditionModifiers)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    instance.addModifier(AttributeModifier("mod-1", "Add 5", 5.0, Operation::Addition));
    instance.addModifier(AttributeModifier("mod-2", "Add 10", 10.0, Operation::Addition));

    EXPECT_DOUBLE_EQ(instance.getValue(), 25.0); // 10 + 5 + 10
}

TEST(AttributeInstance, MultiplyBaseModifier)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    AttributeModifier mod("mod-1", "Multiply Base", 0.5, Operation::MultiplyBase);
    instance.addModifier(mod);

    EXPECT_DOUBLE_EQ(instance.getValue(), 15.0); // 10 + (10 * 0.5) = 15
}

TEST(AttributeInstance, MultiplyTotalModifier)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    AttributeModifier mod("mod-1", "Multiply Total", 0.5, Operation::MultiplyTotal);
    instance.addModifier(mod);

    EXPECT_DOUBLE_EQ(instance.getValue(), 15.0); // 10 * 1.5 = 15
}

TEST(AttributeInstance, MixedModifiers)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    // Addition: 10 + 5 = 15
    instance.addModifier(AttributeModifier("mod-1", "Add 5", 5.0, Operation::Addition));

    // MultiplyBase: 15 + (10 * 0.5) = 20
    instance.addModifier(AttributeModifier("mod-2", "Mult Base", 0.5, Operation::MultiplyBase));

    // MultiplyTotal: 20 * 1.1 = 22
    instance.addModifier(AttributeModifier("mod-3", "Mult Total", 0.1, Operation::MultiplyTotal));

    EXPECT_DOUBLE_EQ(instance.getValue(), 22.0);
}

TEST(AttributeInstance, RemoveModifier)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    instance.addModifier(AttributeModifier("mod-1", "Add 5", 5.0, Operation::Addition));
    EXPECT_DOUBLE_EQ(instance.getValue(), 15.0);

    EXPECT_TRUE(instance.removeModifier("mod-1"));
    EXPECT_DOUBLE_EQ(instance.getValue(), 10.0);

    EXPECT_FALSE(instance.removeModifier("non-existent"));
}

TEST(AttributeInstance, ClearModifiers)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    instance.addModifier(AttributeModifier("mod-1", "Add 5", 5.0, Operation::Addition));
    instance.addModifier(AttributeModifier("mod-2", "Add 10", 10.0, Operation::Addition));

    EXPECT_DOUBLE_EQ(instance.getValue(), 25.0);

    instance.clearModifiers();
    EXPECT_DOUBLE_EQ(instance.getValue(), 10.0);
}

TEST(AttributeInstance, HasModifier)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    instance.addModifier(AttributeModifier("mod-1", "Test", 5.0, Operation::Addition));

    EXPECT_TRUE(instance.hasModifier("mod-1"));
    EXPECT_FALSE(instance.hasModifier("mod-2"));
}

TEST(AttributeInstance, GetModifier)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    AttributeModifier mod("mod-1", "Test Modifier", 5.0, Operation::Addition);
    instance.addModifier(mod);

    const AttributeModifier* found = instance.getModifier("mod-1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name(), "Test Modifier");
    EXPECT_DOUBLE_EQ(found->amount(), 5.0);

    EXPECT_EQ(instance.getModifier("non-existent"), nullptr);
}

TEST(AttributeInstance, DirtyFlag)
{
    Attribute attr("test", 10.0, 0.0, 100.0);
    AttributeInstance instance(attr);

    EXPECT_TRUE(instance.isDirty());

    instance.getValue(); // 计算缓存值
    EXPECT_FALSE(instance.isDirty());

    instance.setBaseValue(20.0);
    EXPECT_TRUE(instance.isDirty());

    instance.getValue();
    EXPECT_FALSE(instance.isDirty());

    instance.addModifier(AttributeModifier("mod-1", "Test", 5.0, Operation::Addition));
    EXPECT_TRUE(instance.isDirty());

    instance.markSynced();
    EXPECT_FALSE(instance.isDirty());
}

// ============================================================================
// AttributeMap 测试
// ============================================================================

TEST(AttributeMap, RegisterAttribute)
{
    AttributeMap map;

    auto attr = Attributes::maxHealth();
    EXPECT_TRUE(map.registerAttribute(*attr));
    EXPECT_FALSE(map.registerAttribute(*attr)); // 重复注册
}

TEST(AttributeMap, GetValue)
{
    AttributeMap map;

    map.registerAttribute(*Attributes::maxHealth());

    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 20.0);
    EXPECT_DOUBLE_EQ(map.getValue("non-existent", 99.0), 99.0);
}

TEST(AttributeMap, SetBaseValue)
{
    AttributeMap map;

    map.registerAttribute(*Attributes::maxHealth());
    map.setBaseValue(Attributes::MAX_HEALTH, 30.0);

    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 30.0);
}

TEST(AttributeMap, AddModifier)
{
    AttributeMap map;

    map.registerAttribute(*Attributes::maxHealth());
    map.addModifier(Attributes::MAX_HEALTH, AttributeModifier("mod-1", "Add 10", 10.0, Operation::Addition));

    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 30.0); // 20 + 10
}

TEST(AttributeMap, RemoveModifier)
{
    AttributeMap map;

    map.registerAttribute(*Attributes::maxHealth());
    map.addModifier(Attributes::MAX_HEALTH, AttributeModifier("mod-1", "Add 10", 10.0, Operation::Addition));

    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 30.0);

    EXPECT_TRUE(map.removeModifier(Attributes::MAX_HEALTH, "mod-1"));
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 20.0);
}

TEST(AttributeMap, HasAttribute)
{
    AttributeMap map;

    EXPECT_FALSE(map.hasAttribute(Attributes::MAX_HEALTH));

    map.registerAttribute(*Attributes::maxHealth());

    EXPECT_TRUE(map.hasAttribute(Attributes::MAX_HEALTH));
}

TEST(AttributeMap, MultipleAttributes)
{
    AttributeMap map;

    map.registerAttribute(*Attributes::maxHealth());
    map.registerAttribute(*Attributes::movementSpeed());
    map.registerAttribute(*Attributes::attackDamage());

    map.setBaseValue(Attributes::MAX_HEALTH, 40.0);
    map.setBaseValue(Attributes::MOVEMENT_SPEED, 0.15);

    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MAX_HEALTH), 40.0);
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::MOVEMENT_SPEED), 0.15);
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::ATTACK_DAMAGE), 2.0);
}

TEST(AttributeMap, CopyFrom)
{
    AttributeMap map1;
    AttributeMap map2;

    map1.registerAttribute(*Attributes::maxHealth());
    map1.setBaseValue(Attributes::MAX_HEALTH, 30.0);
    map1.addModifier(Attributes::MAX_HEALTH, AttributeModifier("mod-1", "Add 10", 10.0, Operation::Addition));

    map2.registerAttribute(*Attributes::maxHealth());
    map2.copyFrom(map1);

    EXPECT_DOUBLE_EQ(map2.getValue(Attributes::MAX_HEALTH), 40.0);
}

// ============================================================================
// Standard Attributes 测试
// ============================================================================

TEST(Attributes, MaxHealth)
{
    auto attr = Attributes::maxHealth();

    EXPECT_EQ(attr->registryName(), Attributes::MAX_HEALTH);
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 20.0);
    // MC 1.16.5: max_health 最小值为 1.0（不能为 0）
    EXPECT_DOUBLE_EQ(attr->minValue(), 1.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 1024.0);
}

TEST(Attributes, MovementSpeed)
{
    auto attr = Attributes::movementSpeed();

    EXPECT_EQ(attr->registryName(), Attributes::MOVEMENT_SPEED);
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 0.7);
}

TEST(Attributes, AttackDamage)
{
    auto attr = Attributes::attackDamage();

    EXPECT_EQ(attr->registryName(), Attributes::ATTACK_DAMAGE);
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 2.0);
}

TEST(Attributes, KnockbackResistance)
{
    auto attr = Attributes::knockbackResistance();

    EXPECT_EQ(attr->registryName(), Attributes::KNOCKBACK_RESISTANCE);
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 0.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 1.0); // 最大值为1（完全免疫）
}

TEST(Attributes, Luck)
{
    // MC 1.16.5: generic.luck 属性
    auto attr = Attributes::luck();

    EXPECT_EQ(attr->registryName(), Attributes::LUCK);
    EXPECT_DOUBLE_EQ(attr->defaultValue(), 0.0);
    EXPECT_DOUBLE_EQ(attr->minValue(), -1024.0);
    EXPECT_DOUBLE_EQ(attr->maxValue(), 1024.0);
}

TEST(AttributeMap, LuckAttribute)
{
    // 测试 LUCK 属性注册和值获取
    AttributeMap map;

    map.registerAttribute(*Attributes::luck());

    // 默认值应为 0.0
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::LUCK), 0.0);

    // 设置基础值
    map.setBaseValue(Attributes::LUCK, 5.0);
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::LUCK), 5.0);

    // 负值也有效
    map.setBaseValue(Attributes::LUCK, -3.0);
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::LUCK), -3.0);

    // 添加修改器（模拟幸运药水效果）
    map.addModifier(Attributes::LUCK, AttributeModifier("luck-potion", "Luck", 1.0, Operation::Addition));
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::LUCK), -2.0); // -3 + 1 = -2
}

TEST(AttributeMap, LuckDefaultValue)
{
    // 测试未注册属性返回默认值
    AttributeMap map;

    // 未注册 LUCK 属性时，应返回默认值
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::LUCK, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(map.getValue(Attributes::LUCK, 10.0), 10.0); // 使用提供的默认值
}
