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
 * LIABILITY,WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file SpawnEggAddEntityE2ETest.cpp
 * @brief 刷怪蛋右键方块 → 服务端生成实体 → 发出 AddEntity 包 端到端集成测试
 *
 * 验证修复 SpawnEggItem::spawnEntity 反查 EntityRegistry 真实工厂后，服务端连接
 * Java 原版客户端时右键刷怪蛋能正确生成实体并向客户端下发 AddEntity 包。
 *
 * 覆盖真实链路（区别于单元测试的桩世界）：
 *   BlockInteractionManager::handleItemUseOn
 *     → SpawnEggItem::onItemUse
 *       → SpawnEggItem::spawnEntity（反查 EntityRegistry）
 *         → ServerWorld::spawnEntity
 *           → EntityTracker::notifyEntityTracked
 *             → EntityTracker::_sendSpawnPacket
 *               → ServerPlayerData::send
 *                 → CapturingConnection::send（捕获 IrPacket）
 *
 * 装配范式参考 BlockInteractionManagerPlacementTest，关键补充：
 * - world->setServer(&testServer)：启用 ServerWorld::spawnEntity 的 notifyEntityTracked 广播
 *   （m_server 为空时只登记不发包，见 ServerWorld.cpp spawnEntity 注释）
 * - CapturingConnection：实现 IServerClientConnection，记录出站 IrPacket 内容（FakeServerConnection
 *   只记字节数不记包内容，无法断言 AddEntity 字段），从捕获的 IrPacket 取出 AddEntity 断言
 *
 * 断言点（修复前全部失败：spawnEntity 返回 false，无实体生成，无 AddEntity 包）：
 * - handleItemUseOn 返回 success
 * - 捕获到至少一个 AddEntity 包
 * - AddEntity.entityTypeId == 100（minecraft:pig 的 vanilla entity_type 注册表 id）
 * - AddEntity 位置 ≈ 刷怪蛋生成位置（方块上方中心）
 */

#include <gtest/gtest.h>

#include "server/interaction/BlockInteractionManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/network/IServerClientConnection.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/BaseTestServer.hpp"
#include "common/TempDirHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/items/special/SpawnEggItem.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/util/Direction.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/entity/JavaEntityTypeIdMap.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <filesystem>
#include <variant>
#include <vector>

using namespace mc;

namespace {

// ============================================================================
// 捕获连接：记录出站 IrPacket 全量内容，供测试断言 AddEntity 字段
//
// FakeServerConnection 只记录字节数（命令测试仅需断言"发过包"），无法还原包内容。
// 本测试需要断言 AddEntity.entityTypeId/位置等字段，故新建独立捕获连接。
// 所有权由 shared_ptr 持有，addPlayer 仅存裸指针（与 BaseTestServer::addTestPlayer 一致）。
// ============================================================================
class CapturingConnection final : public mc::server::net::IServerClientConnection {
public:
    [[nodiscard]] mc::Result<void> send(mc::network::ir::IrPacket packet) override
    {
        m_packets.push_back(std::move(packet));
        return mc::Result<void>::ok();
    }
    void close() override { m_connected = false; }
    [[nodiscard]] bool isConnected() const noexcept override { return m_connected; }
    void disconnect(const std::string& reason) override
    {
        m_disconnectReason = reason;
        m_connected = false;
    }
    [[nodiscard]] std::string peerAddress() const override { return {}; }

    [[nodiscard]] const std::vector<mc::network::ir::IrPacket>& packets() const noexcept { return m_packets; }
    [[nodiscard]] const std::string& disconnectReason() const noexcept { return m_disconnectReason; }

private:
    bool m_connected = true;
    std::string m_disconnectReason;
    std::vector<mc::network::ir::IrPacket> m_packets;
};

// ============================================================================
// 测试服务端：继承 BaseTestServer，override getPlayerWorld 返回装配的 ServerWorld，
// 并提供 playerEntityManager（BaseTestServer 默认桩抛异常，而 _getPlayerEntity 经
// noexcept 路径调用本接口，抛异常会触发 std::terminate）。范式同 BlockInteractionTestServer。
// ============================================================================
class SpawnEggE2ETestServer final : public mc::test::BaseTestServer {
public:
    explicit SpawnEggE2ETestServer(server::ServerWorld& world) { setPlayerWorld(&world); }

