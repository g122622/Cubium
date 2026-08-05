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

// GameTestHelper 接口契约测试（用 NullGameTestHelper，无 ServerWorld 依赖）。
//
// NullGameTestHelper 是框架自带的空实现（对齐基岩版）：所有断言返回通过、坐标变换原样返回、
// 状态查询返回安全默认值。本测试验证 IGameTestHelper 接口契约的形状正确，保证真实
// GameTestHelper（绑 ServerWorld）与 NullGameTestHelper 行为对称（除断言实际判定外）。
//
// 真实 GameTestHelper 的端到端断言判定由 test_gametest_server.cpp 覆盖（需 ServerWorld）。

#include <gtest/gtest.h>

#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/helper/NullGameTestHelper.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

#include <string>

// ============================================================================
// 状态查询默认值
// ============================================================================

TEST(GameTestHelper, NullHelperDefaultState)
{
    mc::test::NullGameTestHelper helper;
    EXPECT_FALSE(helper.isCompleted());
    EXPECT_FALSE(helper.isCleaningUp());
    EXPECT_EQ(helper.currentTick(), 0);
    EXPECT_EQ(helper.maxTicks(), 100);
    EXPECT_EQ(helper.rotation(), mc::Rotation::None);
    EXPECT_EQ(helper.getTestDirection(), mc::Direction::North);
}

TEST(GameTestHelper, NullHelperLifecycleNoCrash)
{
    mc::test::NullGameTestHelper helper;
    helper.startExecution();
    helper.succeed();
    helper.fail(mc::test::GameTestError{mc::test::GameTestErrorType::FailConditionsMet, "x"});
    // 生命期方法空操作，不应崩溃也不应改 isCompleted（Null 永远 false）
    EXPECT_FALSE(helper.isCompleted());
}

// ============================================================================
// 块断言全返回通过
// ============================================================================

TEST(GameTestHelper, NullHelperBlockAssertionsPass)
{
    mc::test::NullGameTestHelper helper;
    const mc::BlockPos pos{1, 2, 3};
    EXPECT_TRUE(mc::test::isPass(helper.assertBlockPresent("stone", pos, true)));
    EXPECT_TRUE(mc::test::isPass(helper.setBlock("stone", pos, 3)));
    EXPECT_TRUE(mc::test::isPass(helper.destroyBlock(pos, false)));
    EXPECT_TRUE(mc::test::isPass(helper.pressButton(pos)));
    EXPECT_TRUE(mc::test::isPass(helper.pullLever(pos)));
    EXPECT_TRUE(mc::test::isPass(helper.pulseRedstone(pos, 2)));
    EXPECT_TRUE(mc::test::isPass(helper.assertRedstonePower(pos, 0)));
    EXPECT_TRUE(mc::test::isPass(helper.assertIsWaterlogged(pos, false)));
}

// ============================================================================
// 坐标变换原样返回（无旋转）
// ============================================================================

TEST(GameTestHelper, NullHelperTransformIdentity)
{
    mc::test::NullGameTestHelper helper;
    const mc::BlockPos rel{5, 10, 15};
    EXPECT_EQ(helper.worldBlockPosition(rel), rel);
    EXPECT_EQ(helper.relativeBlockPosition(rel), rel);
    EXPECT_EQ(helper.rotateDirection(mc::Direction::North), mc::Direction::North);
}

TEST(GameTestHelper, NullHelperBlockStateQueryReturnsNull)
{
    mc::test::NullGameTestHelper helper;
    EXPECT_EQ(helper.getBlock(mc::BlockPos{0, 0, 0}), nullptr);
}

// ============================================================================
// 完成路径方法不崩溃
// ============================================================================

TEST(GameTestHelper, NullHelperCompletionPathsNoCrash)
{
    mc::test::NullGameTestHelper helper;
    const mc::BlockPos pos{0, 0, 0};
    helper.succeedWhenBlockPresent("stone", pos, true);
    helper.succeedWhen([] { return mc::test::pass(); });
    helper.succeedIf([] { return mc::test::pass(); });
    helper.succeedOnTick(10);
    helper.succeedOnTickWhen(10, [] { return mc::test::pass(); });
    helper.failIf([] { return mc::test::pass(); });
    SUCCEED();
}

// ============================================================================
// 实体断言/spawn 返回通过 + out 参置空
// ============================================================================

TEST(GameTestHelper, NullHelperEntityAssertionsPass)
{
    mc::test::NullGameTestHelper helper;
    const mc::BlockPos pos{0, 0, 0};
    EXPECT_TRUE(mc::test::isPass(helper.assertEntityPresent("minecraft:pig", pos, 5.0f, true)));
    EXPECT_TRUE(mc::test::isPass(helper.assertEntityPresentInArea("minecraft:pig", true)));
    EXPECT_TRUE(mc::test::isPass(helper.killAllEntities()));

    mc::Entity* outEntity = reinterpret_cast<mc::Entity*>(0xDEAD);
    EXPECT_TRUE(mc::test::isPass(helper.spawnEntity("minecraft:pig", pos, outEntity)));
    EXPECT_EQ(outEntity, nullptr);
}

// ============================================================================
// 序列集成：startSequence 返回可驱动序列
// ============================================================================

TEST(GameTestHelper, NullHelperStartSequenceDrivesToSuccess)
{
    mc::test::NullGameTestHelper helper;
    auto& seq = helper.startSequence();
    seq.thenExecute([] { return mc::test::pass(); }).thenSucceed();
    bool succeeded = false;
    for (mc::i32 t = 0; t < 20 && !seq.isComplete(); ++t) {
        seq.tick(t);
    }
    if (seq.isComplete() && seq.isSucceeded()) {
        succeeded = true;
    }
    EXPECT_TRUE(succeeded);
}

// ============================================================================
// 工具方法
// ============================================================================

TEST(GameTestHelper, NullHelperGenerateErrorWithContext)
{
    mc::test::NullGameTestHelper helper;
    auto err = helper.generateErrorWithContext(mc::test::GameTestErrorType::Assert, "msg", mc::BlockPos{1, 2, 3});
    EXPECT_EQ(err.type(), mc::test::GameTestErrorType::Assert);
    EXPECT_EQ(err.message(), "msg");
}

TEST(GameTestHelper, NullHelperPrintNoCrash)
{
    mc::test::NullGameTestHelper helper;
    helper.print("hello gametest");
    SUCCEED();
}
