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

#include "common/util/nbt/Nbt.hpp"

using namespace mc;
using namespace mc::nbt;
using namespace mc::nbt::tags;

// ========== 基本类型标签 equals 测试 ==========

TEST(NbtEqualsTest, EndTagEquals)
{
    end_tag a;
    end_tag b;
    EXPECT_TRUE(a.equals(b));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);

    // 与不同类型比较
    byte_tag c(0);
    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, ByteTagEquals)
{
    byte_tag a(static_cast<i8>(42));
    byte_tag b(static_cast<i8>(42));
    byte_tag c(static_cast<i8>(100));

    EXPECT_TRUE(a.equals(b));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);

    EXPECT_FALSE(a.equals(c));
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);

    // 不同类型比较
    short_tag d(static_cast<i16>(42));
    EXPECT_FALSE(a.equals(d));
}

TEST(NbtEqualsTest, ShortTagEquals)
{
    short_tag a(static_cast<i16>(1000));
    short_tag b(static_cast<i16>(1000));
    short_tag c(static_cast<i16>(2000));

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, IntTagEquals)
{
    int_tag a(static_cast<i32>(12345));
    int_tag b(static_cast<i32>(12345));
    int_tag c(static_cast<i32>(99999));

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, LongTagEquals)
{
    long_tag a(static_cast<i64>(9876543210LL));
    long_tag b(static_cast<i64>(9876543210LL));
    long_tag c(static_cast<i64>(0LL));

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, FloatTagEquals)
{
    float_tag a(3.14f);
    float_tag b(3.14f);
    float_tag c(2.71f);

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));

    // NaN 不等于自身
    float_tag nanTag(std::numeric_limits<float>::quiet_NaN());
    EXPECT_FALSE(nanTag.equals(nanTag));
}

TEST(NbtEqualsTest, DoubleTagEquals)
{
    double_tag a(3.141592653589793);
    double_tag b(3.141592653589793);
    double_tag c(2.71828);

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));

    // NaN 不等于自身
    double_tag nanTag(std::numeric_limits<double>::quiet_NaN());
    EXPECT_FALSE(nanTag.equals(nanTag));
}

TEST(NbtEqualsTest, StringTagEquals)
{
    string_tag a(std::string("hello"));
    string_tag b(std::string("hello"));
    string_tag c(std::string("world"));

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));

    // 空字符串
    string_tag empty1(std::string(""));
    string_tag empty2(std::string(""));
    EXPECT_TRUE(empty1.equals(empty2));
}

TEST(NbtEqualsTest, ByteArrayTagEquals)
{
    bytearray_tag a(std::vector<i8>{1, 2, 3, 4, 5});
    bytearray_tag b(std::vector<i8>{1, 2, 3, 4, 5});
    bytearray_tag c(std::vector<i8>{1, 2, 3, 4, 6});
    bytearray_tag d(std::vector<i8>{1, 2, 3});

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
    EXPECT_FALSE(a.equals(d));
}