    [[nodiscard]] server::ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const server::ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

private:
    server::ServerPlayerEntityManager m_playerEntityManager;
};

// 从捕获的 IrPacket 列表中提取首个 AddEntity 包指针（未找到返回 nullptr）
const mc::network::ir::play::AddEntity* findAddEntity(const std::vector<mc::network::ir::IrPacket>& packets)
{
    using namespace mc::network::ir;
    for (const IrPacket& pkt : packets) {
        if (pkt.phase != mc::network::protocol::ConnectionProtocol::Play) {
            continue;
        }
        const PlayPacket& play = std::get<PlayPacket>(pkt.packet);
        if (std::holds_alternative<play::AddEntity>(play)) {
            return &std::get<play::AddEntity>(play);
        }
    }
    return nullptr;
}

// 统计捕获的 AddEntity 包数量
size_t countAddEntity(const std::vector<mc::network::ir::IrPacket>& packets)
{
    using namespace mc::network::ir;
    size_t n = 0;
    for (const IrPacket& pkt : packets) {
        if (pkt.phase != mc::network::protocol::ConnectionProtocol::Play) {
            continue;
        }
        const PlayPacket& play = std::get<PlayPacket>(pkt.packet);
        if (std::holds_alternative<play::AddEntity>(play)) {
            ++n;
        }
    }
    return n;
}

class SpawnEggAddEntityE2ETest : public ::testing::Test {
protected:
    static constexpr PlayerId m_playerId = 1;

