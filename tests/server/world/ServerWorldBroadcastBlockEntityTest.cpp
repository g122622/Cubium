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

/**
 * @file ServerWorldBroadcastBlockEntityTest.cpp
 * @brief ServerWorld::broadcastBlockEntity 广播回调测试
 *
 * 测试 ServerWorld::broadcastBlockEntity 的回调触发行为：
 * - 注册回调后调用 broadcastBlockEntity 应触发回调并传入正确坐标
 * - 未注册回调时调用 broadcastBlockEntity 不应崩溃
 * - 多次调用应多次触发回调
 *
 * MinecraftServer::broadcastBlockEntityInRange 的距离过滤逻辑由集成测试覆盖
 * （需要完整的 MinecraftServer 实例和玩家管理器），本测试聚焦于
 * ServerWorld 层的回调触发契约。
 */

#include "common/TempDirHelper.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "world/block/BlockPos.hpp"

#include <filesystem>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::server;

class ServerWorldBroadcastBlockEntityTest : public ::testing::Test {
protected:
    std::unique_ptr<ServerWorld> createTestWorld(const ServerWorldConfig& config)
    {
        auto world = std::make_unique<ServerWorld>(config);
        world->setSharedStorage(&m_storage);
        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));
        return world;
    }

    void SetUp() override
    {
        VanillaBlocks::initialize();

        // PID + 纳秒时间戳保证 CTest -j16 跨进程唯一，避免同秒 token 碰撞
        m_testDir = mc::test::makeUniqueTestDir("mc_server_world_broadcast_be_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        ServerWorldConfig config;
        config.viewDistance = 3;
        config.dimension = 0;
        config.seed = 12345;
        world = createTestWorld(config);
    }

    void TearDown() override
    {
        world.reset();
        m_storage.close();
        // RocksDB 后台线程可能延迟释放文件句柄，helper 内置 10 次重试覆盖句柄释放窗口
        mc::test::removeTestDir(m_testDir);
    }

    std::unique_ptr<ServerWorld> world;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// ========== broadcastBlockEntity 回调触发测试 ==========

TEST_F(ServerWorldBroadcastBlockEntityTest, BroadcastBlockEntityInvokesCallback)
{
    bool callbackInvoked = false;
    BlockPos capturedPos(0, 0, 0);
    BlockPos expectedPos(100, 64, -200);

    world->setOnBroadcastBlockEntity([&callbackInvoked, &capturedPos](const BlockPos& pos) {
        callbackInvoked = true;
        capturedPos = pos;
    });

    world->broadcastBlockEntity(expectedPos);

    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(capturedPos, expectedPos);
}

TEST_F(ServerWorldBroadcastBlockEntityTest, BroadcastBlockEntityWithoutCallbackDoesNotCrash)
{
    // 未注册回调时调用不应崩溃
    EXPECT_NO_THROW(world->broadcastBlockEntity(BlockPos(0, 0, 0)));
}

TEST_F(ServerWorldBroadcastBlockEntityTest, BroadcastBlockEntityInvokesCallbackMultipleTimes)
{
    int invokeCount = 0;
    world->setOnBroadcastBlockEntity([&invokeCount](const BlockPos& pos) {
        (void)pos;
        ++invokeCount;
    });

    world->broadcastBlockEntity(BlockPos(1, 2, 3));
    world->broadcastBlockEntity(BlockPos(4, 5, 6));
    world->broadcastBlockEntity(BlockPos(7, 8, 9));

    EXPECT_EQ(invokeCount, 3);
}

TEST_F(ServerWorldBroadcastBlockEntityTest, BroadcastBlockEntityPassesCorrectCoordinates)
{
    // 验证负坐标和边界值的正确传递
    BlockPos testCases[] = {
        BlockPos(0, 0, 0),
        BlockPos(-1, -64, -1),
        BlockPos(1000000, 255, -1000000),
        BlockPos(-1000000, -64, 1000000),
    };

    for (const auto& expectedPos : testCases) {
        BlockPos capturedPos(0, 0, 0);
        world->setOnBroadcastBlockEntity([&capturedPos](const BlockPos& pos) { capturedPos = pos; });

        world->broadcastBlockEntity(expectedPos);
        EXPECT_EQ(capturedPos, expectedPos);
    }
}

TEST_F(ServerWorldBroadcastBlockEntityTest, SetOnBroadcastBlockEntityOverwritesPreviousCallback)
{
    int firstCount = 0;
    int secondCount = 0;

    world->setOnBroadcastBlockEntity([&firstCount](const BlockPos& pos) {
        (void)pos;
        ++firstCount;
    });
    world->setOnBroadcastBlockEntity([&secondCount](const BlockPos& pos) {
        (void)pos;
        ++secondCount;
    });

    world->broadcastBlockEntity(BlockPos(0, 0, 0));

    EXPECT_EQ(firstCount, 0);
    EXPECT_EQ(secondCount, 1);
}
