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

#include "server/function/MacroFunction.hpp"
#include "server/function/StringTemplate.hpp"
#include <stdexcept>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::function;

/**
 * @brief MacroFunction 单元测试
 *
 * 对应 MC 1.21.11 net.minecraft.commands.functions.MacroFunction 的行为。
 */
class MacroFunctionTest : public ::testing::Test {
protected:
    /// 构造一个简单的宏函数：含一个宏行 "$(name)" 和一个纯文本行 "say hello"
    std::unique_ptr<MacroFunction> makeSimpleMacroFunction()
    {
        std::vector<MacroFunctionEntry> entries;
        std::vector<std::string> parameters = {"name"};

        // 宏行：say $(name)
        auto tmpl = StringTemplate::fromString("say $(name)");
        entries.push_back(MacroFunctionEntry::macro(std::move(tmpl), {0}));
        // 纯文本行：say hello
        entries.push_back(MacroFunctionEntry::plainText("say hello"));

        return std::make_unique<MacroFunction>(
            ResourceLocation("minecraft", "test/macro"), std::move(entries), std::move(parameters));
    }

    /// 构造一个含多变量的宏函数：$(a) $(b)
    std::unique_ptr<MacroFunction> makeMultiVarMacroFunction()
    {
        std::vector<MacroFunctionEntry> entries;
        std::vector<std::string> parameters = {"a", "b"};

        // 宏行：tellraw @a {"text":"$(a) $(b)"}
        auto tmpl = StringTemplate::fromString("tellraw @a {\"text\":\"$(a) $(b)\"}");
        entries.push_back(MacroFunctionEntry::macro(std::move(tmpl), {0, 1}));

        return std::make_unique<MacroFunction>(
            ResourceLocation("minecraft", "test/multivar"), std::move(entries), std::move(parameters));
    }

    /// 构造一个含重复变量的宏函数：$(a) $(a)
    std::unique_ptr<MacroFunction> makeRepeatedVarMacroFunction()
    {
        std::vector<MacroFunctionEntry> entries;
        std::vector<std::string> parameters = {"a"};

        // 宏行：say $(a) $(a)
        // StringTemplate 会把 variables 解析为 ["a", "a"]，需要两个 indices 都指向 0
        auto tmpl = StringTemplate::fromString("say $(a) $(a)");
        EXPECT_EQ(tmpl.variables().size(), 2u);
        entries.push_back(MacroFunctionEntry::macro(std::move(tmpl), {0, 0}));

        return std::make_unique<MacroFunction>(
            ResourceLocation("minecraft", "test/repeated"), std::move(entries), std::move(parameters));
    }
};

// ========== 基本属性 ==========

TEST_F(MacroFunctionTest, BasicProperties)
{
    auto func = makeSimpleMacroFunction();
    EXPECT_EQ(func->id().toString(), "minecraft:test/macro");
    EXPECT_EQ(func->commandCount(), 2u);
    EXPECT_FALSE(func->isEmpty());
    EXPECT_TRUE(func->isMacro());
    ASSERT_EQ(func->parameters().size(), 1u);
    EXPECT_EQ(func->parameters()[0], "name");
}

TEST_F(MacroFunctionTest, EmptyMacroFunction)
{
    std::vector<MacroFunctionEntry> emptyEntries;
    std::vector<std::string> emptyParams;
    MacroFunction func(ResourceLocation("minecraft", "empty"), std::move(emptyEntries), std::move(emptyParams));
    EXPECT_TRUE(func.isEmpty());
    EXPECT_EQ(func.commandCount(), 0u);
}

// ========== stringify ==========

TEST_F(MacroFunctionTest, Stringify_Float)
{
    nbt::tags::float_tag tag(3.14f);
    auto s = MacroFunction::stringify(tag);
    // %.15g 格式：float 提升为 double 后输出 15 位有效数字
    EXPECT_EQ(s, "3.14000010490417");
}

TEST_F(MacroFunctionTest, Stringify_FloatWholeNumber)
{
    nbt::tags::float_tag tag(5.0f);
    auto s = MacroFunction::stringify(tag);
    // %.15g 对整数浮点输出 "5"
    EXPECT_EQ(s, "5");
}

TEST_F(MacroFunctionTest, Stringify_Double)
{
    nbt::tags::double_tag tag(2.718281828459045);
    auto s = MacroFunction::stringify(tag);
    // %.15g 应保留 15 位有效数字
    EXPECT_EQ(s, "2.71828182845905");
}

TEST_F(MacroFunctionTest, Stringify_Byte)
{
    nbt::tags::byte_tag tag(42);
    auto s = MacroFunction::stringify(tag);
    // Byte → (int) 字符串
    EXPECT_EQ(s, "42");
}

TEST_F(MacroFunctionTest, Stringify_NegativeByte)
{
    nbt::tags::byte_tag tag(-1);
    auto s = MacroFunction::stringify(tag);
    // 负 byte → (int) -1
    EXPECT_EQ(s, "-1");
}

TEST_F(MacroFunctionTest, Stringify_Short)
{
    nbt::tags::short_tag tag(1234);
    auto s = MacroFunction::stringify(tag);
    // Short → (int) 字符串
    EXPECT_EQ(s, "1234");
}

TEST_F(MacroFunctionTest, Stringify_Long)
{
    nbt::tags::long_tag tag(9000000000LL);
    auto s = MacroFunction::stringify(tag);
    // Long → 字符串，无 L 后缀
    EXPECT_EQ(s, "9000000000");
}

TEST_F(MacroFunctionTest, Stringify_String)
{
    nbt::tags::string_tag tag("hello");
    auto s = MacroFunction::stringify(tag);
    // String → 原始字符串
    EXPECT_EQ(s, "hello");
}

