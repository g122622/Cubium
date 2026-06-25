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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file ItemArgumentTest.cpp
 * @brief ItemArgumentType、ItemInput、ItemPredicateArgumentType、ItemPredicateInput 单元测试
 *
 * 测试物品参数和物品谓词参数的解析功能：
 * - ItemArgumentType：物品ID解析（简单名称、带命名空间）
 * - ItemInput：基本属性和物品查找
 * - ItemPredicateArgumentType：通配符、标签、物品ID三种模式
 * - ItemPredicateInput：匹配测试（test方法）
 */

#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"

using mc::ItemId;
using mc::Items;
using mc::ItemStack;
using mc::ResourceLocation;
using mc::command::CommandErrorType;
using mc::command::CommandException;
using mc::command::ItemArgumentType;
using mc::command::ItemInput;
using mc::command::ItemPredicateArgumentType;
using mc::command::ItemPredicateInput;
using mc::command::StringReader;
using mc::item::tag::ItemTags;

// ========== 测试夹具 ==========

class ItemArgumentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化物品注册表（只执行一次）
        Items::initialize();
        ItemTags::initialize();
    }
};

// ========== ItemInput 测试 ==========

TEST_F(ItemArgumentTest, ItemInput_DefaultConstructor)
{
    ItemInput input;
    EXPECT_EQ(input.itemId(), 0u);
    EXPECT_FALSE(input.isValid());
    EXPECT_EQ(input.getItem(), nullptr);
}

TEST_F(ItemArgumentTest, ItemInput_WithItemId)
{
    // 获取钻石的 ItemId
    const mc::Item* diamond = Items::DIAMOND;
    ASSERT_NE(diamond, nullptr);
    ItemId diamondId = diamond->itemId();

    ItemInput input(diamondId);
    EXPECT_EQ(input.itemId(), diamondId);
    EXPECT_TRUE(input.isValid());
    EXPECT_EQ(input.getItem(), diamond);
}

TEST_F(ItemArgumentTest, ItemInput_CreateStack)
{
    const mc::Item* diamond = Items::DIAMOND;
    ASSERT_NE(diamond, nullptr);
    ItemInput input(diamond->itemId());

    auto stack = input.createStack(10);
    ASSERT_NE(stack, nullptr);
    EXPECT_EQ(stack->getItem(), diamond);
    EXPECT_EQ(stack->getCount(), 10);
}

TEST_F(ItemArgumentTest, ItemInput_InvalidItemIdReturnsNull)
{
    // ItemId 0 表示无效物品
    ItemInput input(0);
    EXPECT_FALSE(input.isValid());
    EXPECT_EQ(input.getItem(), nullptr);
}

// ========== ItemArgumentType 解析测试 ==========

TEST_F(ItemArgumentTest, ParseSimpleItemName)
{
    // "diamond" — 简单物品名，默认命名空间为 minecraft
    ItemArgumentType parser;
    StringReader reader("diamond");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isValid());
    EXPECT_NE(result.getItem(), nullptr);
    EXPECT_EQ(result.getItem()->itemLocation().path(), "diamond");
}

TEST_F(ItemArgumentTest, ParseNamespacedItemName)
{
    // "minecraft:diamond" — 带命名空间
    ItemArgumentType parser;
    StringReader reader("minecraft:diamond");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isValid());
    EXPECT_NE(result.getItem(), nullptr);
    EXPECT_EQ(result.getItem()->itemLocation().namespace_(), "minecraft");
    EXPECT_EQ(result.getItem()->itemLocation().path(), "diamond");
}

