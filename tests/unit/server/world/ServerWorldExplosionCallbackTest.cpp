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

#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/ServerWorld.hpp"

#include <unordered_map>

using namespace mc;
using namespace mc::server;

/**
 * @brief ServerWorld::broadcastExplosion() 测试固件
 *
 * 验证 ServerWorld::broadcastExplosion 是否正确委托给 m_onBroadcastExplosion 回调，
 * 以及未注册回调时是否安全无操作（不崩溃）。
 *
 * 参考 ServerWorldCommandExecuteTest 的轻量固件模式：
 * 仅构造 ServerWorld（不初始化存储/区块管理器），因为 broadcastExplosion 不依赖这些子系统。
 */
class ServerWorldExplosionCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册表（ServerWorld 析构时可能访问方块表，需保证已初始化）
        VanillaBlocks::initialize();

        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        m_world = std::make_unique<ServerWorld>(config);
    }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<ServerWorld> m_world;
};

// ============================================================================
// 基本回调委托测试
// ============================================================================

TEST_F(ServerWorldExplosionCallbackTest, WithoutCallback_DoesNotCrash)
{
    // 未注册回调时调用 broadcastExplosion 应安全无操作
    std::vector<BlockPos> blocks = {BlockPos(0, 64, 0), BlockPos(1, 64, 0)};
    std::unordered_map<u64, Vector3> knockback = {{1ULL, Vector3(0.5f, 1.0f, 0.2f)}};

    EXPECT_NO_THROW(m_world->broadcastExplosion(Vector3(0.0f, 64.0f, 0.0f), 2.0f, blocks, knockback));
}

TEST_F(ServerWorldExplosionCallbackTest, WithCallback_ReceivesAllParameters)
{
    // 验证回调完整接收 position/strength/affectedBlocks/playerKnockback 四个参数
    Vector3 capturedPosition(0, 0, 0);
    f32 capturedStrength = 0.0f;
    std::vector<BlockPos> capturedBlocks;
    std::unordered_map<u64, Vector3> capturedKnockback;
    bool callbackInvoked = false;

    m_world->setOnBroadcastExplosion([&](const Vector3& position,
                                         f32 strength,
                                         const std::vector<BlockPos>& affectedBlocks,
                                         const std::unordered_map<u64, Vector3>& playerKnockback) {
        capturedPosition = position;
        capturedStrength = strength;
        capturedBlocks = affectedBlocks;
        capturedKnockback = playerKnockback;
        callbackInvoked = true;
    });

    Vector3 expectedPosition(100.5f, 64.0f, -200.25f);
    std::vector<BlockPos> expectedBlocks = {BlockPos(100, 64, -200), BlockPos(101, 65, -199)};
    std::unordered_map<u64, Vector3> expectedKnockback = {
        {1ULL, Vector3(0.5f, 1.0f, 0.2f)},
        {2ULL, Vector3(-0.3f, 0.8f, 0.1f)},
    };

    m_world->broadcastExplosion(expectedPosition, 4.0f, expectedBlocks, expectedKnockback);

    EXPECT_TRUE(callbackInvoked);
    EXPECT_FLOAT_EQ(capturedPosition.x, expectedPosition.x);
    EXPECT_FLOAT_EQ(capturedPosition.y, expectedPosition.y);
    EXPECT_FLOAT_EQ(capturedPosition.z, expectedPosition.z);
    EXPECT_FLOAT_EQ(capturedStrength, 4.0f);
    EXPECT_EQ(capturedBlocks.size(), expectedBlocks.size());
    EXPECT_EQ(capturedBlocks[0], expectedBlocks[0]);
    EXPECT_EQ(capturedBlocks[1], expectedBlocks[1]);
    EXPECT_EQ(capturedKnockback.size(), expectedKnockback.size());
    EXPECT_FLOAT_EQ(capturedKnockback.at(1ULL).x, 0.5f);
    EXPECT_FLOAT_EQ(capturedKnockback.at(1ULL).y, 1.0f);
    EXPECT_FLOAT_EQ(capturedKnockback.at(1ULL).z, 0.2f);
    EXPECT_FLOAT_EQ(capturedKnockback.at(2ULL).x, -0.3f);
    EXPECT_FLOAT_EQ(capturedKnockback.at(2ULL).y, 0.8f);
    EXPECT_FLOAT_EQ(capturedKnockback.at(2ULL).z, 0.1f);
}