TEST(NbtEqualsTest, IntArrayTagEquals)
{
    intarray_tag a(std::vector<i32>{10, 20, 30});
    intarray_tag b(std::vector<i32>{10, 20, 30});
    intarray_tag c(std::vector<i32>{10, 20, 40});

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, LongArrayTagEquals)
{
    longarray_tag a(std::vector<i64>{100LL, 200LL, 300LL});
    longarray_tag b(std::vector<i64>{100LL, 200LL, 300LL});
    longarray_tag c(std::vector<i64>{100LL, 200LL, 999LL});

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

// ========== 列表标签 equals 测试 ==========

TEST(NbtEqualsTest, NumericListTagEquals)
{
    int_list_tag a(std::vector<i32>{1, 2, 3});
    int_list_tag b(std::vector<i32>{1, 2, 3});
    int_list_tag c(std::vector<i32>{1, 2, 4});
    int_list_tag d(std::vector<i32>{1, 2});

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
    EXPECT_FALSE(a.equals(d));
}

TEST(NbtEqualsTest, StringListTagEquals)
{
    string_list_tag a(std::vector<std::string>{"foo", "bar"});
    string_list_tag b(std::vector<std::string>{"foo", "bar"});
    string_list_tag c(std::vector<std::string>{"foo", "baz"});

    EXPECT_TRUE(a.equals(b));
    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, ListTagDifferentElementTypes)
{
    int_list_tag intList(std::vector<i32>{1, 2});
    string_list_tag strList(std::vector<std::string>{"1", "2"});

    // 不同元素类型的列表不相等
    EXPECT_FALSE(intList.equals(strList));
}

TEST(NbtEqualsTest, TagListTagEquals)
{
    tag_list_tag listA(TagId::Int);
    listA.value.push_back(std::make_unique<int_tag>(1));
    listA.value.push_back(std::make_unique<int_tag>(2));

    tag_list_tag listB(TagId::Int);
    listB.value.push_back(std::make_unique<int_tag>(1));
    listB.value.push_back(std::make_unique<int_tag>(2));

    tag_list_tag listC(TagId::Int);
    listC.value.push_back(std::make_unique<int_tag>(1));
    listC.value.push_back(std::make_unique<int_tag>(3));

    EXPECT_TRUE(listA.equals(listB));
    EXPECT_FALSE(listA.equals(listC));
}

TEST(NbtEqualsTest, CompoundListTagEquals)
{
    compound_list_tag listA;
    {
        compound_tag elem;
        elem.put("x", static_cast<i32>(1));
        listA.value.push_back(elem);
    }
    compound_list_tag listB;
    {
        compound_tag elem;
        elem.put("x", static_cast<i32>(1));
        listB.value.push_back(elem);
    }
    compound_list_tag listC;
    {
        compound_tag elem;
        elem.put("x", static_cast<i32>(2));
        listC.value.push_back(elem);
    }

    EXPECT_TRUE(listA.equals(listB));
    EXPECT_FALSE(listA.equals(listC));
}

// ========== 复合标签 equals 测试 ==========

TEST(NbtEqualsTest, CompoundTagEmptyEquals)
{
    compound_tag a;
    compound_tag b;

    EXPECT_TRUE(a.equals(b));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(NbtEqualsTest, CompoundTagBasicEquals)
{
    compound_tag a;
    a.put("name", std::string("Test"));
    a.put("value", static_cast<i32>(42));
    a.put("active", static_cast<i8>(1));

    compound_tag b;
    b.put("name", std::string("Test"));
    b.put("value", static_cast<i32>(42));
    b.put("active", static_cast<i8>(1));

    EXPECT_TRUE(a.equals(b));
    EXPECT_TRUE(a == b);
}

TEST(NbtEqualsTest, CompoundTagDifferentValues)
{
    compound_tag a;
    a.put("name", std::string("Test"));
    a.put("value", static_cast<i32>(42));

    compound_tag b;
    b.put("name", std::string("Test"));
    b.put("value", static_cast<i32>(100));

    EXPECT_FALSE(a.equals(b));
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST(NbtEqualsTest, CompoundTagMissingKey)
{
    compound_tag a;
    a.put("name", std::string("Test"));
    a.put("value", static_cast<i32>(42));

    compound_tag b;
    b.put("name", std::string("Test"));

    EXPECT_FALSE(a.equals(b));
}

TEST(NbtEqualsTest, CompoundTagExtraKey)
{
    compound_tag a;
    a.put("name", std::string("Test"));

    compound_tag b;
    b.put("name", std::string("Test"));
    b.put("extra", static_cast<i32>(1));

    EXPECT_FALSE(a.equals(b));
}

TEST(NbtEqualsTest, CompoundTagDifferentTypes)
{
    compound_tag a;
    a.put("value", static_cast<i32>(42));

    compound_tag b;
    b.put("value", static_cast<i64>(42));

    EXPECT_FALSE(a.equals(b));
}

TEST(NbtEqualsTest, CompoundTagNestedCompoundEquals)
{
    compound_tag a;
    {
        auto inner = std::make_unique<compound_tag>();
        inner->put("x", static_cast<i32>(10));
        inner->put("y", static_cast<i32>(20));
        a.value.emplace("inner", std::move(inner));
    }

    compound_tag b;
    {
        auto inner = std::make_unique<compound_tag>();
        inner->put("x", static_cast<i32>(10));
        inner->put("y", static_cast<i32>(20));
        b.value.emplace("inner", std::move(inner));
    }

    EXPECT_TRUE(a.equals(b));

    // 嵌套值不同
    compound_tag c;
    {
        auto inner = std::make_unique<compound_tag>();
        inner->put("x", static_cast<i32>(10));
        inner->put("y", static_cast<i32>(99));
        c.value.emplace("inner", std::move(inner));
    }

    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, CompoundTagWithListEquals)
{
    compound_tag a;
    {
        auto list = std::make_unique<int_list_tag>(std::vector<i32>{1, 2, 3});
        a.value.emplace("items", std::move(list));
    }

    compound_tag b;
    {
        auto list = std::make_unique<int_list_tag>(std::vector<i32>{1, 2, 3});
        b.value.emplace("items", std::move(list));
    }

    EXPECT_TRUE(a.equals(b));

    // 列表内容不同
    compound_tag c;
    {
        auto list = std::make_unique<int_list_tag>(std::vector<i32>{1, 2, 4});
        c.value.emplace("items", std::move(list));
    }

    EXPECT_FALSE(a.equals(c));
}

TEST(NbtEqualsTest, CompoundTagDeepNesting)
{
    // 三层嵌套
    compound_tag a;
    {
        auto level1 = std::make_unique<compound_tag>();
        auto level2 = std::make_unique<compound_tag>();
        level2->put("deep", std::string("value"));
        level1->value.emplace("level2", std::move(level2));
        a.value.emplace("level1", std::move(level1));
    }

    compound_tag b;
    {
        auto level1 = std::make_unique<compound_tag>();
        auto level2 = std::make_unique<compound_tag>();
        level2->put("deep", std::string("value"));
        level1->value.emplace("level2", std::move(level2));
        b.value.emplace("level1", std::move(level1));
    }

    EXPECT_TRUE(a.equals(b));
}

TEST(NbtEqualsTest, CompoundTagWithArrayTags)
{
    compound_tag a;
    a.value.emplace("bytes", std::make_unique<bytearray_tag>(std::vector<i8>{1, 2, 3}));
    a.value.emplace("ints", std::make_unique<intarray_tag>(std::vector<i32>{10, 20}));
    a.value.emplace("longs", std::make_unique<longarray_tag>(std::vector<i64>{100LL, 200LL}));

    compound_tag b;
    b.value.emplace("bytes", std::make_unique<bytearray_tag>(std::vector<i8>{1, 2, 3}));
    b.value.emplace("ints", std::make_unique<intarray_tag>(std::vector<i32>{10, 20}));
    b.value.emplace("longs", std::make_unique<longarray_tag>(std::vector<i64>{100LL, 200LL}));

    EXPECT_TRUE(a.equals(b));
}

// ========== 多态比较测试（通过基类引用） ==========

TEST(NbtEqualsTest, PolymorphicComparison)
{
    auto intTag = std::make_unique<int_tag>(42);
    auto anotherIntTag = std::make_unique<int_tag>(42);
    auto stringTag = std::make_unique<string_tag>(std::string("hello"));

    // 通过基类引用比较
    const tag& ref1 = *intTag;
    const tag& ref2 = *anotherIntTag;
    const tag& ref3 = *stringTag;

    EXPECT_TRUE(ref1.equals(ref2));
    EXPECT_TRUE(ref1 == ref2);
    EXPECT_FALSE(ref1.equals(ref3));
    EXPECT_TRUE(ref1 != ref3);
}

TEST(NbtEqualsTest, UniquePtrCompoundTagEquals)
{
    auto a = std::make_unique<compound_tag>();
    a->put("name", std::string("Test"));
    a->put("value", static_cast<i32>(42));

    auto b = std::make_unique<compound_tag>();
    b->put("name", std::string("Test"));
    b->put("value", static_cast<i32>(42));

    // 通过解引用比较
    EXPECT_TRUE(*a == *b);
    EXPECT_FALSE(*a != *b);
}

// ========== CappedStructureProcessor 场景测试 ==========

TEST(NbtEqualsTest, CappedProcessorScenario)
{
    // 模拟 CappedStructureProcessor 中比较处理前后 NBT 的场景

    // 场景1：两个完全相同的 NBT
    auto nbt1 = std::make_unique<compound_tag>();
    nbt1->put("id", std::string("minecraft:chest"));
    nbt1->put("x", static_cast<i32>(10));
    nbt1->put("y", static_cast<i32>(20));
    nbt1->put("z", static_cast<i32>(30));

    auto nbt2 = std::make_unique<compound_tag>();
    nbt2->put("id", std::string("minecraft:chest"));
    nbt2->put("x", static_cast<i32>(10));
    nbt2->put("y", static_cast<i32>(20));
    nbt2->put("z", static_cast<i32>(30));

    EXPECT_TRUE(nbt1->equals(*nbt2));

    // 场景2：值不同（键相同）
    auto nbt3 = std::make_unique<compound_tag>();
    nbt3->put("id", std::string("minecraft:chest"));
    nbt3->put("x", static_cast<i32>(10));
    nbt3->put("y", static_cast<i32>(99)); // 不同的值
    nbt3->put("z", static_cast<i32>(30));

    EXPECT_FALSE(nbt1->equals(*nbt3));

    // 场景3：键集合不同
    auto nbt4 = std::make_unique<compound_tag>();
    nbt4->put("id", std::string("minecraft:chest"));
    nbt4->put("x", static_cast<i32>(10));
    nbt4->put("y", static_cast<i32>(20));
    // 缺少 z
    nbt4->put("CustomName", std::string("My Chest")); // 多了一个键

    EXPECT_FALSE(nbt1->equals(*nbt4));
}

TEST(NbtEqualsTest, SelfEquality)
{
    compound_tag a;
    a.put("name", std::string("Test"));
    a.put("value", static_cast<i32>(42));

    // 自反性：a == a
    EXPECT_TRUE(a.equals(a));
    EXPECT_TRUE(a == a);

    // 各类型标签的自反性
    byte_tag b(static_cast<i8>(1));
    EXPECT_TRUE(b.equals(b));

    string_tag s(std::string("test"));
    EXPECT_TRUE(s.equals(s));

    bytearray_tag arr(std::vector<i8>{1, 2, 3});
    EXPECT_TRUE(arr.equals(arr));
}