TEST_F(MacroFunctionTest, Stringify_Int_UsesSNBT)
{
    nbt::tags::int_tag tag(42);
    auto s = MacroFunction::stringify(tag);
    // Int → SNBT 文本（即 "42"）
    EXPECT_EQ(s, "42");
}

TEST_F(MacroFunctionTest, Stringify_Compound_UsesSNBT)
{
    nbt::tags::compound_tag tag;
    tag.put("key", std::string("value"));
    auto s = MacroFunction::stringify(tag);
    // Compound → SNBT 文本
    EXPECT_FALSE(s.empty());
    // 应包含 key 和 value
    EXPECT_NE(s.find("key"), std::string::npos);
    EXPECT_NE(s.find("value"), std::string::npos);
}

// ========== instantiate ==========

TEST_F(MacroFunctionTest, Instantiate_NullArguments_Throws)
{
    auto func = makeSimpleMacroFunction();
    EXPECT_THROW(func->instantiate(nullptr), FunctionInstantiationException);
}

TEST_F(MacroFunctionTest, Instantiate_MissingParameter_Throws)
{
    auto func = makeSimpleMacroFunction();
    nbt::tags::compound_tag args;
    // 不含 "name" 形参
    EXPECT_THROW(func->instantiate(&args), FunctionInstantiationException);
}

TEST_F(MacroFunctionTest, Instantiate_SingleParameter)
{
    auto func = makeSimpleMacroFunction();
    nbt::tags::compound_tag args;
    args.put("name", std::string("Steve"));
    auto commands = func->instantiate(&args);
    ASSERT_EQ(commands.size(), 2u);
    EXPECT_EQ(commands[0], "say Steve");
    EXPECT_EQ(commands[1], "say hello");
}

TEST_F(MacroFunctionTest, Instantiate_MultipleParameters)
{
    auto func = makeMultiVarMacroFunction();
    nbt::tags::compound_tag args;
    args.put("a", std::string("Hello"));
    args.put("b", std::string("World"));
    auto commands = func->instantiate(&args);
    ASSERT_EQ(commands.size(), 1u);
    EXPECT_EQ(commands[0], "tellraw @a {\"text\":\"Hello World\"}");
}

TEST_F(MacroFunctionTest, Instantiate_RepeatedVariable)
{
    auto func = makeRepeatedVarMacroFunction();
    nbt::tags::compound_tag args;
    args.put("a", std::string("X"));
    auto commands = func->instantiate(&args);
    ASSERT_EQ(commands.size(), 1u);
    EXPECT_EQ(commands[0], "say X X");
}

TEST_F(MacroFunctionTest, Instantiate_NumericParameter_Stringify)
{
    // 用 Byte 参数验证 stringify 行为
    std::vector<MacroFunctionEntry> entries;
    std::vector<std::string> parameters = {"count"};

    auto tmpl = StringTemplate::fromString("give @a diamond $(count)");
    entries.push_back(MacroFunctionEntry::macro(std::move(tmpl), {0}));

    MacroFunction func(ResourceLocation("minecraft", "numeric"), std::move(entries), std::move(parameters));

    nbt::tags::compound_tag args;
    args.put("count", static_cast<std::int8_t>(5));
    auto commands = func.instantiate(&args);
    ASSERT_EQ(commands.size(), 1u);
    // Byte → "5"（int 字符串）
    EXPECT_EQ(commands[0], "give @a diamond 5");
}

// ========== LRU 缓存 ==========

TEST_F(MacroFunctionTest, Cache_HitReturnsSameCommands)
{
    auto func = makeSimpleMacroFunction();
    nbt::tags::compound_tag args1;
    args1.put("name", std::string("Steve"));
    auto commands1 = func->instantiate(&args1);

    nbt::tags::compound_tag args2;
    args2.put("name", std::string("Steve"));
    auto commands2 = func->instantiate(&args2);

    // 两次调用应返回相同的命令列表
    ASSERT_EQ(commands1.size(), commands2.size());
    for (Size i = 0; i < commands1.size(); ++i) {
        EXPECT_EQ(commands1[i], commands2[i]);
    }
}

TEST_F(MacroFunctionTest, Cache_DifferentArgumentsProduceDifferentCommands)
{
    auto func = makeSimpleMacroFunction();
    nbt::tags::compound_tag args1;
    args1.put("name", std::string("Steve"));
    auto commands1 = func->instantiate(&args1);
    EXPECT_EQ(commands1[0], "say Steve");

    nbt::tags::compound_tag args2;
    args2.put("name", std::string("Alex"));
    auto commands2 = func->instantiate(&args2);
    EXPECT_EQ(commands2[0], "say Alex");
}

TEST_F(MacroFunctionTest, Cache_EvictionAtMaxEntries)
{
    // 验证缓存上限 MAX_CACHE_ENTRIES = 8 不会导致内存无限增长或崩溃
    auto func = makeSimpleMacroFunction();
    // 注入 10 个不同的参数组合（超过缓存上限 8）
    for (int i = 0; i < 10; ++i) {
        nbt::tags::compound_tag args;
        args.put("name", std::string("player") + std::to_string(i));
        auto commands = func->instantiate(&args);
        ASSERT_EQ(commands.size(), 2u);
        EXPECT_EQ(commands[0], "say player" + std::to_string(i));
    }
    // 再访问一次最早条目（应重新实例化，因为已被淘汰）
    nbt::tags::compound_tag args;
    args.put("name", std::string("player0"));
    auto commands = func->instantiate(&args);
    EXPECT_EQ(commands[0], "say player0");
}
