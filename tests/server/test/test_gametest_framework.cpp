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

// GameTest 框架核心（引擎无关层）单元测试。
//
// 覆盖 base/ + framework/ 中不依赖 ServerWorld 的纯逻辑：
//   - GameTestError / GameTestResult / GameTestErrorContext / GameTestErrorType
//   - TestData schema + json 序列化
//   - TestTransform 坐标变换（含 4 旋转）
//   - GameTestSequence 状态机（用 NullGameTestHelper 驱动 tick，无世界依赖）
//   - GameTestTicker 单例 3 态状态机
//   - EnvironmentRegistry 默认环境
//
// 不覆盖：BaseGameTestInstance（抽象，需子类放结构）/ GameTestRunner（需 ServerWorld），
// 这两者由 test_gametest_server.cpp 端到端覆盖。

#include <gtest/gtest.h>

#include "common/test/base/coords/TestTransform.hpp"
#include "common/test/base/data/TestData.hpp"
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorContext.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/environment/EnvironmentRegistry.hpp"
#include "common/test/framework/helper/NullGameTestHelper.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "common/util/Direction.hpp" // Rotation
#include "common/world/block/BlockPos.hpp"

#include <nlohmann/json.hpp>

namespace {
// 辅助：驱动序列 tick 直到完成或超 tick 上限，返回是否成功完成。
bool _driveSequenceToCompletion(mc::test::GameTestSequence& seq, mc::i32 maxTicks)
{
    for (mc::i32 t = 0; t < maxTicks && !seq.isComplete(); ++t) {
        auto result = seq.tick(t);
        if (result.has_value()) {
            return false; // 序列返回错误即失败
        }
    }
    return seq.isComplete() && seq.isSucceeded();
}
} // namespace

// ============================================================================
// GameTestError / GameTestResult
// ============================================================================

TEST(GameTestFramework, PassResultIsNullopt)
{
    auto result = mc::test::pass();
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(mc::test::isPass(result));
}

TEST(GameTestFramework, FailResultCarriesError)
{
    auto result = mc::test::fail(mc::test::GameTestErrorType::Assert, "assertion failed");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(mc::test::isPass(result));
    EXPECT_EQ(result->type(), mc::test::GameTestErrorType::Assert);
    EXPECT_EQ(result->message(), "assertion failed");
}

TEST(GameTestFramework, FailResultWithParamsAndContext)
{
    std::vector<std::string> params{"stone", "dirt"};
    mc::test::GameTestErrorContext context(mc::BlockPos{1, 2, 3}, mc::BlockPos{0, 0, 0}, 5);
    auto result = mc::test::fail(mc::test::GameTestErrorType::AssertAtPosition, "block mismatch", params, context);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type(), mc::test::GameTestErrorType::AssertAtPosition);
    EXPECT_EQ(result->params().size(), 2u);
    ASSERT_TRUE(result->context().has_value());
    EXPECT_EQ(result->context()->absolutePosition().x, 1);
    EXPECT_EQ(result->context()->tickCount(), 5);
}

TEST(GameTestFramework, ErrorFormattedContainsMessage)
{
    mc::test::GameTestError error(mc::test::GameTestErrorType::ExecutionTimeout, "timed out");
    const std::string formatted = error.formattedMessage();
    EXPECT_NE(formatted.find("timed out"), std::string::npos);
}

TEST(GameTestFramework, ErrorTypeNameRoundTrip)
{
    using E = mc::test::GameTestErrorType;
    EXPECT_STREQ(mc::test::gameTestErrorTypeName(E::Assert), "Assert");
    EXPECT_STREQ(mc::test::gameTestErrorTypeName(E::FailConditionsMet), "FailConditionsMet");
    EXPECT_STREQ(mc::test::gameTestErrorTypeName(E::ExecutionTimeout), "ExecutionTimeout");
}

// ============================================================================
// TestData schema + json 序列化
// ============================================================================

TEST(GameTestFramework, TestDataDefaults)
{
    mc::test::TestData data;
    EXPECT_EQ(data.environment(), "default");
    EXPECT_EQ(data.maxTicks(), 100);
    EXPECT_TRUE(data.required());
    EXPECT_EQ(data.rotation(), mc::Rotation::None);
    EXPECT_FALSE(data.manualOnly());
    EXPECT_EQ(data.maxAttempts(), 1);
    EXPECT_EQ(data.requiredSuccesses(), 1);
    EXPECT_FALSE(data.skyAccess());
    EXPECT_EQ(data.padding(), 0);
    EXPECT_EQ(data.batchName(), "default");
    EXPECT_FALSE(data.isFlaky());
}