TEST_F(ItemArgumentTest, ParseUnknownItemThrows)
{
    // 不存在的物品应抛出异常
    ItemArgumentType parser;
    StringReader reader("minecraft:nonexistent_item_xyz");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(ItemArgumentTest, ParseEmptyInputThrows)
{
    ItemArgumentType parser;
    StringReader reader("");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

// ========== ItemPredicateInput 测试 ==========

TEST_F(ItemArgumentTest, PredicateInput_DefaultConstructorIsAny)
{
    ItemPredicateInput input;
    EXPECT_EQ(input.mode(), ItemPredicateInput::Mode::Any);
    EXPECT_TRUE(input.isAny());
    EXPECT_FALSE(input.isItem());
    EXPECT_FALSE(input.isTag());
}

TEST_F(ItemArgumentTest, PredicateInput_ItemMode)
{
    const mc::Item* diamond = Items::DIAMOND;
    ASSERT_NE(diamond, nullptr);

    ItemPredicateInput input(diamond->itemId());
    EXPECT_EQ(input.mode(), ItemPredicateInput::Mode::Item);
    EXPECT_FALSE(input.isAny());
    EXPECT_TRUE(input.isItem());
    EXPECT_FALSE(input.isTag());
    EXPECT_EQ(input.itemId(), diamond->itemId());
    EXPECT_EQ(input.getItem(), diamond);
}

TEST_F(ItemArgumentTest, PredicateInput_TagMode)
{
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicateInput input(tagId);
    EXPECT_EQ(input.mode(), ItemPredicateInput::Mode::Tag);
    EXPECT_FALSE(input.isAny());
    EXPECT_FALSE(input.isItem());
    EXPECT_TRUE(input.isTag());
    EXPECT_EQ(input.tagId().toString(), "minecraft:flowers");
}

TEST_F(ItemArgumentTest, PredicateInput_DisplayNameAny)
{
    ItemPredicateInput input;
    EXPECT_EQ(input.displayName(), "*");
}

TEST_F(ItemArgumentTest, PredicateInput_DisplayNameItem)
{
    const mc::Item* diamond = Items::DIAMOND;
    ASSERT_NE(diamond, nullptr);

    ItemPredicateInput input(diamond->itemId());
    EXPECT_EQ(input.displayName(), "minecraft:diamond");
}

TEST_F(ItemArgumentTest, PredicateInput_DisplayNameTag)
{
    ResourceLocation tagId("minecraft", "logs");
    ItemPredicateInput input(tagId);
    EXPECT_EQ(input.displayName(), "#minecraft:logs");
}

// ========== ItemPredicateInput::test() 匹配测试 ==========

TEST_F(ItemArgumentTest, PredicateTest_AnyMatchesNonEmpty)
{
    ItemPredicateInput input; // Mode::Any
    ItemStack stack(Items::DIAMOND, 1);
    EXPECT_TRUE(input.test(stack));
}

TEST_F(ItemArgumentTest, PredicateTest_AnyRejectsEmpty)
{
    ItemPredicateInput input; // Mode::Any
    ItemStack emptyStack;
    EXPECT_FALSE(input.test(emptyStack));
}

TEST_F(ItemArgumentTest, PredicateTest_ItemMatchesCorrectItem)
{
    const mc::Item* diamond = Items::DIAMOND;
    ASSERT_NE(diamond, nullptr);

    ItemPredicateInput input(diamond->itemId());
    ItemStack diamondStack(Items::DIAMOND, 1);
    EXPECT_TRUE(input.test(diamondStack));
}

TEST_F(ItemArgumentTest, PredicateTest_ItemRejectsWrongItem)
{
    const mc::Item* diamond = Items::DIAMOND;
    ASSERT_NE(diamond, nullptr);

    ItemPredicateInput input(diamond->itemId());
    ItemStack ironStack(Items::IRON_INGOT, 1);
    EXPECT_FALSE(input.test(ironStack));
}

TEST_F(ItemArgumentTest, PredicateTest_ItemRejectsEmpty)
{
    const mc::Item* diamond = Items::DIAMOND;
    ASSERT_NE(diamond, nullptr);

    ItemPredicateInput input(diamond->itemId());
    ItemStack emptyStack;
    EXPECT_FALSE(input.test(emptyStack));
}

TEST_F(ItemArgumentTest, PredicateTest_TagMatchesItemInTag)
{
    // FLOWERS 标签包含各种花
    ItemTags::FLOWERS(); // 确保标签已初始化
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicateInput input(tagId);

    // 雏菊应该在花朵标签中
    const mc::Item* dandelion = Items::DANDELION;
    if (dandelion != nullptr) {
        ItemStack dandelionStack(dandelion, 1);
        EXPECT_TRUE(input.test(dandelionStack));
    }
}

TEST_F(ItemArgumentTest, PredicateTest_TagRejectsItemNotInTag)
{
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicateInput input(tagId);

    // 钻石不在花朵标签中
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(input.test(diamond));
}

TEST_F(ItemArgumentTest, PredicateTest_TagRejectsEmpty)
{
    ResourceLocation tagId("minecraft", "flowers");
    ItemPredicateInput input(tagId);

    ItemStack emptyStack;
    EXPECT_FALSE(input.test(emptyStack));
}

TEST_F(ItemArgumentTest, PredicateTest_UnknownTagRejectsAll)
{
    // 不存在的标签不匹配任何物品
    ResourceLocation unknownTag("minecraft", "nonexistent_tag_xyz");
    ItemPredicateInput input(unknownTag);

    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(input.test(diamond));
}

// ========== ItemPredicateArgumentType 解析测试 ==========

class ItemPredicateArgumentTypeTest : public ::testing::Test {
protected:
    ItemPredicateArgumentType parser;

    static void SetUpTestSuite()
    {
        Items::initialize();
        ItemTags::initialize();
    }
};

TEST_F(ItemPredicateArgumentTypeTest, ParseWildcard)
{
    // "*" — 通配符，匹配任意物品
    StringReader reader("*");
    auto result = parser.parse(reader);
    EXPECT_EQ(result.mode(), ItemPredicateInput::Mode::Any);
    EXPECT_TRUE(result.isAny());
    EXPECT_EQ(reader.getCursor(), 1); // 消费了 "*"
}

TEST_F(ItemPredicateArgumentTypeTest, ParseTagReference)
{
    // "#minecraft:flowers" — 标签引用
    StringReader reader("#minecraft:flowers");
    auto result = parser.parse(reader);
    EXPECT_EQ(result.mode(), ItemPredicateInput::Mode::Tag);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.tagId().namespace_(), "minecraft");
    EXPECT_EQ(result.tagId().path(), "flowers");
}

TEST_F(ItemPredicateArgumentTypeTest, ParseTagReferenceNoNamespace)
{
    // "#flowers" — 无命名空间的标签，默认为 minecraft
    StringReader reader("#flowers");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.tagId().namespace_(), "minecraft");
    EXPECT_EQ(result.tagId().path(), "flowers");
}

