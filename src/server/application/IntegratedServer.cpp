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

#include "IntegratedServer.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/inventory/container/CrafterContainer.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/packet/BlockBreakAnimPacket.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/blockentity/trial/CrafterBlockEntity.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/TimeManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/util/assert/AssertAll.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::server {

namespace {

/**
 * @brief 获取菜单玩家
 * TODO: 这是临时方案，后续需要优化容器系统的玩家上下文处理
 */
Player& _getMenuPlayer()
{
    static Player player(0, "IntegratedServerMenu");
    return player;
}

} // namespace

IntegratedServer::IntegratedServer()
    : MinecraftServer(m_integratedSettings)
{}

IntegratedServer::~IntegratedServer()
{
    if (m_initialized) {
        stop();
    }
}

Result<void> IntegratedServer::initialize()
{
    IntegratedServerParams params;
    params.allowCommands = false;
    return initialize(params);
}

Result<void> IntegratedServer::initialize(const IntegratedServerParams& params)
{
    MC_TRACE_EVENT("server.initialization", "IntegratedServer::initialize");

    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Server already initialized");
    }

    // 保存集成服务器参数
    m_params = params;

    // 将参数应用到设置
    m_integratedSettings.viewDistance.set(params.viewDistance);
    m_integratedSettings.defaultGameMode.set(static_cast<i32>(params.defaultGameMode));
    m_integratedSettings.levelSeed.set(params.seed != 0 ? std::to_string(params.seed) : "");
    m_integratedSettings.maxPlayers.set(1); // 内置服务器只支持单人
    m_integratedSettings.tickRate.set(params.tickRate);
    m_integratedSettings.worldName.set(params.worldName);

    // 初始化游戏目录并扫描数据包
    m_gameDirectory = params.gameDirectoryRoot.empty() ? GameDirectory::defaultDirectory()
                                                       : GameDirectory::fromRoot(params.gameDirectoryRoot);
    auto dirResult = m_gameDirectory.ensureDirectoriesExist();
    if (dirResult.failed()) {
        spdlog::warn("Failed to create game directories: {}", dirResult.error().toString());
    }

    auto dataPackDir = m_gameDirectory.dataPacksDir();
    auto scanResult = m_dataPackList.scanDirectory(dataPackDir);
    if (scanResult.failed()) {
        spdlog::warn(
            "Failed to scan data pack directory '{}': {}", dataPackDir.string(), scanResult.error().toString());
    } else if (scanResult.value() > 0) {
        spdlog::info("Scanned {} data packs from '{}'", scanResult.value(), dataPackDir.string());
    }

    // 初始化游戏注册表
    initializeRegistries(false);

    spdlog::info("Initializing integrated server...");
    spdlog::info("World: {}, Seed: {}, View distance: {}", params.worldName, params.seed, params.viewDistance);

    // 创建本地连接对
    m_connectionPair = std::make_unique<network::LocalConnectionPair>();
    m_connectionPair->connect();
    m_serverEndpoint = &m_connectionPair->serverEndpoint();

    // 初始化核心管理器
    initializeCoreManagers();

    // 加载 OP 列表（集成服务器使用默认路径）
    // 白名单和封禁列表在集成服务器中通常不需要
    auto opsResult = m_opListManager->load("ops.json");
    if (opsResult.failed()) {
        spdlog::error("No ops.json found or failed to load: {}", opsResult.error().message());
    }

    auto storageInitResult = initializeSharedStorage(m_gameDirectory, params.worldName);
    if (storageInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to initialize shared world storage: " + storageInitResult.error().message());
    }

    // 初始化维度管理器
    auto dimInitResult =
        m_dimensionManager->initialize(static_cast<u64>(params.seed), params.viewDistance, params.worldType);
    if (dimInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to initialize dimension manager: " + dimInitResult.error().message());
    }

    auto* overworld = m_dimensionManager->getOverworld();
    MC_ASSERT_RELEASE(overworld != nullptr);
    MC_ASSERT_RELEASE(overworld->world() != nullptr);

    // 为所有维度绑定世界回调和命令绑定
    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (world) {
            attachWorldBindings(*world);
            attachWorldCommandBindings(*world);
        }
    });

    // 设置命令执行回调（用于命令方块矿车等实体执行命令）
    // 为所有维度设置命令执行回调
    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (world) {
            world->setOnExecuteCommand(
                [this, world](const std::string& command, const Vector3d& position, i32 permissionLevel) -> i32 {
                    std::string cmd = command;
                    if (!cmd.empty() && cmd[0] != '/') {
                        cmd = "/" + cmd;
                    }

                    command::ServerCommandSource source(
                        this, nullptr, world->dimension(), position, Vector2f(0.0f, 0.0f), permissionLevel, 0, "@");
                    auto result = m_commandRegistry->execute(cmd, source);
                    if (result.failed()) {
                        spdlog::info("Command execution failed for '{}': {}", cmd, result.error().message());
                        return 0;
                    }

                    return result.value();
                });
        }
    });

    auto worldResult = initializeWorld();
    if (worldResult.failed()) {
        return worldResult;
    }

    if (overworld->world()->isDebugWorld()) {
        spdlog::info("Configuring debug world special settings...");
        m_settings.defaultGameMode.set(static_cast<i32>(GameMode::Spectator));
        if (m_timeManager) {
            m_timeManager->setDayTime(6000);
            m_timeManager->setDaylightCycleEnabled(false);
            spdlog::info("Debug world: Time set to noon (6000), daylight cycle disabled");
        }
        if (overworld->world()->weatherManager()) {
            overworld->world()->weatherManager()->setClear(999999999);
            spdlog::info("Debug world: Weather set to clear");
        }
    }

    setupRaidManagerCallbacks();
    setupDragonFightBossBar();
    initializeInteractionManagers();

    // 初始化同步管理器
    initializeSyncManagers();

    // 初始化区块同步管理器（需要 world 已初始化）
    initializeChunkSyncManagers();

    // 设置世界回调（包括光照变化回调）
    setupWorldCallbacks();

    // 启动服务端线程
    m_running = true;
    m_serverThread = std::make_unique<std::thread>([this]() { _mainLoop(); });

    m_initialized = true;
    spdlog::info("Integrated server initialized");
    return Result<void>::ok();
}

