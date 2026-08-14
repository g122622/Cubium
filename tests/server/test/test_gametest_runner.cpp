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

// GameTestRunner 编排层单元测试（不依赖 ServerWorld）。
//
// GameTestRunner::build() 需 ServerWorld&（runner 在其上放结构/生成实体），无法 headless。
// 本测试覆盖可独立验证的编排组件：
//   - StructureGridSpawner 网格几何（peekOrigin/advance 两步协议、8/行、列/行间距 32）
//   - GameTestBatch 构造与访问器
//   - GameTestRunnerBuilder 链式方法（不调 build，避免 ServerWorld 依赖）
// 端到端 runner 调度由 test_gametest_server.cpp 覆盖。

#include <gtest/gtest.h>

#include "common/test/base/data/TestData.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/batch/GameTestBatch.hpp"
#include "common/test/framework/environment/EnvironmentRegistry.hpp"
#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"
#include "common/test/native/NativeGameTestFunction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/test/runner/GameTestRunnerBuilder.hpp"
#include "server/test/runner/spawner/StructureGridSpawner.hpp"

#include <memory>
#include <string>
#include <type_traits>

using mc::i32; // i32 属 mc::（非 mc::test），测试内简写

namespace {
mc::test::NativeGameTestFunction::TestBody _passBody()
{
    return [](mc::test::IGameTestHelper& /*h*/) { return mc::test::pass(); };
}

std::shared_ptr<mc::test::NativeGameTestFunction> _makeFunction(const std::string& name)
{
    mc::test::TestData data;
    data.setStructure("gametest:empty_3x3").setMaxTicks(20);
    return std::make_shared<mc::test::NativeGameTestFunction>(
        std::string{"SuiteRunner"}, name, std::string{"gametest:empty_3x3"}, data, _passBody());
}
} // namespace

// ============================================================================
// StructureGridSpawner 网格几何
//
// 两步协议：每个测试先 peekOrigin() 取原点（不推进），再 advance(sizeX, sizeZ, padding) 推进游标。
// ============================================================================

TEST(GameTestRunner, GridSpawnerFirstOriginAtGridStart)
{
    mc::test::StructureGridSpawner spawner(mc::BlockPos{100, -59, 200}, 8);
    EXPECT_EQ(spawner.gridStart(), (mc::BlockPos{100, -59, 200}));
    EXPECT_EQ(spawner.testsPerRow(), 8);
    // 首个测试原点 = 网格起点（peekOrigin 不推进，结构尺寸不影响首个原点）
    EXPECT_EQ(spawner.peekOrigin(), (mc::BlockPos{100, -59, 200}));
}

TEST(GameTestRunner, GridSpawnerAdvancesXWithinRow)
{
    mc::test::StructureGridSpawner spawner(mc::BlockPos{0, 0, 0}, 8);
    const i32 sizeX = 3;
    const i32 sizeZ = 3;
    const auto first = spawner.peekOrigin();
    spawner.advance(sizeX, sizeZ, 0);
    const auto second = spawner.peekOrigin();
    // 同行第 2 个 X 应增大（结构 sizeX + 列间距），Z 不变
    EXPECT_GT(second.x, first.x);
    EXPECT_EQ(second.z, first.z);
}

TEST(GameTestRunner, GridSpawnerWrapsToNextRow)
{
    mc::test::StructureGridSpawner spawner(mc::BlockPos{0, 0, 0}, 2); // 每行 2 个
    const i32 sizeX = 3;
    const i32 sizeZ = 3;
    const auto row0col0 = spawner.peekOrigin();
    spawner.advance(sizeX, sizeZ, 0);
    const auto row0col1 = spawner.peekOrigin();
    spawner.advance(sizeX, sizeZ, 0);
    const auto row1col0 = spawner.peekOrigin(); // 换行
    // 换行后 Z 增大（结构 sizeZ + 行间距），X 回到行首
    EXPECT_GT(row1col0.z, row0col0.z);
    EXPECT_EQ(row1col0.x, row0col0.x);
    EXPECT_EQ(row0col1.z, row0col0.z); // 同行 Z 不变
}

TEST(GameTestRunner, GridSpawnerPaddingIncreasesSpacing)
{
    // padding 计入间距：相邻原点 X 差 = sizeX + 2*padding + SPACE_BETWEEN_COLUMNS
    mc::test::StructureGridSpawner spawner(mc::BlockPos{0, 0, 0}, 8);
    const i32 sizeX = 3;
    const i32 sizeZ = 3;
    const i32 padding = 2;
    const auto first = spawner.peekOrigin();
    spawner.advance(sizeX, sizeZ, padding);
    const auto second = spawner.peekOrigin();
    EXPECT_EQ(second.x - first.x, sizeX + padding * 2 + mc::test::StructureGridSpawner::SPACE_BETWEEN_COLUMNS);
    EXPECT_EQ(second.z, first.z);
}

TEST(GameTestRunner, GridSpawnerZeroTestsPerRowDefaultsTo8)
{
    mc::test::StructureGridSpawner spawner(mc::BlockPos{0, 0, 0}, 0);
    EXPECT_EQ(spawner.testsPerRow(), 8); // 0 被兜底为 8
}

// ============================================================================
// GameTestBatch 构造与访问器
// ============================================================================

TEST(GameTestRunner, BatchHoldsFunctionsAndAccessors)
{
    auto fn1 = _makeFunction("runner_a");
    auto fn2 = _makeFunction("runner_b");
    std::vector<std::shared_ptr<mc::test::BaseGameTestFunction>> fns{fn1, fn2};

    bool beforeCalled = false;
    bool afterCalled = false;
    auto env = mc::test::EnvironmentRegistry::instance().getEnvironment("default");
    ASSERT_NE(env, nullptr);

    mc::test::GameTestBatch batch(
        std::string{"day"},
        std::move(fns),
        [&beforeCalled] { beforeCalled = true; },
        [&afterCalled] { afterCalled = true; },
        env);

    EXPECT_EQ(batch.name(), "day");
    EXPECT_EQ(batch.testFunctions().size(), 2u);
    ASSERT_TRUE(batch.beforeBatch());
    ASSERT_TRUE(batch.afterBatch());
    batch.beforeBatch()();
    batch.afterBatch()();
    EXPECT_TRUE(beforeCalled);
    EXPECT_TRUE(afterCalled);
    EXPECT_EQ(batch.environment(), env);
}

// ============================================================================
// GameTestRunnerBuilder 链式方法（不 build，避免 ServerWorld）
// ============================================================================

TEST(GameTestRunner, BuilderChainReturnsSelf)
{
    mc::test::GameTestRunnerBuilder b;
    auto fn = _makeFunction("runner_builder");
    std::vector<std::shared_ptr<mc::test::BaseGameTestFunction>> fns{fn};
    auto env = mc::test::EnvironmentRegistry::instance().getEnvironment("default");
    ASSERT_NE(env, nullptr);
    std::vector<mc::test::GameTestBatch> batches;
    batches.emplace_back(std::string{"day"}, std::move(fns), std::function<void()>{}, std::function<void()>{}, env);

    // 链式设置（world/ticker 因需 ServerWorld/GameTestTicker 实例跳过，仅验证链式返回类型）
    auto& ref = b.batches(std::move(batches)).gridStart(mc::BlockPos{0, -59, 0}).testsPerRow(4);
    static_assert(
        std::is_same_v<decltype(ref), mc::test::GameTestRunnerBuilder&>, "链式方法须返回 GameTestRunnerBuilder&");
    // 不调 build()（需 ServerWorld），仅验证链式编译通过
    SUCCEED();
}
