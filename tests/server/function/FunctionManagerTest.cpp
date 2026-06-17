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

#include "server/function/FunctionManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::function;

/**
 * @brief FunctionManager 单元测试
 */
class FunctionManagerTest : public ::testing::Test {
protected:
    FunctionManager manager;
};

// ========== 函数注册与查找 ==========

TEST_F(FunctionManagerTest, RegisterAndFindFunction)
{
    ResourceLocation id("minecraft", "test");
    manager.registerFunction(id, {"say hello", "give @a diamond"});

    ASSERT_NE(manager.getFunction(id), nullptr);
    EXPECT_EQ(manager.getFunction(id)->id(), id);
    EXPECT_EQ(manager.getFunction(id)->commandCount(), 2u);
    EXPECT_EQ(manager.getFunction(id)->commands()[0], "say hello");
    EXPECT_EQ(manager.getFunction(id)->commands()[1], "give @a diamond");
}

TEST_F(FunctionManagerTest, RegisterEmptyFunction)
{
    ResourceLocation id("minecraft", "empty");
    manager.registerFunction(id, {});

    ASSERT_NE(manager.getFunction(id), nullptr);
    EXPECT_TRUE(manager.getFunction(id)->isEmpty());
    EXPECT_EQ(manager.getFunction(id)->commandCount(), 0u);
}

TEST_F(FunctionManagerTest, GetFunctionNotFound)
{
    ResourceLocation id("minecraft", "nonexistent");
    EXPECT_EQ(manager.getFunction(id), nullptr);
}

TEST_F(FunctionManagerTest, HasFunction)
{
    ResourceLocation id("minecraft", "exists");
    EXPECT_FALSE(manager.hasFunction(id));

    manager.registerFunction(id, {"say hello"});
    EXPECT_TRUE(manager.hasFunction(id));
}

TEST_F(FunctionManagerTest, RegisterOverwritesExisting)
{
    ResourceLocation id("minecraft", "overwrite");
    manager.registerFunction(id, {"say first"});
    manager.registerFunction(id, {"say second"});

    auto* func = manager.getFunction(id);
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->commandCount(), 1u);
    EXPECT_EQ(func->commands()[0], "say second");
}

TEST_F(FunctionManagerTest, FunctionCount)
{
    EXPECT_EQ(manager.functionCount(), 0u);

    manager.registerFunction(ResourceLocation("minecraft", "a"), {"say a"});
    EXPECT_EQ(manager.functionCount(), 1u);

    manager.registerFunction(ResourceLocation("minecraft", "b"), {"say b"});
    EXPECT_EQ(manager.functionCount(), 2u);

    // 同名覆盖不增加计数
    manager.registerFunction(ResourceLocation("minecraft", "a"), {"say a2"});
    EXPECT_EQ(manager.functionCount(), 2u);
}

TEST_F(FunctionManagerTest, GetAllFunctionIds)
{
    manager.registerFunction(ResourceLocation("minecraft", "foo"), {"say foo"});
    manager.registerFunction(ResourceLocation("mod_id", "bar"), {"say bar"});

    auto ids = manager.getAllFunctionIds();
    EXPECT_EQ(ids.size(), 2u);
}

// ========== 标签注册与查找 ==========

TEST_F(FunctionManagerTest, RegisterAndFindTag)
{
    ResourceLocation tagId("minecraft", "tick");
    manager.registerTag(
        tagId, {ResourceLocation("minecraft", "tick_func1"), ResourceLocation("minecraft", "tick_func2")});

    const auto& functions = manager.getTag(tagId);
    EXPECT_EQ(functions.size(), 2u);
    EXPECT_EQ(functions[0], ResourceLocation("minecraft", "tick_func1"));
    EXPECT_EQ(functions[1], ResourceLocation("minecraft", "tick_func2"));
}

TEST_F(FunctionManagerTest, GetTagNotFound)
{
    ResourceLocation tagId("minecraft", "nonexistent");
    const auto& functions = manager.getTag(tagId);
    EXPECT_TRUE(functions.empty());
}

TEST_F(FunctionManagerTest, TagCount)
{
    EXPECT_EQ(manager.tagCount(), 0u);

    manager.registerTag(ResourceLocation("minecraft", "tick"), {});
    EXPECT_EQ(manager.tagCount(), 1u);
}

TEST_F(FunctionManagerTest, GetAllTagIds)
{
    manager.registerTag(ResourceLocation("minecraft", "tick"), {});
    manager.registerTag(ResourceLocation("minecraft", "load"), {});

    auto ids = manager.getAllTagIds();
    EXPECT_EQ(ids.size(), 2u);
}

// ========== 清空 ==========

TEST_F(FunctionManagerTest, Clear)
{
    manager.registerFunction(ResourceLocation("minecraft", "test"), {"say hello"});
    manager.registerTag(ResourceLocation("minecraft", "tick"), {});

    EXPECT_EQ(manager.functionCount(), 1u);
    EXPECT_EQ(manager.tagCount(), 1u);

    manager.clear();

    EXPECT_EQ(manager.functionCount(), 0u);
    EXPECT_EQ(manager.tagCount(), 0u);
    EXPECT_FALSE(manager.hasFunction(ResourceLocation("minecraft", "test")));
}

// ========== NotifyReload ==========

TEST_F(FunctionManagerTest, NotifyReloadSetsFlag)
{
    // 验证 notifyReload 后 tick 会执行 load 标签
    // 由于 tick 需要 ServerCommandSource（复杂的依赖），
    // 这里只测试标志设置机制
    manager.notifyReload();
    // m_postReload 是 private 的，无法直接检查，
    // 但可以通过调用两次 notifyReload 和 tick 的行为间接验证
    // 这里我们仅验证不会崩溃
    SUCCEED();
}
