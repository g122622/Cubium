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

// GameTest 注册体系单元测试（不依赖 ServerWorld）。
//
// 覆盖：
//   - GameTestRegistrar::register 返回 NativeTestRegistrationBuilder 链式 11 方法
//   - NativeGameTestFunction 直接构造（5 参）
//   - GameTestRegistry 单例：registerTestMethod / allTestFunctions / getTestFunction /
//     getTestsByPattern / 重复 testName 返回 false / clearAllTestMethods
//
// 测试隔离：每个 TEST_F 用例结束 clearAllTestMethods()，避免跨用例污染。
// 静态初始化的 MC_REGISTER_GAME_TEST 在进程启动时已注册内置样例，故用例内用独立 testName。

#include <gtest/gtest.h>

#include "common/test/base/data/TestData.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"
#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "common/test/native/NativeGameTestFunction.hpp"
#include "common/test/native/NativeTestRegistrationBuilder.hpp"
#include "server/test/facade/GameTestRegistrar.hpp"

#include <memory>
#include <string>

namespace {
// 测试夹具：每用例清空注册表，避免 testName 撞或跨用例残留。
class GameTestRegistrationFixture : public ::testing::Test {
protected:
    void TearDown() override { mc::test::GameTestRegistry::instance().clearAllTestMethods(); }
};

// 构造一个总是通过的 TestBody。
mc::test::NativeGameTestFunction::TestBody _passBody()
{
    return [](mc::test::IGameTestHelper& /*helper*/) { return mc::test::pass(); };
}
} // namespace

// ============================================================================
// NativeTestRegistrationBuilder 链式方法（经 GameTestRegistrar::register）
// ============================================================================

TEST_F(GameTestRegistrationFixture, RegistrarChainSetsAllFields)
{
    auto builder = mc::test::GameTestRegistrar::create("SuiteChain", "test_chain", _passBody());
    bool ok = builder.batch("day")
                  .maxAttempts(3)
                  .maxTicks(150)
                  .padding(2)
                  .required(false)
                  .requiredSuccessfulAttempts(2)
                  .rotate(true)
                  .setupTicks(5)
                  .structureName("gametest:empty_3x3")
                  .structureLocation("gametest:empty_3x3")
                  .tag("smoke")
                  .registerTest();
    EXPECT_TRUE(ok);

    auto fn = mc::test::GameTestRegistry::instance().getTestFunction("test_chain");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->testName(), "test_chain");
    EXPECT_EQ(fn->structureName(), "gametest:empty_3x3");
    EXPECT_EQ(fn->data().maxTicks(), 150);
    EXPECT_EQ(fn->data().setupTicks(), 5);
    EXPECT_FALSE(fn->data().required());
    EXPECT_EQ(fn->data().maxAttempts(), 3);
    EXPECT_EQ(fn->data().requiredSuccesses(), 2);
    EXPECT_EQ(fn->data().padding(), 2);
    EXPECT_TRUE(fn->hasTag("smoke"));
}

TEST_F(GameTestRegistrationFixture, RegistrarDefaultsWhenNotChained)
{
    auto builder = mc::test::GameTestRegistrar::create("SuiteDefault", "test_default", _passBody());
    bool ok = builder.structureName("gametest:empty_3x3").registerTest();
    EXPECT_TRUE(ok);

    auto fn = mc::test::GameTestRegistry::instance().getTestFunction("test_default");
    ASSERT_NE(fn, nullptr);
    // 未链式设置的应取 TestData 默认值
    EXPECT_EQ(fn->data().maxTicks(), 100);
    EXPECT_TRUE(fn->data().required());
    EXPECT_EQ(fn->data().maxAttempts(), 1);
}

// ============================================================================
// GameTestRegistry 查询
// ============================================================================

TEST_F(GameTestRegistrationFixture, RegisterAndRetrieveByExactName)
{
    auto builder = mc::test::GameTestRegistrar::create("SuiteQuery", "test_query_exact", _passBody());
    builder.structureName("gametest:empty_3x3").registerTest();

    EXPECT_EQ(mc::test::GameTestRegistry::instance().allTestFunctions().size(), 1u);
    auto fn = mc::test::GameTestRegistry::instance().getTestFunction("test_query_exact");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->testName(), "test_query_exact");
}

TEST_F(GameTestRegistrationFixture, DuplicateTestNameRejected)
{
    auto b1 = mc::test::GameTestRegistrar::create("SuiteDup", "test_dup", _passBody());
    EXPECT_TRUE(b1.structureName("gametest:empty_3x3").registerTest());

    auto b2 = mc::test::GameTestRegistrar::create("SuiteDup", "test_dup", _passBody());
    EXPECT_FALSE(b2.structureName("gametest:empty_3x3").registerTest());

    // 仍只有一个
    EXPECT_EQ(mc::test::GameTestRegistry::instance().allTestFunctions().size(), 1u);
}