TEST(GameTestFramework, TestDataFluentSetters)
{
    auto data = mc::test::TestData{}
                    .setStructure("gametest:empty_3x3")
                    .setMaxTicks(200)
                    .setRequired(false)
                    .setRotation(mc::Rotation::Clockwise90)
                    .setMaxAttempts(3)
                    .setRequiredSuccesses(2)
                    .setPadding(2)
                    .setBatchName("night");
    EXPECT_EQ(data.structure(), "gametest:empty_3x3");
    EXPECT_EQ(data.maxTicks(), 200);
    EXPECT_FALSE(data.required());
    EXPECT_EQ(data.rotation(), mc::Rotation::Clockwise90);
    EXPECT_EQ(data.maxAttempts(), 3);
    EXPECT_EQ(data.requiredSuccesses(), 2);
    EXPECT_EQ(data.padding(), 2);
    EXPECT_EQ(data.batchName(), "night");
    EXPECT_TRUE(data.isFlaky()); // maxAttempts > 1
}

TEST(GameTestFramework, TestDataJsonRoundTrip)
{
    auto data = mc::test::TestData{}
                    .setEnvironment("custom_env")
                    .setStructure("gametest:foo")
                    .setMaxTicks(42)
                    .setSetupTicks(3)
                    .setRequired(true)
                    .setRotation(mc::Rotation::Clockwise180)
                    .setManualOnly(false)
                    .setMaxAttempts(2)
                    .setRequiredSuccesses(1)
                    .setSkyAccess(true)
                    .setPadding(1)
                    .setBatchName("day");
    nlohmann::json j = data;
    EXPECT_EQ(j["environment"], "custom_env");
    EXPECT_EQ(j["structure"], "gametest:foo");
    EXPECT_EQ(j["max_ticks"], 42);
    EXPECT_EQ(j["setup_ticks"], 3);
    EXPECT_EQ(j["required"], true);
    EXPECT_EQ(j["rotation"], "180"); // Clockwise180 → "180"（对齐 Java Rotation codec 名）
    EXPECT_EQ(j["max_attempts"], 2);
    EXPECT_EQ(j["required_successes"], 1);
    EXPECT_EQ(j["sky_access"], true);
    EXPECT_EQ(j["padding"], 1);
    EXPECT_EQ(j["batch_name"], "day");

    // 反序列化回 TestData 验证往返
    mc::test::TestData restored = j.get<mc::test::TestData>();
    EXPECT_EQ(restored.environment(), "custom_env");
    EXPECT_EQ(restored.structure(), "gametest:foo");
    EXPECT_EQ(restored.maxTicks(), 42);
    EXPECT_EQ(restored.rotation(), mc::Rotation::Clockwise180);
    EXPECT_EQ(restored.skyAccess(), true);
    EXPECT_EQ(restored.padding(), 1);
}

// ============================================================================
// TestTransform 坐标变换（含 4 旋转）
// ============================================================================

TEST(GameTestFramework, TransformNoneIdentity)
{
    mc::test::TestTransform t(mc::BlockPos{100, 64, 200}, mc::BlockPos{3, 3, 3}, mc::Rotation::None);
    EXPECT_EQ(t.relativeToWorld(mc::BlockPos{0, 0, 0}), (mc::BlockPos{100, 64, 200}));
    EXPECT_EQ(t.relativeToWorld(mc::BlockPos{2, 1, 2}), (mc::BlockPos{102, 65, 202}));
    // 往返
    EXPECT_EQ(t.worldToRelative(mc::BlockPos{102, 65, 202}), (mc::BlockPos{2, 1, 2}));
}

TEST(GameTestFramework, TransformClockwise90)
{
    // sizeX=2 sizeZ=4；Clockwise90: (rx,ry,rz) -> (sizeZ-1-rz, ry, rx)
    mc::test::TestTransform t(mc::BlockPos{0, 0, 0}, mc::BlockPos{2, 2, 4}, mc::Rotation::Clockwise90);
    EXPECT_EQ(t.relativeToWorld(mc::BlockPos{0, 0, 0}), (mc::BlockPos{3, 0, 0}));
    EXPECT_EQ(t.relativeToWorld(mc::BlockPos{1, 1, 3}), (mc::BlockPos{0, 1, 1}));
}

TEST(GameTestFramework, TransformRotatedSizeSwaps90)
{
    mc::test::TestTransform t90(mc::BlockPos{0, 0, 0}, mc::BlockPos{2, 3, 4}, mc::Rotation::Clockwise90);
    EXPECT_EQ(t90.rotatedSize(), (mc::BlockPos{4, 3, 2})); // X/Z 互换
    mc::test::TestTransform t180(mc::BlockPos{0, 0, 0}, mc::BlockPos{2, 3, 4}, mc::Rotation::Clockwise180);
    EXPECT_EQ(t180.rotatedSize(), (mc::BlockPos{2, 3, 4})); // 不变
}

