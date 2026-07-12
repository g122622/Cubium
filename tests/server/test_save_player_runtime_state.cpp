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
// 2. 钩子被调用后，通过 fromPlayer() + savePlayer() 提取的玩家运行时状态
//    会经过 saveAllWorldData() → PlayerDataManager::saveAll() 落盘到 RocksDB
// 3. 重新打开存档后，loadPlayer(uuid) 可以读回之前在线玩家的数据
//
// 这些测试针对 IntegratedServer，但 StandaloneServer 的 stop() 流程完全一致
// （先 join 主循环线程，再调用 savePlayerRuntimeState，再 stopCore），因此
// 验证逻辑同样适用于独立服务器。
// ============================================================================

#include "common/network/connection/LocalConnection.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/application/IntegratedServer.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::server;
using namespace mc::network;
using namespace mc::world::storage;
using namespace mc;
using namespace std::chrono_literals;

namespace {

// ============================================================================
// 辅助：构造带 12 字节包头的 LoginRequestPacket
// 包头格式：u32 size | u16 type | u16 flags | u16 reserved | u16 padding
// ============================================================================
std::vector<u8> buildLoginRequestPacket(const std::string& username)
{
    LoginRequestPacket packet(username, protocol::VERSION);

    PacketSerializer payloadSer;
    packet.serialize(payloadSer);
    const auto& payload = payloadSer.buffer();

    PacketSerializer ser;
    ser.writeU32(static_cast<u32>(PACKET_HEADER_SIZE + payload.size())); // size
    ser.writeU16(static_cast<u16>(PacketType::LoginRequest));            // type
    ser.writeU16(0);                                                     // flags
    ser.writeU16(0);                                                     // reserved
    ser.writeU16(0);                                                     // padding
    ser.writeBytes(payload.data(), payload.size());

    return ser.buffer();
}

// ============================================================================
// 辅助：从收到的字节数据中解析 LoginResponsePacket
// 返回 (success, playerId, entityId, username)
// ============================================================================
struct ParsedLoginResponse {
    bool success = false;
    PlayerId playerId = 0;
    EntityId entityId = INVALID_ENTITY_ID;
    std::string username;
    bool parsed = false;
};

ParsedLoginResponse parseLoginResponse(const std::vector<u8>& data)
{
    ParsedLoginResponse result;
    if (data.size() < PACKET_HEADER_SIZE) {
        return result;
    }

    PacketDeserializer deser(data.data(), data.size());
    (void)deser.readU32();             // size
    auto typeResult = deser.readU16(); // type
    if (typeResult.failed()) {
        return result;
    }
    if (typeResult.value() != static_cast<u16>(PacketType::LoginResponse)) {
        return result;
    }
    (void)deser.readU16(); // flags
    (void)deser.readU16(); // reserved
    (void)deser.readU16(); // padding

    auto respResult = LoginResponsePacket::deserialize(deser);
    if (respResult.failed()) {
        return result;
    }

    const auto& resp = respResult.value();
    result.success = resp.success();
    result.playerId = resp.playerId();
    result.entityId = resp.entityId();
    result.username = resp.username();
    result.parsed = true;
    return result;
}

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