TEST_F(ItemPredicateArgumentTypeTest, ParseTagReferenceCustomNamespace)
{
    // "#mymod:custom_tag" — 自定义命名空间的标签
    StringReader reader("#mymod:custom_tag");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.tagId().namespace_(), "mymod");
    EXPECT_EQ(result.tagId().path(), "custom_tag");
}

TEST_F(ItemPredicateArgumentTypeTest, ParseSpecificItem)
{
    // "diamond" — 特定物品ID
    StringReader reader("diamond");
    auto result = parser.parse(reader);
    EXPECT_EQ(result.mode(), ItemPredicateInput::Mode::Item);
    EXPECT_TRUE(result.isItem());
    EXPECT_NE(result.getItem(), nullptr);
    EXPECT_EQ(result.getItem()->itemLocation().path(), "diamond");
}

TEST_F(ItemPredicateArgumentTypeTest, ParseNamespacedItem)
{
    // "minecraft:diamond" — 带命名空间的物品ID
    StringReader reader("minecraft:diamond");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isItem());
    EXPECT_NE(result.getItem(), nullptr);
    EXPECT_EQ(result.getItem()->itemLocation().namespace_(), "minecraft");
    EXPECT_EQ(result.getItem()->itemLocation().path(), "diamond");
}

TEST_F(ItemPredicateArgumentTypeTest, ParseTagWithTrailingSpace)
{
    // "#minecraft:flowers extra" — 标签引用后有空格
    StringReader reader("#minecraft:flowers extra");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.tagId().path(), "flowers");
    // 应该只消费 "#minecraft:flowers"（18 字符）
    EXPECT_EQ(reader.getCursor(), 18);
}

TEST_F(ItemPredicateArgumentTypeTest, ParseTagHashOnlyThrows)
{
    // "#" 后面没有标识符应抛出异常
    StringReader reader("#");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(ItemPredicateArgumentTypeTest, ParseTagHashSpaceThrows)
{
    // "# " 后面只有空格应抛出异常
    StringReader reader("# ");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(ItemPredicateArgumentTypeTest, ParseUnknownItemThrows)
{
    // 不存在的物品应抛出异常
    StringReader reader("minecraft:nonexistent_item_xyz");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(ItemPredicateArgumentTypeTest, ParseEmptyInputThrows)
{
    StringReader reader("");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

// ========== ItemPredicateArgumentType 类型信息测试 ==========

TEST_F(ItemPredicateArgumentTypeTest, GetTypeName)
{
    EXPECT_EQ(parser.getTypeName(), "item_predicate");
}

TEST_F(ItemPredicateArgumentTypeTest, GetExamples)
{
    auto examples = parser.getExamples();
    EXPECT_EQ(examples.size(), 4u);
    EXPECT_EQ(examples[0], "minecraft:stone");
    EXPECT_EQ(examples[1], "stone");
    EXPECT_EQ(examples[2], "#minecraft:logs");
    EXPECT_EQ(examples[3], "*");
}

TEST_F(ItemPredicateArgumentTypeTest, FactoryMethod)
{
    auto ptr = ItemPredicateArgumentType::itemPredicate();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->getTypeName(), "item_predicate");
}

// ========== ItemPredicateArgumentType 标签贪婪读取测试 ==========

TEST_F(ItemPredicateArgumentTypeTest, TagStopsAtInvalidCharacters)
{
    // "#minecraft:flow!ers" — '!' 不是合法标识符字符
    // "#minecraft:flow" = 15 字符
    StringReader reader("#minecraft:flow!ers");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.tagId().path(), "flow");
    EXPECT_EQ(reader.getCursor(), 15);
}

TEST_F(ItemPredicateArgumentTypeTest, TagWithDigitsAndUnderscores)
{
    // "#minecraft:tag_01" — 下划线和数字是合法的
    StringReader reader("#minecraft:tag_01");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isTag());
    EXPECT_EQ(result.tagId().path(), "tag_01");
}

TEST_F(ItemPredicateArgumentTypeTest, WildcardOnlyMatchesSingleAsterisk)
{
    // "*" 只消费一个字符
    StringReader reader("* extra");
    auto result = parser.parse(reader);
    EXPECT_TRUE(result.isAny());
    EXPECT_EQ(reader.getCursor(), 1);
}
