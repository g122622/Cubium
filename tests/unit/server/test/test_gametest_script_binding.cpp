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

// @minecraft/server-gametest JS 绑定骨架测试。
//
// 覆盖：
//   - GameTestModuleBinding 元数据（name/uuid/supportedVersions/dependencies）
//   - 工厂注册到独立 ScriptManager 引擎（addModuleFactory + findModuleFactory 可见）
//
// 不覆盖 registerBindings（需 IScriptContext + IDependencyLoader + IScriptPrinter，
// 重型依赖）；完整 JS 执行链路（register → ScriptGameTestFunction → GameTestRegistry）
// 由 test_gametest_server.cpp 经 GameTestServer 挂载模块工厂端到端验证。
// 第一阶段骨架的 thenWait/idle/Async 等 TODO 见 src/server/test/script/README.md。

#include <gtest/gtest.h>

#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "server/test/script/GameTestModuleBinding.hpp"

#include <memory>
#include <string>

// ============================================================================
// 模块元数据
// ============================================================================

TEST(GameTestScriptBinding, ModuleNameMatchesBedrockContract)
{
    mc::test::GameTestModuleBinding binding;
    EXPECT_EQ(binding.name(), "@minecraft/server-gametest");
}

TEST(GameTestScriptBinding, ModuleUuidIsStable)
{
    mc::test::GameTestModuleBinding binding;
    EXPECT_EQ(binding.uuid(), "b9c4d8e1-2f3a-4b5c-9d6e-7f8a9b0c1d2e");
}

TEST(GameTestScriptBinding, SupportedVersionsNonEmpty)
{
    mc::test::GameTestModuleBinding binding;
    auto versions = binding.supportedVersions();
    EXPECT_FALSE(versions.empty());
}

TEST(GameTestScriptBinding, HasAliasMatchesName)
{
    mc::test::GameTestModuleBinding binding;
    EXPECT_TRUE(binding.hasAlias("@minecraft/server-gametest"));
    EXPECT_FALSE(binding.hasAlias("@minecraft/server"));
}

TEST(GameTestScriptBinding, DependenciesForFirstVersionReturnsVector)
{
    mc::test::GameTestModuleBinding binding;
    auto versions = binding.supportedVersions();
    ASSERT_FALSE(versions.empty());
    // dependencies 返回依赖列表（可为空 vector，但须不崩）。
    auto deps = binding.dependencies(versions.front());
    // 第一阶段无显式依赖（@minecraft/server-gametest 独立），deps 可空。
    EXPECT_NO_FATAL_FAILURE(deps.size());
}

// ============================================================================
// 工厂注册到 ScriptManager 引擎
// ============================================================================

TEST(GameTestScriptBinding, FactoryRegisteredInStandaloneEngine)
{
    mc::mod::bedrock::addon::ScriptManager manager;
    ASSERT_TRUE(manager.initialize().success());

    manager.engine().addModuleFactory(std::make_unique<mc::test::GameTestModuleBinding>());

    auto* factory = manager.engine().findModuleFactory("@minecraft/server-gametest");
    EXPECT_NE(factory, nullptr);
    if (factory != nullptr) {
        EXPECT_EQ(factory->name(), "@minecraft/server-gametest");
    }

    manager.shutdown();
}

TEST(GameTestScriptBinding, FactoryNotPresentBeforeRegistration)
{
    mc::mod::bedrock::addon::ScriptManager manager;
    ASSERT_TRUE(manager.initialize().success());

    // 未注册前查找应返回 nullptr。
    auto* factory = manager.engine().findModuleFactory("@minecraft/server-gametest");
    EXPECT_EQ(factory, nullptr);

    manager.shutdown();
}
