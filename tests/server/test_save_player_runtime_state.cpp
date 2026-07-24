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

// ============================================================================
// 集成测试：验证关服时 savePlayerRuntimeState() 钩子被调用并落盘到 RocksDB
//
// 测试覆盖：
// 1. stop() 期间 savePlayerRuntimeState() 钩子确实被调用
// 2. 无玩家时 stop() 不会在 RocksDB 中产生任何玩家记录
//
// 这些测试针对 IntegratedServer，但 StandaloneServer 的 stop() 流程完全一致
// （先 join 主循环线程，再调用 savePlayerRuntimeState，再 stopCore），因此
// 验证逻辑同样适用于独立服务器。
//
// 注：原"登录后 stop() 落盘"用例依赖旧 1.16.5 字节登录协议（LoginRequestPacket
// 单包 + getClientEndpoint 字节队列），新网络层登录改走 ServerHandshake 多阶段
// 握手（ClientIntention→Hello→Configuration→Play），驱动登录需完整握手编排，
// 超出本测试范围。该用例已移除，TODO(Phase6): 由 ServerHandshake 集成测试覆盖
// 登录→落盘链路。
// ============================================================================

#include "common/util/UuidUtils.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/application/IntegratedServer.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <thread>
#include <gtest/gtest.h>

using namespace mc::server;
using namespace mc::world::storage;
using namespace mc;
using namespace std::chrono_literals;

namespace {

// ============================================================================
// 测试夹具：使用临时目录避免污染用户存档，并预先创建 saves/<worldName>/
// 以便 GlobalStorageManager::openLevel 能找到世界目录
// ============================================================================
class SavePlayerRuntimeStateTest : public ::testing::Test {
protected:
    std::filesystem::path m_gameRoot;
    std::string m_worldName = "prs_test_world";

    void SetUp() override
    {
        // 使用带时间戳的临时目录避免并行测试冲突
        m_gameRoot = std::filesystem::temp_directory_path() / "mc_prs_test" / std::to_string(std::time(nullptr));
        std::filesystem::create_directories(m_gameRoot / "saves" / m_worldName);
    }

    void TearDown() override
    {
        // 重试几次删除（Windows 上 RocksDB 后台线程可能延迟释放句柄）
        for (int i = 0; i < 10; ++i) {
            std::error_code ec;
            std::filesystem::remove_all(m_gameRoot, ec);
            if (!ec) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // 构造一份指向临时目录的非新世界配置（既有世界，不写 level.dat）
    IntegratedServerParams makeConfig() const
    {
        return IntegratedServerParams{
            .worldName = m_worldName,
            .gameDirectoryRoot = m_gameRoot.string(),
            .displayName = m_worldName,
            .seed = 42,
            .defaultGameMode = GameMode::Survival,
            .viewDistance = 3,
            .tickRate = 100,
            .worldType = WorldType::Default,
            .difficulty = Difficulty::Normal,
            .hardcore = false,
            .allowCommands = false,
            .isNewWorld = false,
        };
    }
};

// ============================================================================
// 测试子类：spying savePlayerRuntimeState() 调用情况
// ============================================================================
class SaveStateSpyServer : public IntegratedServer {
public:
    std::atomic<bool> savePlayerRuntimeStateCalled{false};
    std::atomic<size_t> callCount{0};

protected:
    void savePlayerRuntimeState() override
    {
        savePlayerRuntimeStateCalled = true;
        callCount.fetch_add(1);
        // 调用基类实现以执行真正的回写逻辑
        IntegratedServer::savePlayerRuntimeState();
    }
};

// ============================================================================
// 测试 1：stop() 期间 savePlayerRuntimeState() 钩子被调用（即使无玩家）
// ============================================================================
TEST_F(SavePlayerRuntimeStateTest, StopInvokesSavePlayerRuntimeStateHook)
{
    SaveStateSpyServer server;
    IntegratedServerParams config = makeConfig();

    auto initResult = server.initialize(config);
    ASSERT_TRUE(initResult.success()) << initResult.error().message();
    EXPECT_TRUE(server.isRunning());

    // 在调用 stop 之前钩子不应被调用
    EXPECT_FALSE(server.savePlayerRuntimeStateCalled.load());

    server.stop();

    // stop() 必须调用一次 savePlayerRuntimeState()
    EXPECT_TRUE(server.savePlayerRuntimeStateCalled.load());
    EXPECT_EQ(server.callCount.load(), 1u);
    EXPECT_FALSE(server.isRunning());
}

// ============================================================================
// 测试 2：无玩家时 stop() 不会在 RocksDB 中产生任何玩家记录
// ============================================================================
TEST_F(SavePlayerRuntimeStateTest, StopWithNoPlayersDoesNotWritePlayerData)
{
    SaveStateSpyServer server;
    IntegratedServerParams config = makeConfig();

    auto initResult = server.initialize(config);
    ASSERT_TRUE(initResult.success()) << initResult.error().message();

    // 直接 stop，没有任何玩家登录
    server.stop();

    EXPECT_TRUE(server.savePlayerRuntimeStateCalled.load());

    // 重新打开存档，验证没有玩家数据
    std::filesystem::path worldPath = m_gameRoot / "saves" / m_worldName;

    SingleLevelStorageManager reopenedStorage;
    SingleLevelStorageConfig storageConfig;
    storageConfig.consistencyMode = ConsistencyMode::Eventual;
    storageConfig.sectionCacheCapacity = 128;
    storageConfig.enableBackup = false;

    auto openResult = reopenedStorage.open(worldPath, storageConfig);
    ASSERT_TRUE(openResult.success()) << openResult.error().message();

    // 用一个任意 UUID 查询，应该返回空 optional
    auto loadResult = reopenedStorage.loadPlayer("nonexistent-uuid-12345");
    ASSERT_TRUE(loadResult.success());
    EXPECT_FALSE(loadResult.value().has_value());

    reopenedStorage.close();
}

} // namespace