    // 等待接收一个数据包，超时返回 false
    bool waitForPacket(LocalEndpoint* endpoint, std::vector<u8>& outData, int timeoutMs = 3000)
    {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() <
            timeoutMs) {
            if (endpoint->receive(outData)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return false;
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
// 测试子类： spying savePlayerRuntimeState() 调用情况
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
// 测试 2：登录后 stop()，玩家数据通过 savePlayerRuntimeState() 落盘到 RocksDB
// 完整流程：
//   LoginRequestPacket → 服务端创建 Player 实体 → stop() →
//   savePlayerRuntimeState() 遍历玩家实体，调用 fromPlayer() + savePlayer() →
//   saveAllWorldData() 通过 PlayerDataManager::saveAll() 落盘 →
//   重新打开存档，loadPlayer(uuid) 验证数据
// ============================================================================
TEST_F(SavePlayerRuntimeStateTest, PlayerDataPersistsToRocksDBAfterStop)
{
    SaveStateSpyServer server;
    IntegratedServerParams config = makeConfig();

    auto initResult = server.initialize(config);
    ASSERT_TRUE(initResult.success()) << initResult.error().message();

    auto* clientEndpoint = server.getClientEndpoint();
    ASSERT_NE(clientEndpoint, nullptr);
    EXPECT_TRUE(clientEndpoint->isConnected());

    // 计算与 IntegratedServer::handleLoginRequestPacket 一致的离线 UUID
    const std::string username = "SpyPlayer";
    const Uuid offlineUuid = util::generateOfflineUuid(username);
    const std::string uuidStr = util::uuidToString(offlineUuid);

    // 发送 LoginRequestPacket
    auto packetData = buildLoginRequestPacket(username);
    ASSERT_FALSE(packetData.empty());
    clientEndpoint->send(packetData.data(), packetData.size());

    // 等待 LoginResponsePacket
    std::vector<u8> recvData;
    ASSERT_TRUE(waitForPacket(clientEndpoint, recvData, 5000)) << "Did not receive login response";

    auto parsed = parseLoginResponse(recvData);
    ASSERT_TRUE(parsed.parsed) << "Failed to parse LoginResponsePacket";
    EXPECT_TRUE(parsed.success) << "Login should succeed";
    EXPECT_EQ(parsed.username, username);
    EXPECT_NE(parsed.entityId, INVALID_ENTITY_ID);

    // 给服务端一点时间完成登录后的初始化（发送游戏状态、物品栏等）
    // 这些包不影响测试结果，但需要让服务端把玩家实体状态稳定下来
    std::this_thread::sleep_for(200ms);

    // 调用 stop()，触发 savePlayerRuntimeState() + saveAllWorldData()
    server.stop();

    // 验证钩子被调用
    EXPECT_TRUE(server.savePlayerRuntimeStateCalled.load());
    EXPECT_EQ(server.callCount.load(), 1u);

    // 重新打开存档，验证玩家数据已落盘到 RocksDB
    // 注意：stop() 后 sharedStorage() 返回 nullptr，必须用全新的 SingleLevelStorageManager
    std::filesystem::path worldPath = m_gameRoot / "saves" / m_worldName;
    ASSERT_TRUE(std::filesystem::exists(worldPath)) << "World directory should still exist";

    SingleLevelStorageManager reopenedStorage;
    SingleLevelStorageConfig storageConfig;
    storageConfig.consistencyMode = ConsistencyMode::Eventual;
    storageConfig.sectionCacheCapacity = 128;
    // 重新打开时不需要备份，加快测试
    storageConfig.enableBackup = false;

    auto openResult = reopenedStorage.open(worldPath, storageConfig);
    ASSERT_TRUE(openResult.success()) << "Failed to reopen world storage: " << openResult.error().message();
    EXPECT_TRUE(reopenedStorage.isOpen());

    // 加载玩家数据 — 这一步是从 RocksDB 读取（缓存为空）
    auto loadResult = reopenedStorage.loadPlayer(uuidStr);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().message();
    ASSERT_TRUE(loadResult.value().has_value()) << "Player data should have been persisted to RocksDB";

    const auto& savedData = loadResult.value().value();
    EXPECT_EQ(savedData.uuid, uuidStr);
    // username 是 fromPlayer() 从 Player 实体提取的字段，登录时构造的实体使用
    // handleLoginRequestPacket 传入的 username
    EXPECT_EQ(savedData.username, username);
    // 玩家默认游戏模式为 Survival
    EXPECT_EQ(savedData.gameMode, GameMode::Survival);

    // 验证 saveData.uuid 不是空字符串：savePlayerRuntimeState() 会用 PlayerManager
    // 中的权威 UUID（playerData->uuid）覆盖 fromPlayer() 提取的空 uuid，确保
    // 落盘 key 与登录查询 key 一致。如果该覆盖逻辑缺失，loadPlayer(uuidStr) 将
    // 返回空 optional，上面的断言会失败。
    EXPECT_FALSE(savedData.uuid.empty());

    reopenedStorage.close();
}

// ============================================================================
// 测试 3：无玩家时 stop() 不会在 RocksDB 中产生任何玩家记录
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