void IntegratedServer::shutdown()
{
    stop();
}

void IntegratedServer::savePlayerRuntimeState()
{
    MC_TRACE_EVENT("server.initialization", "IntegratedServer::savePlayerRuntimeState");

    // 外来只读存档不写盘，直接跳过
    if (isSharedStorageReadonlyForeignWorld()) {
        return;
    }

    auto* storage = sharedStorage();
    if (storage == nullptr || !storage->isOpen()) {
        return;
    }
    auto* pdm = storage->playerDataManager();
    if (pdm == nullptr) {
        return;
    }

    // 遍历所有维度，对每个在线 Player 实体回写运行时状态到 PlayerDataManager 缓存
    // 注意：必须在 clearAll 之前调用，否则玩家实体已被从 EntityManager 移除
    size_t savedCount = 0;
    m_dimensionManager->forEachDimension([&](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (world == nullptr) {
            return;
        }

        const auto playerIds = m_playerEntityManager.getPlayerIds();
        for (PlayerId playerId : playerIds) {
            Player* player = m_playerEntityManager.getPlayerEntity(playerId, *world);
            if (player == nullptr) {
                continue;
            }

            // fromPlayer() 提取位置、生命、饥饿、经验、背包、效果等运行时状态
            // savePlayer() 同时更新缓存并标记脏，后续 saveAllWorldData() 会落盘
            auto saveData = world::storage::PlayerDataManager::fromPlayer(*player);

            // Player 实体的 m_uuid 由登录流程（handleLoginRequestPacket）计算离线
            // UUID 后存入 ServerPlayerData，但未回写到实体本身。这里用 PlayerManager
            // 中的权威 UUID 覆盖 saveData.uuid，避免以空字符串作为键落盘导致数据丢失。
            if (auto* playerData = m_playerManager->getPlayer(playerId)) {
                if (!playerData->uuid.empty()) {
                    saveData.uuid = playerData->uuid;
                }
            }

            auto result = pdm->savePlayer(saveData);
            if (result.success()) {
                ++savedCount;
            } else {
                spdlog::warn(
                    "Failed to save player runtime state for playerId={}: {}", playerId, result.error().message());
            }
        }
    });

    if (savedCount > 0) {
        spdlog::info("Saved runtime state for {} player(s) before shutdown", savedCount);
    }
}