    static void SetUpTestSuite()
    {
        // 真实注册表初始化：SpawnEggItem::spawnEntity 反查 EntityRegistry 真实工厂，
        // 必须先注册 vanilla 实体类型与物品。BlockItemRegistry 用于 block-item 映射。
        static bool s_initialized = false;
        if (!s_initialized) {
            VanillaBlocks::initialize();
            entity::VanillaEntities::registerAll();
            Items::initialize();
            BlockItemRegistry::instance().initializeVanillaBlockItems();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        // 跨进程唯一目录，避免 CTest -j16 同秒目录撞车
        m_testDir = mc::test::makeUniqueTestDir("mc_spawn_egg_e2e_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        server::ServerWorldConfig config;
        config.viewDistance = 8;
        config.dimension = 0;
        config.seed = 114514;

        m_world = std::make_unique<server::ServerWorld>(config);
        m_world->setSharedStorage(&m_storage);
        // 装配区块管理器（ServerWorld::initialize 要求 m_chunkManager != nullptr）
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<server::ServerChunkManager>(*m_world, std::move(generator));
        m_world->setChunkManager(std::move(chunkManager));

        auto worldInit = m_world->initialize();
        ASSERT_TRUE(worldInit.success());

        m_server = std::make_unique<SpawnEggE2ETestServer>(*m_world);
        // 关键：注入 IServer 到 ServerWorld，启用 spawnEntity 的 notifyEntityTracked 广播
        // （m_server 为空时 spawnEntity 只登记实体不发包）
        m_world->setServer(m_server.get());
        // ServerWorld::difficulty() 经 m_difficultyCallback 获取（无回调返回 Normal），
        // 注入回调委托到 test server 的难度，使 setDifficulty(Peaceful) 能被 spawnEntity 的
        // 和平难度检查（对齐 Java isAllowedInPeaceful）正确感知
        m_world->setDifficultyCallback([this]() { return m_server->difficulty(); });

        // 玩家必须注册到 m_server->playerManager()（而非独立实例）：
        // EntityTracker::notifyEntityTracked 经 server.playerManager().getPlayerIds() 遍历观察者，
        // 若 PlayerManager 不一致，观察者列表为空，spawnEntity 不会向任何玩家下发 AddEntity。
        m_connection = std::make_shared<CapturingConnection>();
        m_player = m_server->playerManager().addPlayer(m_playerId,
            mc::util::uuidToString(mc::util::generateOfflineUuid("SpawnEggTester")),
            "SpawnEggTester",
            m_connection.get());
        ASSERT_NE(m_player, nullptr);
        // 玩家站在目标方块旁，满足 _canInteract（6 格内）与 _shouldTrack（160 格内）
        m_player->x = 0.5f;
        m_player->y = 64.0f;
        m_player->z = 0.5f;
        m_player->yaw = 0.0f;
        m_player->pitch = 0.0f;
        m_player->gameMode = GameMode::Survival;
        m_player->loggedIn = true;

        // 复用 m_server 的 InventoryManager（同样基于 m_server->playerManager()）
        m_server->inventoryManager().initializeInventory(m_playerId);

        // BlockInteractionManager 必须绑定 m_server->playerManager()，使 _validatePlayer/_canInteract
        // 与 EntityTracker 的观察者遍历使用同一份玩家数据
        m_blockInteractionManager = std::make_unique<server::interaction::BlockInteractionManager>(
            m_server->playerManager(), m_lootTableManager);
        m_blockInteractionManager->setInventoryManager(&m_server->inventoryManager());
        m_blockInteractionManager->setServer(m_server.get());
    }

    void TearDown() override
    {
        m_blockInteractionManager.reset();

        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
        m_storage.close();
        mc::test::removeTestDir(m_testDir);
    }

    // 把刷怪蛋注入主手槽位
    void setHeldSpawnEgg(const Item* item, i32 count)
    {
        ASSERT_NE(item, nullptr);
        PlayerInventory* inventory = m_server->inventoryManager().getInventory(m_playerId);
        ASSERT_NE(inventory, nullptr);
        inventory->setSelectedSlot(0);
        inventory->setItem(0, ItemStack(*item, count));
    }

    [[nodiscard]] ItemStack heldItem() const { return m_server->inventoryManager().getHeldItem(m_playerId); }

    std::unique_ptr<server::ServerWorld> m_world;
    std::unique_ptr<SpawnEggE2ETestServer> m_server;
    std::unique_ptr<server::interaction::BlockInteractionManager> m_blockInteractionManager;
    std::shared_ptr<CapturingConnection> m_connection;

    loot::LootTableManager m_lootTableManager;
    server::ServerPlayerData* m_player = nullptr;

    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// ============================================================================
// 端到端：右键石头顶面放猪刷怪蛋 → 服务端生成 pig → 客户端收到 AddEntity(pig)
// ============================================================================
TEST_F(SpawnEggAddEntityE2ETest, UseItemOnSpawnEggSendsAddEntityToClient)
{
    // 目标方块：石头，玩家站在其旁
    constexpr i32 BLOCK_X = 0;
    constexpr i32 BLOCK_Y = 63;
    constexpr i32 BLOCK_Z = 0;
    m_world->setBlockState(BLOCK_X, BLOCK_Y, BLOCK_Z, &VanillaBlocks::STONE->defaultState());
    // 清空方块上方一格（避免地形生成的方块阻塞生成位置）
    m_world->setBlockState(BLOCK_X, BLOCK_Y + 1, BLOCK_Z, &VanillaBlocks::AIR->defaultState());

    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    setHeldSpawnEgg(Items::PIG_SPAWN_EGG, 4);

    // 右键石头顶面
    const BlockPos targetPos(BLOCK_X, BLOCK_Y, BLOCK_Z);
    const Vector3 hitPos(0.5f, static_cast<f32>(BLOCK_Y) + 0.99f, 0.5f);
    auto result = m_blockInteractionManager->handleItemUseOn(
        m_playerId, targetPos, hitPos, Direction::Up, Hand::MainHand, heldItem());

    // handleItemUseOn 应成功（SpawnEggItem::onItemUse 返回 Success）
    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(result.value().success) << "onItemUse should succeed";

    // 生存模式应消耗 1 个刷怪蛋
    EXPECT_EQ(m_server->inventoryManager().getHeldItem(m_playerId).getCount(), 3);

    // 核心：客户端应收到 AddEntity 包（修复前 spawnEntity 返回 false，无任何包发出）
    const auto& packets = m_connection->packets();
    EXPECT_GE(countAddEntity(packets), 1u) << "server must send AddEntity for spawned pig";

    const auto* addEntity = findAddEntity(packets);
    ASSERT_NE(addEntity, nullptr);

    // entityTypeId 必须是 vanilla 1.21.11 的 pig 注册表 id（100），Java 客户端据此 spawn pig
    // getJavaEntityTypeId 按 "minecraft:pig" 查 JavaEntityTypeIdMap，未初始化时自动 initialize
    const u32 expectedPigTypeId = JavaEntityTypeIdMap::instance().toJavaRegistryId("minecraft:pig");
    ASSERT_EQ(expectedPigTypeId, 100u) << "vanilla pig entity_type id must be 100";
    EXPECT_EQ(addEntity->entityTypeId, static_cast<i32>(expectedPigTypeId))
        << "AddEntity.entityTypeId must be vanilla pig id (100), got " << addEntity->entityTypeId;

    // 位置应在方块上方中心（spawnPos = pos.offset(Up) = (0,64,0)，spawnEntity 内 pos.x+0.5/pos.y/pos.z+0.5）
    EXPECT_NEAR(addEntity->x, 0.5, 1e-6);
    EXPECT_NEAR(addEntity->y, static_cast<f64>(BLOCK_Y + 1), 1e-6);
    EXPECT_NEAR(addEntity->z, 0.5, 1e-6);

    // UUID 应为 16 字节（uuidFromString 解析结果）
    EXPECT_EQ(addEntity->uuid.size(), 16u);

    // entityId 应为服务端分配的有效 id（spawnEntity 内 setId）
    EXPECT_GT(addEntity->entityId, 0);
}

// ============================================================================
// 端到端：和平难度下僵尸刷怪蛋不应生成实体、不应发出 AddEntity 包
// （对齐 Java isAllowedInPeaceful：怪物类在和平难度不生成）
// ============================================================================
TEST_F(SpawnEggAddEntityE2ETest, PeacefulDifficultyMonsterEggSendsNoAddEntity)
{
    m_server->setDifficulty(Difficulty::Peaceful);

    constexpr i32 BLOCK_X = 0;
    constexpr i32 BLOCK_Y = 63;
    constexpr i32 BLOCK_Z = 0;
    m_world->setBlockState(BLOCK_X, BLOCK_Y, BLOCK_Z, &VanillaBlocks::STONE->defaultState());
    m_world->setBlockState(BLOCK_X, BLOCK_Y + 1, BLOCK_Z, &VanillaBlocks::AIR->defaultState());

    ASSERT_NE(Items::ZOMBIE_SPAWN_EGG, nullptr);
    setHeldSpawnEgg(Items::ZOMBIE_SPAWN_EGG, 4);

    const BlockPos targetPos(BLOCK_X, BLOCK_Y, BLOCK_Z);
    const Vector3 hitPos(0.5f, static_cast<f32>(BLOCK_Y) + 0.99f, 0.5f);
    auto result = m_blockInteractionManager->handleItemUseOn(
        m_playerId, targetPos, hitPos, Direction::Up, Hand::MainHand, heldItem());

    // SpawnEggItem::onItemUse 在 spawnEntity 返回 false 时返回 Fail
    EXPECT_FALSE(result.value().success) << "zombie egg in peaceful should not succeed";

    // 和平难度怪物不生成：无 AddEntity 包
    EXPECT_EQ(countAddEntity(m_connection->packets()), 0u) << "no AddEntity in peaceful for monster egg";

    // 不消耗刷怪蛋（spawn 失败）
    EXPECT_EQ(m_server->inventoryManager().getHeldItem(m_playerId).getCount(), 4);
}

// ============================================================================
// 端到端：创造模式右键刷怪蛋生成实体但不消耗
// ============================================================================
TEST_F(SpawnEggAddEntityE2ETest, CreativeModeSpawnsEntityWithoutConsuming)
{
    m_player->gameMode = GameMode::Creative;

    constexpr i32 BLOCK_X = 0;
    constexpr i32 BLOCK_Y = 63;
    constexpr i32 BLOCK_Z = 0;
    m_world->setBlockState(BLOCK_X, BLOCK_Y, BLOCK_Z, &VanillaBlocks::STONE->defaultState());
    m_world->setBlockState(BLOCK_X, BLOCK_Y + 1, BLOCK_Z, &VanillaBlocks::AIR->defaultState());

    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    setHeldSpawnEgg(Items::PIG_SPAWN_EGG, 4);

    const BlockPos targetPos(BLOCK_X, BLOCK_Y, BLOCK_Z);
    const Vector3 hitPos(0.5f, static_cast<f32>(BLOCK_Y) + 0.99f, 0.5f);
    auto result = m_blockInteractionManager->handleItemUseOn(
        m_playerId, targetPos, hitPos, Direction::Up, Hand::MainHand, heldItem());

    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(result.value().success);

    // 创造模式不消耗
    EXPECT_EQ(m_server->inventoryManager().getHeldItem(m_playerId).getCount(), 4);

    // 仍应发出 AddEntity 包
    EXPECT_GE(countAddEntity(m_connection->packets()), 1u);
}

// ============================================================================
// 端到端：未知实体类型刷怪蛋（反查 EntityRegistry 失败）不应生成、不应发包
//
// 直接在真实 ServerWorld 上调 SpawnEggItem::spawnEntity 验证反查失败路径
// （不经 InventoryManager：未注册物品无法经物品栏注入，会触发 ItemRegistry 查询失败）。
// ============================================================================
TEST_F(SpawnEggAddEntityE2ETest, UnknownEntityTypeEggSendsNoAddEntity)
{
    // 构造一个 name 不在 EntityRegistry 中的刷怪蛋：spawnEntity 反查失败返回 false
    auto unknownType = entity::EntityType::Builder(
        [](mc::IWorld*, mc::ecs::EntityRegistry&) -> std::unique_ptr<mc::Entity> { return nullptr; },
        entity::EntityClassification::Creature)
                           .build();
    const_cast<std::string&>(unknownType.name()) = "minecraft:nonexistent_entity";

    mc::item::SpawnEggItem unknownEgg(
        std::move(unknownType), 0xFFFFFF, 0x000000, mc::ItemProperties().maxStackSize(64));

    const BlockPos spawnPos(0, 64, 0);
    // 反查 EntityRegistry 失败 → spawnEntity 返回 false，不生成实体、不下发 AddEntity
    EXPECT_FALSE(unknownEgg.spawnEntity(*m_world, spawnPos, world::spawn::SpawnReason::SpawnEgg));
    EXPECT_EQ(countAddEntity(m_connection->packets()), 0u);
}

} // namespace
