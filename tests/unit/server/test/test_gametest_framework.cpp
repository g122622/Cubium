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

#include "common/TempDirHelper.hpp"
#include "common/util/Direction.hpp" // Rotation
#include "common/world/block/BlockPos.hpp"
#include "server/test/base/coords/TestTransform.hpp"
#include "server/test/base/data/TestData.hpp"
#include "server/test/base/error/GameTestError.hpp"
#include "server/test/base/error/GameTestErrorContext.hpp"
#include "server/test/base/error/GameTestErrorType.hpp"
#include "server/test/base/error/GameTestResult.hpp"
#include "server/test/framework/environment/EnvironmentRegistry.hpp"
#include "server/test/framework/helper/NullGameTestHelper.hpp"
#include "server/test/framework/instance/BaseGameTestInstance.hpp"
#include "server/test/framework/listener/IGameTestListener.hpp"
#include "server/test/framework/sequence/GameTestSequence.hpp"
#include "server/test/framework/ticker/GameTestTicker.hpp"
#include "server/test/native/NativeGameTestFunction.hpp"
#include "server/test/runner/reporter/JUnitTestReporter.hpp"
#include "server/test/runner/watchdog/GameTestWatchdog.hpp"

#include <fstream>
#include <future>
#include <thread>

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

namespace {
class _TimeoutHelperProvider final : public mc::test::IGameTestHelperProvider {
public:
    std::unique_ptr<mc::test::IGameTestHelper> createGameTestHelper(mc::test::BaseGameTestInstance&) override
    {
        return std::make_unique<mc::test::NullGameTestHelper>();
    }
    std::unique_ptr<mc::test::IGameTestHelperProvider> clone() const override
    {
        return std::make_unique<_TimeoutHelperProvider>();
    }
};

class _TimeoutInstance final : public mc::test::BaseGameTestInstance {
public:
    explicit _TimeoutInstance(const mc::test::BaseGameTestFunction& function)
        : BaseGameTestInstance(function, std::make_unique<_TimeoutHelperProvider>())
    {}

protected:
    bool hasStructureBlock() const override { return true; }
    void clearStructure() override {}
    void spawnStructure() override {}
    mc::i32 _getLevelTick() const override { return 0; }
    bool _isTestReady() override { return true; }
};

std::shared_ptr<mc::test::NativeGameTestFunction> _timeoutFunction(const std::string& name)
{
    return std::make_shared<mc::test::NativeGameTestFunction>(
        "timeouts", name, "empty", mc::test::TestData{}, [](mc::test::IGameTestHelper&) { return mc::test::pass(); });
}
} // namespace

TEST(GameTestTimeout, WatchdogFiresWhileCallingThreadWaits)
{
    std::promise<std::string> expired;
    auto result = expired.get_future();
    mc::test::GameTestWatchdog watchdog(std::chrono::steady_clock::now() + std::chrono::milliseconds(30),
        "tick",
        [&](const std::string& phase) { expired.set_value(phase); });
    ASSERT_EQ(result.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    EXPECT_EQ(result.get(), "tick");
}

TEST(GameTestTimeout, EarlierBatchDeadlineReplacesSuiteDeadline)
{
    std::promise<std::string> expired;
    auto result = expired.get_future();
    mc::test::GameTestWatchdog watchdog(std::chrono::steady_clock::now() + std::chrono::hours(1),
        "suite",
        [&](const std::string& phase) { expired.set_value(phase); });
    watchdog.arm(std::chrono::steady_clock::now() + std::chrono::milliseconds(30), "batch");
    ASSERT_EQ(result.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    EXPECT_EQ(result.get(), "batch");
}

TEST(GameTestTimeout, FinishedTickDisarmsItsShortDeadline)
{
    std::promise<std::string> expired;
    auto result = expired.get_future();
    mc::test::GameTestWatchdog watchdog(std::chrono::steady_clock::now() + std::chrono::milliseconds(200),
        "tick",
        [&](const std::string& phase) { expired.set_value(phase); });
    watchdog.arm(std::chrono::steady_clock::now() + std::chrono::milliseconds(500), "batch");
    EXPECT_EQ(result.wait_for(std::chrono::milliseconds(250)), std::future_status::timeout);
    ASSERT_EQ(result.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    EXPECT_EQ(result.get(), "batch");
}

TEST(GameTestTimeout, ExpiredTickCannotBeForgivenByNextBatch)
{
    std::promise<std::string> expired;
    auto result = expired.get_future();
    mc::test::GameTestWatchdog watchdog(std::chrono::steady_clock::now() - std::chrono::milliseconds(1),
        "expired tick",
        [&](const std::string& phase) { expired.set_value(phase); });
    watchdog.arm(std::chrono::steady_clock::now() + std::chrono::hours(1), "next batch");
    ASSERT_EQ(result.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    EXPECT_EQ(result.get(), "expired tick");
}

TEST(GameTestTimeout, DestructionCancelsOutstandingDeadline)
{
    bool fired = false;
    const auto started = std::chrono::steady_clock::now();
    {
        mc::test::GameTestWatchdog watchdog(
            started + std::chrono::hours(1), "suite", [&](const std::string&) { fired = true; });
    }
    EXPECT_FALSE(fired);
    EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(1));
}

TEST(GameTestTimeout, SnapshotPreservesFinishedInterruptedAndUnstartedCases)
{
    const auto dir = mc::test::makeUniqueTestDir("gametest_timeout_report");
    const auto report = dir / "results.xml";
    auto passed = _timeoutFunction("completed");
    auto failed = _timeoutFunction("failed");
    auto active = _timeoutFunction("interrupted");
    auto pending = _timeoutFunction("not_run");
    auto excluded = _timeoutFunction("manual_only");
    _TimeoutInstance passedInstance(*passed), failedInstance(*failed), activeInstance(*active);
    mc::test::JUnitTestReporter reporter(report);
    reporter.prepareTests({passed, failed, active, pending, excluded});
    reporter.onTestSkipped(*excluded, "Manual-only test excluded from automatic run");
    reporter.onTestStarted(activeInstance);
    passedInstance.succeed();
    reporter.onTestPassed(passedInstance);
    failedInstance.fail(mc::test::GameTestError(mc::test::GameTestErrorType::ExecutionTimeout, "timed out"));
    reporter.onTestFailed(failedInstance);
    // 不调用 onAllFinished，模拟进程被看门狗中止后读取最后一份完整快照。
    std::ifstream input(report);
    const std::string xml(std::istreambuf_iterator<char>{input}, {});
    EXPECT_FALSE(reporter.hasIoError());
    EXPECT_NE(xml.find("tests=\"5\" failures=\"1\" skipped=\"2\" errors=\"1\""), std::string::npos);
    EXPECT_NE(xml.find("<error message=\"Execution interrupted"), std::string::npos);
    EXPECT_NE(xml.find("<skipped message=\"Not started"), std::string::npos);
    EXPECT_NE(xml.find("Manual-only test excluded"), std::string::npos);
    EXPECT_NE(xml.find("</testsuites>"), std::string::npos);
    input.close();
    mc::test::removeTestDir(dir);
}

TEST(GameTestTimeout, FinishedElapsedTimeStopsAdvancing)
{
    auto function = _timeoutFunction("elapsed");
    _TimeoutInstance instance(*function);
    instance.succeed();
    const auto elapsed = instance.wallTimeSeconds();
    EXPECT_GE(elapsed, 0.0);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_EQ(instance.wallTimeSeconds(), elapsed);
}

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