void IntegratedServer::requestStop()
{
    MinecraftServer::requestStop();

    if (m_connectionPair) {
        m_connectionPair->disconnect();
    }
}

void IntegratedServer::stop()
{
    // IntegratedServer 停止流程整体区间。stopCore()/shutdownManagers()/saveAllWorldData()
    // 等子阶段在各自函数内部有独立 trace，形成 slice 嵌套。
    // 注意：集成服运行在客户端进程内，trace 会话生命周期由客户端统一管理，
    // 本函数不关闭 Perfetto（与 StandaloneServer::stop() 不同）。
    MC_TRACE_EVENT("server.initialization", "IntegratedServer::stop");

    if (!m_initialized) {
        return;
    }

    spdlog::info("Stopping integrated server...");
    m_running = false;

    // 断开连接
    if (m_connectionPair) {
        m_connectionPair->disconnect();
    }

    // 等待服务端线程结束
    {
        MC_TRACE_EVENT("server.initialization", "IntegratedServer::stop::JoinServerThread");
        if (m_serverThread && m_serverThread->joinable()) {
            m_serverThread->join();
        }
        m_serverThread.reset();
    }

    // 回写在线玩家运行时状态到 PlayerDataManager 缓存
    // 必须在 clearAll() 之前调用，否则玩家实体已被从 EntityManager 移除
    // saveAllWorldData() 后续会通过 PlayerDataManager::saveAll() 把缓存落盘
    savePlayerRuntimeState();

    // 清理玩家实体（遍历所有维度）
    {
        MC_TRACE_EVENT("server.initialization", "IntegratedServer::stop::ClearPlayerEntities");
        m_dimensionManager->forEachDimension([this](Dimension& dim) {
            auto* serverDim = static_cast<ServerDimension*>(&dim);
            auto* world = serverDim->world();
            if (world) {
                m_playerEntityManager.clearAll(*world);
            }
        });
    }

    // 停止核心组件
    stopCore();

    // 释放客户端连接
    m_clientConnection.reset();

    // 关闭连接对
    if (m_connectionPair) {
        m_connectionPair.reset();
    }
    m_serverEndpoint = nullptr;
    m_clientPlayerId = 0;
    m_clientEntityId = INVALID_ENTITY_ID;
    m_initialized = false;

    spdlog::info("Integrated server stopped");
}

network::LocalEndpoint* IntegratedServer::getClientEndpoint()
{
    if (m_connectionPair) {
        return &m_connectionPair->clientEndpoint();
    }
    return nullptr;
}

PlayerInventory* IntegratedServer::playerInventory(PlayerId playerId)
{
    if (playerId == m_clientPlayerId) {
        return &m_clientInventory;
    }

    return MinecraftServer::playerInventory(playerId);
}

const PlayerInventory* IntegratedServer::playerInventory(PlayerId playerId) const
{
    if (playerId == m_clientPlayerId) {
        return &m_clientInventory;
    }

    return MinecraftServer::playerInventory(playerId);
}