TEST_F(ServerWorldExplosionCallbackTest, EmptyAffectedBlocks_PassedThrough)
{
    // 风弹等不破坏方块的爆炸会传入空 affectedBlocks，验证空列表正确透传
    std::vector<BlockPos> capturedBlocks;
    bool callbackInvoked = false;

    m_world->setOnBroadcastExplosion(
        [&](const Vector3&, f32, const std::vector<BlockPos>& affectedBlocks, const std::unordered_map<u64, Vector3>&) {
            capturedBlocks = affectedBlocks;
            callbackInvoked = true;
        });

    std::unordered_map<u64, Vector3> knockback = {{1ULL, Vector3(0.1f, 0.2f, 0.3f)}};
    m_world->broadcastExplosion(Vector3(0.0f, 64.0f, 0.0f), 1.2f, {}, knockback);

    EXPECT_TRUE(callbackInvoked);
    EXPECT_TRUE(capturedBlocks.empty());
}

TEST_F(ServerWorldExplosionCallbackTest, EmptyPlayerKnockback_PassedThrough)
{
    // 当爆炸范围内没有可被击退的玩家时（如全是怪物），playerKnockback 为空
    std::unordered_map<u64, Vector3> capturedKnockback;
    bool callbackInvoked = false;

    m_world->setOnBroadcastExplosion([&](const Vector3&,
                                         f32,
                                         const std::vector<BlockPos>&,
                                         const std::unordered_map<u64, Vector3>& playerKnockback) {
        capturedKnockback = playerKnockback;
        callbackInvoked = true;
    });

    std::vector<BlockPos> blocks = {BlockPos(0, 64, 0)};
    m_world->broadcastExplosion(Vector3(0.0f, 64.0f, 0.0f), 2.0f, blocks, {});

    EXPECT_TRUE(callbackInvoked);
    EXPECT_TRUE(capturedKnockback.empty());
}

TEST_F(ServerWorldExplosionCallbackTest, MultipleCalls_InvokeCallbackEachTime)
{
    // 验证回调可被重复调用（每次爆炸都应触发）
    u32 callCount = 0;

    m_world->setOnBroadcastExplosion(
        [&](const Vector3&, f32, const std::vector<BlockPos>&, const std::unordered_map<u64, Vector3>&) {
            ++callCount;
        });

    for (u32 i = 0; i < 3; ++i) {
        m_world->broadcastExplosion(Vector3(0.0f, 64.0f, 0.0f), 1.0f, {}, {});
    }

    EXPECT_EQ(callCount, 3u);
}

TEST_F(ServerWorldExplosionCallbackTest, ResetCallback_OverwritesPrevious)
{
    // setOnBroadcastExplosion 应覆盖旧回调（与 setOnExecuteCommand 等其他回调一致）
    u32 firstCallbackCount = 0;
    u32 secondCallbackCount = 0;

    m_world->setOnBroadcastExplosion(
        [&](const Vector3&, f32, const std::vector<BlockPos>&, const std::unordered_map<u64, Vector3>&) {
            ++firstCallbackCount;
        });
    m_world->broadcastExplosion(Vector3(0.0f, 64.0f, 0.0f), 1.0f, {}, {});

    m_world->setOnBroadcastExplosion(
        [&](const Vector3&, f32, const std::vector<BlockPos>&, const std::unordered_map<u64, Vector3>&) {
            ++secondCallbackCount;
        });
    m_world->broadcastExplosion(Vector3(0.0f, 64.0f, 0.0f), 1.0f, {}, {});

    EXPECT_EQ(firstCallbackCount, 1u);
    EXPECT_EQ(secondCallbackCount, 1u);
}