// ============================================================================
// GameTestSequence 状态机（NullGameTestHelper 驱动）
// ============================================================================

TEST(GameTestFramework, SequenceThenSucceedCompletes)
{
    mc::test::NullGameTestHelper helper;
    auto& seq = helper.startSequence();
    seq.thenExecute([] { return mc::test::pass(); }).thenSucceed();
    EXPECT_TRUE(_driveSequenceToCompletion(seq, 20));
    EXPECT_TRUE(seq.isComplete());
    EXPECT_TRUE(seq.isSucceeded());
}

TEST(GameTestFramework, SequenceThenIdleDelaysSucceed)
{
    mc::test::NullGameTestHelper helper;
    auto& seq = helper.startSequence();
    seq.thenIdle(3).thenSucceed();
    // thenIdle(3) 需 tick>=3 才完成；前 3 tick 不应成功
    seq.tick(0);
    seq.tick(1);
    EXPECT_FALSE(seq.isComplete());
    EXPECT_TRUE(_driveSequenceToCompletion(seq, 20));
    EXPECT_TRUE(seq.isSucceeded());
}

TEST(GameTestFramework, SequenceThenFailReturnsError)
{
    mc::test::NullGameTestHelper helper;
    auto& seq = helper.startSequence();
    seq.thenExecute([] { return mc::test::pass(); })
        .thenFail(mc::test::GameTestError{mc::test::GameTestErrorType::FailConditionsMet, "intentional"});
    bool sawError = false;
    for (mc::i32 t = 0; t < 20 && !seq.isComplete(); ++t) {
        if (seq.tick(t).has_value()) {
            sawError = true;
        }
    }
    EXPECT_TRUE(seq.isComplete());
    EXPECT_FALSE(seq.isSucceeded());
    EXPECT_TRUE(sawError);
}

TEST(GameTestFramework, SequenceStepErrorFailsSequence)
{
    mc::test::NullGameTestHelper helper;
    auto& seq = helper.startSequence();
    seq.thenExecute([] { return mc::test::fail(mc::test::GameTestErrorType::Assert, "step fail"); });
    bool sawError = false;
    for (mc::i32 t = 0; t < 20 && !seq.isComplete(); ++t) {
        if (seq.tick(t).has_value()) {
            sawError = true;
        }
    }
    EXPECT_TRUE(seq.isComplete());
    EXPECT_FALSE(seq.isSucceeded());
    EXPECT_TRUE(sawError);
}

// ============================================================================
// GameTestTicker 单例状态机
// ============================================================================

TEST(GameTestFramework, TickerInitiallyIdleAndEmpty)
{
    // forceStop 保证前序测试残留清空
    mc::test::GameTestTicker::instance().forceStop();
    EXPECT_EQ(mc::test::GameTestTicker::instance().state(), mc::test::GameTestTicker::State::Idle);
    // isEmpty 在 forceStop 后必为 true
    EXPECT_TRUE(mc::test::GameTestTicker::instance().isEmpty());
    EXPECT_EQ(mc::test::GameTestTicker::instance().instanceCount(), 0u);
}

TEST(GameTestFramework, TickerForceStopClearsImmediately)
{
    // 不添加实例（避免依赖 BaseGameTestInstance），仅验证 forceStop 不崩且状态归 Idle
    mc::test::GameTestTicker::instance().forceStop();
    EXPECT_TRUE(mc::test::GameTestTicker::instance().isEmpty());
}

// ============================================================================
// EnvironmentRegistry 默认环境
// ============================================================================

TEST(GameTestFramework, EnvironmentRegistryDefaultRegistered)
{
    mc::test::EnvironmentRegistry::instance().registerBuiltinDefaults();
    EXPECT_TRUE(mc::test::EnvironmentRegistry::instance().hasEnvironment("default"));
    auto env = mc::test::EnvironmentRegistry::instance().getEnvironment("default");
    EXPECT_NE(env, nullptr);
}

TEST(GameTestFramework, EnvironmentRegistryMissingReturnsNull)
{
    auto env = mc::test::EnvironmentRegistry::instance().getEnvironment("nonexistent_env_xyz");
    EXPECT_EQ(env, nullptr);
    EXPECT_FALSE(mc::test::EnvironmentRegistry::instance().hasEnvironment("nonexistent_env_xyz"));
}