void IntegratedServer::_mainLoop()
{
    using clock = std::chrono::steady_clock;
    const auto tickDuration = std::chrono::milliseconds(1000 / m_settings.tickRate.get());

    mc::perfetto::PerfettoManager::instance().setThreadName("IntegratedServerThread");

    spdlog::info("Integrated server started ({} TPS)", m_settings.tickRate.get());

    // 平滑 tick 耗时的指数移动平均因子（与 MC 一致）
    constexpr f32 SMOOTH_FACTOR = 0.2f;
    f32 smoothedTickTimeMs = 0.0f;

    // 更新目标每tick毫秒数
    m_debugStats.targetMsPerTick.store(static_cast<f32>(tickDuration.count()), std::memory_order::relaxed);

    while (m_running.load(std::memory_order::acquire)) {
        MC_TRACE_EVENT("server.tick", "MainLoopIteration");

        auto startTime = clock::now();

        tick();

        auto elapsed = clock::now() - startTime;
        const f32 tickTimeMs = std::chrono::duration<f32, std::milli>(elapsed).count();

        // 指数移动平均平滑 tick 耗时
        if (smoothedTickTimeMs == 0.0f) {
            smoothedTickTimeMs = tickTimeMs;
        } else {
            smoothedTickTimeMs = smoothedTickTimeMs * (1.0f - SMOOTH_FACTOR) + tickTimeMs * SMOOTH_FACTOR;
        }

        // 更新调试统计（原子写入，客户端线程可安全读取）
        m_debugStats.smoothedTickTimeMs.store(smoothedTickTimeMs, std::memory_order::relaxed);

        // 更新强制区块计数（从主维度获取）
        if (m_dimensionManager != nullptr) {
            if (auto* overworld = m_dimensionManager->getOverworld(); overworld != nullptr) {
                if (auto* world = overworld->world(); world != nullptr) {
                    if (auto* chunkMgr = world->chunkManager(); chunkMgr != nullptr) {
                        const auto& ticketManager = chunkMgr->ticketManager();
                        m_debugStats.forcedChunkCount.store(
                            static_cast<i32>(ticketManager.getForcedChunks().size()), std::memory_order::relaxed);
                    }
                }
            }
        }

        auto sleepTime = tickDuration - elapsed;
        MC_TRACE_COUNTER("server.tick", "ServerTickTime", static_cast<i64>(elapsed.count()));

        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

void IntegratedServer::pollNetwork()
{
    MC_TRACE_EVENT("server.network", "ProcessPackets");
    std::vector<u8> packetData;
    while (m_running.load() && m_serverEndpoint && m_serverEndpoint->receive(packetData)) {
        // 使用会话ID 0（单玩家模式只有一个客户端）
        dispatchPacket(0, packetData.data(), packetData.size());
    }
}

void IntegratedServer::broadcastPacket(const u8* data, size_t size)
{
    _sendToClient(data, size);
}

// ============================================================================
// 数据包处理
// ============================================================================

void IntegratedServer::handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size)
{
    (void)sessionId;
    MC_TRACE_EVENT("server.network", "IntegratedServer::handleLoginRequestPacket");

    network::PacketDeserializer deser(data, size);
    auto result = network::LoginRequestPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::warn("Failed to parse login request");
        _sendLoginResponse(false, 0, INVALID_ENTITY_ID, "", "Invalid login request");
        return;
    }

    auto& packet = result.value();
    std::string username = packet.username();

    spdlog::info("Player '{}' attempting to join", username);

    // 白名单检查（白名单启用时拒绝不在名单中的玩家）
    if (m_whitelistManager->isEnabled() && !m_whitelistManager->isNameWhitelisted(username)) {
        spdlog::info("Player '{}' rejected: not in whitelist", username);
        _sendLoginResponse(false, 0, INVALID_ENTITY_ID, username, "You are not whitelisted on this server!");
        return;
    }

    // 创建本地连接
    m_clientConnection = std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());

    // 分配玩家ID
    m_clientPlayerId = m_playerManager->nextPlayerId();

    // 生成离线模式 UUID（基于用户名）
    Uuid offlineUuid = util::generateOfflineUuid(username);
    std::string uuidStr = util::uuidToString(offlineUuid);

    // 添加玩家会话信息
    auto* playerData = m_playerManager->addPlayer(m_clientPlayerId, uuidStr, username, m_clientConnection);
    if (!playerData) {
        _sendLoginResponse(false, 0, INVALID_ENTITY_ID, username, "Failed to add player");
        return;
    }

    // 设置玩家初始状态
    setupInitialPlayerState(playerData, static_cast<GameMode>(m_settings.defaultGameMode.get()));

    // 创建玩家实体并加入世界（关键：玩家实体纳入 EntityManager 和 EntityTracker）
    // 玩家始终在主世界生成
    {
        MC_TRACE_EVENT("server.network",
            "IntegratedServer::handleLoginRequestPacket::CreatePlayerEntity",
            "username",
            username,
            "playerId",
            m_clientPlayerId);

        auto* overworld = m_dimensionManager->getOverworld();
        MC_ASSERT_RELEASE(overworld != nullptr && overworld->world() != nullptr);
        Player* playerEntity = m_playerEntityManager.createPlayerEntity(m_clientPlayerId,
            username,
            *overworld->world(),
            this,
            m_clientConnection,
            static_cast<f32>(playerData->x),
            static_cast<f32>(playerData->y),
            static_cast<f32>(playerData->z));

        if (!playerEntity) {
            spdlog::error("Failed to create player entity for {}", username);
            m_playerManager->removePlayer(m_clientPlayerId);
            _sendLoginResponse(false, 0, INVALID_ENTITY_ID, username, "Failed to create player entity");
            return;
        }

        // 登录阶段必须先建立玩家维度映射，否则 TeleportConfirm 回来后无法解析玩家所在世界，
        // 首次区块票据和区块包维度也会失去上下文。
        m_dimensionManager->playerJoinDimension(m_clientPlayerId, overworld->id());

        // 记录实体ID
        m_clientEntityId = playerEntity->id();
    }

    // 从 OP 列表设置玩家权限等级（集成服务器中单机玩家默认拥有完整权限）
    i32 playerPermissionLevel = resolveOpLevel(playerData->uuid);
    {
        auto* world = getPlayerWorld(m_clientPlayerId);
        if (world != nullptr) {
            if (Player* player = playerEntityManager().getPlayerEntity(m_clientPlayerId, *world)) {
                player->setPermissionLevel(playerPermissionLevel);

                // 从存档加载玩家数据并恢复到实体
                auto* storage = sharedStorage();
                if (storage) {
                    auto loadResult = storage->loadPlayer(playerData->uuid);
                    if (loadResult.success() && loadResult.value().has_value()) {
                        const auto& saveData = loadResult.value().value();
                        world::storage::PlayerDataManager::applyToPlayer(*player, saveData);
                        spdlog::info("Player '{}' loaded saved data (level {}, gameMode {})",
                            playerData->username,
                            saveData.experienceLevel,
                            static_cast<i32>(saveData.gameMode));
                    }
                }
            }
        }
    }

    // 初始化物品栏
    {
        MC_TRACE_EVENT("server.player", "IntegratedServer::handleLoginRequestPacket::InitInventory");

        m_clientInventory.clear();
        m_clientInventory.setSelectedSlot(0);

        if (playerData->gameMode == GameMode::Creative) {
            if (Items::DIAMOND_PICKAXE != nullptr) {
                m_clientInventory.setItem(0, ItemStack(*Items::DIAMOND_PICKAXE, 1));
            }
            i32 slot = 1;
            BlockItemRegistry::instance().forEachBlockItem([this, &slot](const BlockItem& item) {
                if (slot >= PlayerInventory::TOTAL_SIZE) {
                    return;
                }
                m_clientInventory.setItem(slot, ItemStack(item, 64));
                ++slot;
            });
        }
    }

    // 发送登录成功响应（包含 playerId 和 entityId）
    _sendLoginResponse(true, m_clientPlayerId, m_clientEntityId, username, "Welcome to singleplayer world!");

    // 同步玩家权限等级到客户端（同时发送 EntityStatusPacket 和命令树）
    sendPermissionLevelChange(m_clientPlayerId, playerPermissionLevel);

    // 发送初始游戏状态
    sendInitialGameState(
        m_clientPlayerId, playerData->x, playerData->y, playerData->z, playerData->yaw, playerData->pitch);
    _sendPlayerInventory();

    spdlog::info(
        "Player '{}' (PlayerId={}, EntityId={}) joined the game", username, m_clientPlayerId, m_clientEntityId);
}