TEST_F(GameTestRegistrationFixture, GetTestsByPatternPrefix)
{
    auto b1 = mc::test::GameTestRegistrar::create("SuitePat", "pat_one", _passBody());
    b1.structureName("gametest:empty_3x3").registerTest();
    auto b2 = mc::test::GameTestRegistrar::create("SuitePat", "pat_two", _passBody());
    b2.structureName("gametest:empty_3x3").registerTest();
    auto b3 = mc::test::GameTestRegistrar::create("SuitePat", "other_three", _passBody());
    b3.structureName("gametest:empty_3x3").registerTest();

    // "pat*" 通配符前缀匹配（对齐 Java FilenameUtils.wildcardMatch），命中 pat_one/pat_two。
    auto matched = mc::test::GameTestRegistry::instance().getTestsByPattern("pat*");
    EXPECT_EQ(matched.size(), 2u);
}

TEST_F(GameTestRegistrationFixture, GetTestsByPatternSubstringAndSingleChar)
{
    auto b1 = mc::test::GameTestRegistrar::create("SuiteWc", "llama_spits", _passBody());
    b1.structureName("gametest:empty_3x3").registerTest();
    auto b2 = mc::test::GameTestRegistrar::create("SuiteWc", "llama_defends", _passBody());
    b2.structureName("gametest:empty_3x3").registerTest();
    auto b3 = mc::test::GameTestRegistrar::create("SuiteWc", "camel_walks", _passBody());
    b3.structureName("gametest:empty_3x3").registerTest();

    // "*llama*" 子串匹配（修复旧 prefix.* 实现下 .*llama.* 因空前缀静默匹配全部的 bug）。
    auto sub = mc::test::GameTestRegistry::instance().getTestsByPattern("*llama*");
    EXPECT_EQ(sub.size(), 2u);

    // "?" 匹配单字符："camel_?alks" 命中 camel_walks，不命中 llama_spits/llama_defends。
    auto single = mc::test::GameTestRegistry::instance().getTestsByPattern("camel_?alks");
    EXPECT_EQ(single.size(), 1u);
}

TEST_F(GameTestRegistrationFixture, GetTestsByPatternLiteralDotDoesNotMatchUnderscore)
{
    auto b1 = mc::test::GameTestRegistrar::create("SuiteDot", "pat_one", _passBody());
    b1.structureName("gametest:empty_3x3").registerTest();

    // "." 在通配符中是字面点（非任意分隔符），"pat.*" 命中 "pat.<后缀>" 而非 "pat_one"。
    // 这正是旧 prefix.* 实现的错误：把 ".*" 当成"前缀通配"，实际应只有 "pat*" 才命中 pat_one。
    auto matched = mc::test::GameTestRegistry::instance().getTestsByPattern("pat.*");
    EXPECT_EQ(matched.size(), 0u);
}

TEST_F(GameTestRegistrationFixture, ClearAllTestMethodsEmptiesRegistry)
{
    auto b = mc::test::GameTestRegistrar::create("SuiteClear", "test_clear", _passBody());
    b.structureName("gametest:empty_3x3").registerTest();
    ASSERT_EQ(mc::test::GameTestRegistry::instance().allTestFunctions().size(), 1u);

    mc::test::GameTestRegistry::instance().clearAllTestMethods();
    EXPECT_EQ(mc::test::GameTestRegistry::instance().allTestFunctions().size(), 0u);
    EXPECT_EQ(mc::test::GameTestRegistry::instance().getTestFunction("test_clear"), nullptr);
}

// ============================================================================
// NativeGameTestFunction 直接构造（不经 Registrar，验证底层 5 参 ctor）
// ============================================================================

TEST_F(GameTestRegistrationFixture, NativeGameTestFunctionDirectConstruction)
{
    mc::test::TestData data;
    data.setStructure("gametest:direct").setMaxTicks(50).setRequired(true);
    auto fn = std::make_shared<mc::test::NativeGameTestFunction>(
        std::string{"SuiteDirect"}, std::string{"test_direct"}, std::string{"gametest:direct"}, data, _passBody());

    EXPECT_EQ(fn->testName(), "test_direct");
    EXPECT_EQ(fn->structureName(), "gametest:direct");
    EXPECT_EQ(fn->data().maxTicks(), 50);

    EXPECT_TRUE(mc::test::GameTestRegistry::instance().registerTestMethod("SuiteDirect", fn));
    auto retrieved = mc::test::GameTestRegistry::instance().getTestFunction("test_direct");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->structureName(), "gametest:direct");
}