void IntegratedServer::handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleHotbarSelect");
    auto* player = _getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = HotbarSelectPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse hotbar select packet: {}", result.error().message());
        return;
    }

    m_clientInventory.setSelectedSlot(result.value().slot());
}

void IntegratedServer::handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleContainerClick");
    auto* player = _getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = ContainerClickPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse container click packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    Player& menuPlayer = _getMenuPlayer();
    if (!ContainerPacketHandler::handleContainerClick(menuPlayer, packet)) {
        return;
    }

    _sendContainerContent(*m_openMenu);
    _sendPlayerInventory();
}

void IntegratedServer::handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleCloseContainer");
    auto* player = _getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = CloseContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse close container packet: {}", result.error().message());
        return;
    }

    if (!m_openMenu || result.value().containerId() != m_openMenu->getId()) {
        return;
    }

    _closeCurrentContainer(false);
    _sendPlayerInventory();
}

// ============================================================================
// 数据包发送
// ============================================================================

void IntegratedServer::_sendLoginResponse(
    bool success, PlayerId playerId, EntityId entityId, const std::string& username, const std::string& message)
{
    MC_TRACE_EVENT("server.network",
        "IntegratedServer::_sendLoginResponse",
        "success",
        success,
        "playerId",
        playerId,
        "entityId",
        entityId);

    auto* overworldForLogin = m_dimensionManager->getOverworld();
    bool isDebugWorld = overworldForLogin && overworldForLogin->world() && overworldForLogin->world()->isDebugWorld();
    network::LoginResponsePacket response(success, playerId, entityId, username, message, isDebugWorld);
    network::PacketSerializer ser;
    response.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::LoginResponse, ser.buffer());
    _sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::_sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    MC_TRACE_EVENT(
        "server.network", "IntegratedServer::_sendTeleport", "x", x, "y", y, "z", z, "teleportId", teleportId);

    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Teleport, ser.buffer());
    _sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::_sendPlayerInventory()
{
    MC_TRACE_EVENT("server.network", "IntegratedServer::_sendPlayerInventory");

    PlayerInventoryPacket packet(m_clientInventory);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlayerInventory, ser.buffer());
    _sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::_sendContainerContent(const AbstractContainerMenu& menu)
{
    MC_TRACE_EVENT("server.network", "IntegratedServer::_sendContainerContent", "containerId", menu.getId());

    if (m_serverEndpoint == nullptr || !m_serverEndpoint->isConnected()) {
        return;
    }

    network::PacketSerializer payload;
    auto packet = ContainerPacketHandler::createContentPacket(menu);
    packet.serialize(payload);

    auto fullPacket =
        core::ConnectionManager::encapsulatePacket(network::PacketType::ContainerContent, payload.buffer());
    m_serverEndpoint->send(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::_sendOpenContainer(
    ContainerId containerId, mc::ContainerType type, const std::string& title, i32 slotCount)
{
    MC_TRACE_EVENT("server.network",
        "IntegratedServer::_sendOpenContainer",
        "containerId",
        containerId,
        "type",
        static_cast<i32>(type));

    (void)slotCount; // slotCount 不再发送到客户端
    if (m_serverEndpoint == nullptr || !m_serverEndpoint->isConnected()) {
        return;
    }

    network::PacketSerializer payload;
    auto packet =
        ContainerPacketHandler::createOpenContainerPacket(containerId, ContainerTypes::toNetworkType(type), title);
    packet.serialize(payload);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::OpenContainer, payload.buffer());
    m_serverEndpoint->send(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::_sendCloseContainer(ContainerId containerId)
{
    MC_TRACE_EVENT("server.network", "IntegratedServer::_sendCloseContainer", "containerId", containerId);

    if (m_serverEndpoint == nullptr || !m_serverEndpoint->isConnected()) {
        return;
    }

    network::PacketSerializer payload;
    CloseContainerPacket packet(containerId);
    packet.serialize(payload);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::CloseContainer, payload.buffer());
    m_serverEndpoint->send(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::_sendToClient(const u8* data, size_t size)
{
    MC_TRACE_EVENT("server.network", "IntegratedServer::_sendToClient", "size", size);

    if (m_serverEndpoint && m_serverEndpoint->isConnected()) {
        m_serverEndpoint->send(data, size);
    }
}

void IntegratedServer::_sendBlockBreakAnim(EntityId breakerId, i32 x, i32 y, i32 z, i8 stage)
{
    MC_TRACE_EVENT("server.network",
        "IntegratedServer::_sendBlockBreakAnim",
        "breakerId",
        breakerId,
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "stage",
        stage);

    network::BlockBreakAnimPacket packet;
    packet.setBreakerEntityId(breakerId);
    packet.setPosition(BlockPos(x, y, z));
    packet.setStage(stage);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize BlockBreakAnim packet: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::BlockBreakAnim, result.value());
    _sendToClient(fullPacket.data(), fullPacket.size());
}

i32 IntegratedServer::resolveOpLevel(const std::string& uuid) const noexcept
{
    const i32 base = static_cast<i32>(m_opListManager->getLevel(uuid));

    // 主机身份动态判定：m_clientPlayerId 登录后非 0，playerManager 持有其 UUID
    bool isOwner = false;
    if (m_clientPlayerId != 0 && m_playerManager != nullptr) {
        const auto* host = m_playerManager->getPlayer(m_clientPlayerId);
        isOwner = (host != nullptr && host->uuid == uuid);
    }

    return core::applyOwnerCheatsBoost(base, isOwner, m_params.allowCommands);
}

bool IntegratedServer::openContainerRequest(ContainerType type, const BlockPos& pos, Player& player)
{
    (void)player;
    return _openContainerMenu(type, pos);
}

bool IntegratedServer::_openContainerMenu(ContainerType type, const BlockPos& pos)
{
    auto* player = _getPlayerData();
    if (!player) {
        return false;
    }

    _closeCurrentContainer(true);

    ContainerId containerId = m_nextContainerId++;
    std::unique_ptr<AbstractContainerMenu> menu;

    switch (type) {
        case ContainerType::Crafting: {
            auto craftingMenu = std::make_unique<CraftingMenu>(containerId, &m_clientInventory, nullptr);
            craftingMenu->updateResult();
            menu = std::move(craftingMenu);
            break;
        }
        case ContainerType::Generic9x3:
        case ContainerType::Generic9x6: {
            auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(m_clientPlayerId);
            auto* playerWorld = playerDim ? playerDim->world() : nullptr;
            if (playerWorld == nullptr) {
                return false;
            }

            BlockEntity* blockEntity = playerWorld->getBlockEntity(pos);
            if (blockEntity == nullptr) {
                return false;
            }

            if (blockEntity->getType() != BlockEntityType::Chest &&
                blockEntity->getType() != BlockEntityType::TrappedChest) {
                return false;
            }

            auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);
            if (chest->isDoubleChest(*playerWorld)) {
                auto doubleInventory = chest->getDoubleInventory(*playerWorld);
                if (!doubleInventory) {
                    return false;
                }

                m_openInventoryOwner = std::shared_ptr<IInventory>(std::move(doubleInventory));
                menu = blockentity::ChestContainer::createDouble(
                    containerId, &m_clientInventory, m_openInventoryOwner.get());
            } else {
                m_openInventoryOwner.reset();
                menu =
                    blockentity::ChestContainer::createSingle(containerId, &m_clientInventory, chest->getInventory());
            }
            break;
        }
        case ContainerType::Furnace:
        case ContainerType::BlastFurnace:
        case ContainerType::Smoker: {
            auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(m_clientPlayerId);
            auto* playerWorld = playerDim ? playerDim->world() : nullptr;
            if (playerWorld == nullptr) {
                return false;
            }

            BlockEntity* blockEntity = playerWorld->getBlockEntity(pos);
            if (blockEntity == nullptr) {
                return false;
            }

            if (blockEntity->getType() != BlockEntityType::Furnace &&
                blockEntity->getType() != BlockEntityType::BlastFurnace &&
                blockEntity->getType() != BlockEntityType::Smoker) {
                return false;
            }

            auto* furnace = static_cast<blockentity::AbstractFurnaceEntity*>(blockEntity);
            m_openInventoryOwner.reset();
            menu = std::make_unique<blockentity::FurnaceContainer>(
                containerId, &m_clientInventory, furnace->getInventory(), furnace);
            break;
        }
        case ContainerType::Crafter: {
            auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(m_clientPlayerId);
            auto* playerWorld = playerDim ? playerDim->world() : nullptr;
            if (playerWorld == nullptr) {
                return false;
            }

            BlockEntity* blockEntity = playerWorld->getBlockEntity(pos);
            if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Crafter) {
                return false;
            }

            auto* crafter = static_cast<CrafterBlockEntity*>(blockEntity);
            m_openInventoryOwner.reset();
            menu = std::make_unique<mc::CrafterContainer>(
                containerId, &m_clientInventory, crafter->getInventory(), crafter);
            break;
        }
        case ContainerType::Anvil: {
            auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(m_clientPlayerId);
            auto* playerWorld = playerDim ? playerDim->world() : nullptr;
            if (playerWorld == nullptr) {
                return false;
            }

            menu = std::make_unique<mc::AnvilContainer>(containerId, &m_clientInventory, pos, playerWorld);
            break;
        }
        case ContainerType::Player:
        default:
            return false;
    }

    if (!menu) {
        return false;
    }

    _sendOpenContainer(containerId, type, std::string(ContainerTypes::getDefaultTitle(type)), menu->getSlotCount());
    _sendContainerContent(*menu);

    m_openContainerType = type;
    m_openContainerPos = pos;
    m_openMenu = std::move(menu);
    _getMenuPlayer().setOpenContainerMenu(m_openMenu.get());
    return true;
}

void IntegratedServer::_closeCurrentContainer(bool sendClosePacket)
{
    if (!m_openMenu) {
        return;
    }

    auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(m_clientPlayerId);
    auto* playerWorld = playerDim ? playerDim->world() : nullptr;
    if (playerWorld &&
        (m_openContainerType == ContainerType::Generic9x3 || m_openContainerType == ContainerType::Generic9x6 ||
            m_openContainerType == ContainerType::ShulkerBox)) {
        BlockEntity* blockEntity = playerWorld->getBlockEntity(m_openContainerPos);
        if (blockEntity != nullptr &&
            (blockEntity->getType() == BlockEntityType::Chest ||
                blockEntity->getType() == BlockEntityType::TrappedChest)) {
            auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);
            chest->closeContainer(nullptr);

            // 双箱时，同步减少另一半的打开计数
            if (chest->isDoubleChest(*playerWorld)) {
                blockentity::ChestEntity* connected = chest->getConnectedChest(*playerWorld);
                if (connected != nullptr) {
                    connected->closeContainer(nullptr);
                }
            }
        }
    }

    Player& menuPlayer = _getMenuPlayer();
    m_openMenu->removed(menuPlayer);
    if (sendClosePacket) {
        _sendCloseContainer(m_openMenu->getId());
    }
    menuPlayer.clearOpenContainerMenu();
    m_openMenu.reset();
    m_openInventoryOwner.reset();
    m_openContainerType = ContainerType::Player;
    m_openContainerPos = BlockPos();
}

void IntegratedServer::_openCraftingTableMenu()
{
    (void)_openContainerMenu(ContainerType::Crafting, BlockPos());
}

void IntegratedServer::onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory)
{
    MC_UNUSED(playerId);
    MC_UNUSED(inventory);
}

ItemStack IntegratedServer::getHeldItemForPlacement(PlayerId playerId)
{
    MC_UNUSED(playerId);
    return m_clientInventory.getSelectedStack();
}

i32 IntegratedServer::getSelectedHotbarSlot(PlayerId playerId)
{
    MC_UNUSED(playerId);
    return m_clientInventory.getSelectedSlot();
}

void IntegratedServer::setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack)
{
    MC_UNUSED(playerId);
    m_clientInventory.setItem(slotIndex, stack);
}

void IntegratedServer::syncPlayerInventory(PlayerId playerId)
{
    MC_UNUSED(playerId);
    _sendPlayerInventory();
}

bool IntegratedServer::tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos)
{
    MC_UNUSED(playerId);
    MC_UNUSED(pos);
    _openCraftingTableMenu();
    return true;
}

} // namespace mc::server
