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

#include "StandaloneServer.hpp"
#include "common/core/DefaultValues.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/inventory/PlayerEnderChestInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/entity/inventory/container/CartographyContainer.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/inventory/container/CrafterContainer.hpp"
#include "common/entity/inventory/container/EnchantmentContainer.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/blockentity/interactive/EnchantingTableEntity.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/blockentity/storage/EnderChestEntity.hpp"
#include "common/world/blockentity/trial/CrafterBlockEntity.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/network/TcpConnection.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "minecraft-reborn/version.h"

#include <thread>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

StandaloneServer::StandaloneServer()
    : MinecraftServer(m_settings)
{}

StandaloneServer::~StandaloneServer()
{
    if (m_initialized) {
        stop();
    }
}

Result<void> StandaloneServer::initialize()
{
    return initialize(StandaloneServerParams{});
}

Result<void> StandaloneServer::initialize(const StandaloneServerParams& params)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Server already initialized");
    }

    // 加载设置
    std::filesystem::path settingsFilePath;
    if (params.configPath.has_value()) {
        settingsFilePath = std::filesystem::path(*params.configPath);
    } else {
        settingsFilePath = GameDirectory::defaultDirectory().serverOptionsPath();
    }
    auto settingsResult = _loadSettings(settingsFilePath.string());
    if (settingsResult.failed()) {
        spdlog::warn("Failed to load settings from {}: {}. Using defaults.",
            settingsFilePath.string(),
            settingsResult.error().toString());
    }

    // 从配置文件路径推导游戏目录，并确保目录结构存在
    m_gameDirectory = GameDirectory::fromConfigPath(settingsFilePath);
    auto dirResult = m_gameDirectory.ensureDirectoriesExist();
    if (dirResult.failed()) {
        spdlog::warn("Failed to create game directories: {}", dirResult.error().toString());
    }

    // 扫描数据包目录
    auto dataPackDir = m_gameDirectory.dataPacksDir();
    auto scanResult = m_dataPackList.scanDirectory(dataPackDir);
    if (scanResult.failed()) {
        spdlog::warn(
            "Failed to scan data pack directory '{}': {}", dataPackDir.string(), scanResult.error().toString());
    } else {
        spdlog::info("Scanned {} data packs from '{}'", scanResult.value(), dataPackDir.string());
    }

    // 应用设置到系统
    _applySettings();

    // 设置日志级别
    const auto& logLevel = m_settings.logLevel.get();
    if (logLevel == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (logLevel == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (logLevel == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (logLevel == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (logLevel == "error") {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::info("=== Cubium Server ===");
    spdlog::info("Version: {}.{}.{}", MC_VERSION_MAJOR, MC_VERSION_MINOR, MC_VERSION_PATCH);
    spdlog::info("Initializing standalone server...");

    // 初始化性能追踪
    mc::profiler::TraceConfig traceConfig;
    traceConfig.outputPath = "server_trace.perfetto-trace";
    traceConfig.bufferSizeKb = 65536; // 64MB
    mc::profiler::ProfilerManager::instance().initialize(traceConfig);
    mc::profiler::ProfilerManager::instance().startTracing();

    // 设置进程和主线程名称
    mc::profiler::ProfilerManager::instance().setProcessName("MinecraftServer");
    mc::profiler::ProfilerManager::instance().setThreadName("ServerMainThread");
    spdlog::info("Perfetto tracing initialized");

    // 初始化游戏注册表（包括实体类型）
    initializeRegistries(true);

    // 初始化核心管理器（从设置中读取配置）
    initializeCoreManagers();

    // 加载白名单、封禁列表和 OP 列表
    if (m_gameDirectory.isValid()) {
        const auto& gameRoot = m_gameDirectory.root();
        // 加载白名单
        auto whitelistPath = gameRoot / "whitelist.json";
        auto whitelistResult = m_whitelistManager->load(whitelistPath);
        if (whitelistResult.failed()) {
            spdlog::warn("Failed to load whitelist: {}", whitelistResult.error().message());
        }

        // 加载玩家封禁列表
        auto bannedPlayersPath = gameRoot / "banned-players.json";
        auto bannedPlayersResult = m_bannedPlayerList->load(bannedPlayersPath);
        if (bannedPlayersResult.failed()) {
            spdlog::warn("Failed to load banned players: {}", bannedPlayersResult.error().message());
        }

        // 加载 IP 封禁列表
        auto bannedIpsPath = gameRoot / "banned-ips.json";
        auto bannedIpsResult = m_bannedIpList->load(bannedIpsPath);
        if (bannedIpsResult.failed()) {
            spdlog::warn("Failed to load banned IPs: {}", bannedIpsResult.error().message());
        }

        // 加载 OP 列表
        auto opsPath = gameRoot / "ops.json";
        auto opsResult = m_opListManager->load(opsPath);
        if (opsResult.failed()) {
            spdlog::warn("Failed to load ops: {}", opsResult.error().message());
        }
    }

    auto storageInitResult = initializeSharedStorage(m_gameDirectory, m_settings.worldName.get());
    if (storageInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to initialize shared world storage: " + storageInitResult.error().message());
    }

    containerManager().setMenuFactory([this](ContainerId containerId,
                                          mc::ContainerType type,
                                          const BlockPos& pos,
                                          PlayerInventory* playerInventory,
                                          PlayerId playerId) {
        ContainerMenuCreateResult result;

        if (playerInventory == nullptr) {
            return result;
        }

        auto* overworld = m_dimensionManager->getOverworld();
        if (overworld == nullptr || overworld->world() == nullptr) {
            return result;
        }
        auto* world = overworld->world();

        switch (type) {
            case mc::ContainerType::Crafting: {
                auto menu = std::make_unique<mc::CraftingMenu>(containerId, playerInventory, nullptr);
                menu->updateResult();
                result.menu = std::move(menu);
                return result;
            }
            case mc::ContainerType::Generic9x3:
            case mc::ContainerType::Generic9x6:
            case mc::ContainerType::ShulkerBox: {
                BlockEntity* blockEntity = world->getBlockEntity(pos);
                if (blockEntity == nullptr) {
                    return result;
                }

                // 末影箱：物品栏存储在玩家身上，而非方块实体中
                if (blockEntity->getType() == BlockEntityType::EnderChest) {
                    auto* serverWorld = world->asServerWorld();
                    if (serverWorld == nullptr) {
                        return result;
                    }
                    auto* playerEntity = playerEntityManager().getPlayerEntity(playerId, *serverWorld);
                    if (playerEntity == nullptr) {
                        return result;
                    }
                    auto& enderChestInv = playerEntity->enderChestInventory();
                    result.menu =
                        blockentity::ChestContainer::createSingle(containerId, playerInventory, &enderChestInv);
                    return result;
                }

                if (blockEntity->getType() != BlockEntityType::Chest &&
                    blockEntity->getType() != BlockEntityType::TrappedChest) {
                    return result;
                }

                auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);
                if (chest->isDoubleChest(*world)) {
                    auto doubleInventory = chest->getDoubleInventory(*world);
                    if (!doubleInventory) {
                        return result;
                    }

                    result.inventoryOwner = std::shared_ptr<IInventory>(std::move(doubleInventory));
                    result.menu = blockentity::ChestContainer::createDouble(
                        containerId, playerInventory, result.inventoryOwner.get());
                    return result;
                }

                result.menu =
                    blockentity::ChestContainer::createSingle(containerId, playerInventory, chest->getInventory());
                return result;
            }
            case mc::ContainerType::Furnace:
            case mc::ContainerType::BlastFurnace:
            case mc::ContainerType::Smoker: {
                BlockEntity* blockEntity = world->getBlockEntity(pos);
                if (blockEntity == nullptr) {
                    return result;
                }

                if (blockEntity->getType() != BlockEntityType::Furnace &&
                    blockEntity->getType() != BlockEntityType::BlastFurnace &&
                    blockEntity->getType() != BlockEntityType::Smoker) {
                    return result;
                }

                auto* furnace = static_cast<blockentity::AbstractFurnaceEntity*>(blockEntity);
                result.menu = std::make_unique<blockentity::FurnaceContainer>(
                    containerId, playerInventory, furnace->getInventory(), furnace);
                return result;
            }
            case mc::ContainerType::Enchantment: {
                result.menu = std::make_unique<mc::EnchantmentContainer>(containerId, playerInventory, pos, world);
                return result;
            }
            case mc::ContainerType::Anvil: {
                result.menu = std::make_unique<mc::AnvilContainer>(containerId, playerInventory, pos, world);
                return result;
            }
            case mc::ContainerType::Cartography: {
                result.menu = std::make_unique<mc::CartographyContainer>(containerId, playerInventory, pos, world);
                return result;
            }
            case mc::ContainerType::Crafter: {
                BlockEntity* blockEntity = world->getBlockEntity(pos);
                if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Crafter) {
                    return result;
                }
                auto* crafter = static_cast<CrafterBlockEntity*>(blockEntity);
                result.menu = std::make_unique<mc::CrafterContainer>(
                    containerId, playerInventory, crafter->getInventory(), crafter);
                return result;
            }
            case mc::ContainerType::Player:
            default:
                return result;
        }
    });

    // 容器网络回调：将 ContainerManager 事件转发为客户端协议包。
    containerManager().setOnContainerOpen([this](PlayerId playerId,
                                              ContainerId containerId,
                                              mc::ContainerType type,
                                              const std::string& title,
                                              i32 slotCount) {
        (void)slotCount; // slotCount 不再发送到客户端
        const std::string resolvedTitle = title.empty() ? std::string(ContainerTypes::getDefaultTitle(type)) : title;

        network::PacketSerializer ser;
        auto packet = ContainerPacketHandler::createOpenContainerPacket(
            containerId, ContainerTypes::toNetworkType(type), resolvedTitle);
        packet.serialize(ser);

        auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::OpenContainer, ser.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    });

    containerManager().setOnContainerClose(
        [this](PlayerId playerId, ContainerId containerId, mc::ContainerType type, const BlockPos& pos) {
            if (type == mc::ContainerType::Generic9x3 || type == mc::ContainerType::Generic9x6 ||
                type == mc::ContainerType::ShulkerBox) {
                auto* playerDim = m_dimensionManager->getPlayerDimensionWorld(playerId);
                auto* world = playerDim ? playerDim->world() : nullptr;
                if (world) {
                    BlockEntity* blockEntity = world->getBlockEntity(pos);
                    if (blockEntity != nullptr) {
                        if (blockEntity->getType() == BlockEntityType::Chest ||
                            blockEntity->getType() == BlockEntityType::TrappedChest) {
                            auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);
                            chest->closeContainer(nullptr);

                            // 双箱时，同步减少另一半的打开计数
                            if (chest->isDoubleChest(*world)) {
                                blockentity::ChestEntity* connected = chest->getConnectedChest(*world);
                                if (connected != nullptr) {
                                    connected->closeContainer(nullptr);
                                }
                            }
                        }
                        // 末影箱的关闭由 ChestContainer::removed() →
                        // PlayerEnderChestInventory::closeInventory() → stopOpen() 处理，
                        // 无需在此重复调用
                    }
                }
            }

            network::PacketSerializer ser;
            CloseContainerPacket packet(containerId);
            packet.serialize(ser);

            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::CloseContainer, ser.buffer());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        });

    containerManager().setOnContainerUpdate([this](PlayerId playerId, const AbstractContainerMenu& menu) {
        network::PacketSerializer ser;
        auto packet = ContainerPacketHandler::createContentPacket(menu);
        packet.serialize(ser);

        auto fullPacket =
            core::ConnectionManager::encapsulatePacket(network::PacketType::ContainerContent, ser.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    });

    // 初始化维度管理器
    WorldType overworldType = WorldType::Default;
    switch (m_settings.levelType.get()) {
        case LevelType::Flat:
            overworldType = WorldType::Flat;
            break;
        case LevelType::LargeBiomes:
            overworldType = WorldType::LargeBiomes;
            break;
        case LevelType::Amplified:
            overworldType = WorldType::Amplified;
            break;
        case LevelType::Debug:
            overworldType = WorldType::Debug;
            break;
        case LevelType::Default:
        default:
            overworldType = WorldType::Default;
            break;
    }

    auto dimInitResult = m_dimensionManager->initialize(
        static_cast<u64>(std::stoll(m_settings.levelSeed.get())), m_settings.viewDistance.get(), overworldType);
    if (dimInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to initialize dimension manager: " + dimInitResult.error().message());
    }

    auto* overworld = m_dimensionManager->getOverworld();
    MC_ASSERT_RELEASE(overworld != nullptr);

    m_dimensionManager->forEachDimension([this](Dimension& dim) {
        auto* serverDim = static_cast<ServerDimension*>(&dim);
        auto* world = serverDim->world();
        if (world) {
            attachWorldBindings(*world);
            attachWorldCommandBindings(*world);
        }
    });

    auto worldResult = initializeWorld();
    if (worldResult.failed()) {
        return Error(ErrorCode::InitializationFailed, "Failed to initialize world: " + worldResult.error().message());
    }

    if (overworld->world() && overworld->world()->isDebugWorld()) {
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

    // 初始化区块同步管理器
    initializeChunkSyncManagers();

    // 设置世界回调（包括光照变化回调）
    setupWorldCallbacks();

    // 初始化网络服务器
    m_tcpServer = std::make_unique<TcpServer>();

    // 设置网络回调
    m_tcpServer->setOnConnect([this](TcpSession* session) { _onClientConnect(session); });

    m_tcpServer->setOnDisconnect(
        [this](TcpSession* session, const std::string& reason) { _onClientDisconnect(session, reason); });

    m_tcpServer->setOnPacket([this](TcpSession* session, const u8* data, size_t size) {
        dispatchPacket(static_cast<u32>(session->id()), data, size);
    });

    // 启动服务器
    TcpServerConfig serverConfig;
    serverConfig.port = static_cast<u16>(m_settings.serverPort.get());
    serverConfig.maxConnections = static_cast<u32>(m_settings.maxPlayers.get());

    auto serverResult = m_tcpServer->start(serverConfig);
    if (serverResult.failed()) {
        return Error(ErrorCode::InitializationFailed, "Failed to start server: " + serverResult.error().message());
    }

    spdlog::info("Server initialized successfully");
    spdlog::info("Port: {}", m_settings.serverPort.get());
    spdlog::info("Max players: {}", m_settings.maxPlayers.get());
    spdlog::info("World: {}", m_settings.worldName.get());

    m_initialized = true;
    return Result<void>::ok();
}

void StandaloneServer::shutdown()
{
    stop();
}

void StandaloneServer::savePlayerRuntimeState()
{
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
    // 注意：必须在 stopCore()（含 shutdownManagers）之前调用，
    // 否则维度已被卸载、玩家实体已被销毁
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

void StandaloneServer::stop()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("Stopping server...");
    m_running = false;

    // 先 join 主循环线程，确保 tick() 已完全退出，再回写玩家状态或清理核心组件。
    // 否则 savePlayerRuntimeState() 遍历玩家实体时可能与正在执行的 tick() 并发，
    // 造成数据竞争（玩家位置、生命、背包等被同时读写）。
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    m_serverThread.reset();

    // 回写在线玩家运行时状态到 PlayerDataManager 缓存
    // 此时主循环已退出，玩家实体不再被 tick() 修改，可安全遍历。
    // 必须在 stopCore()（含 shutdownManagers）之前调用，确保维度和玩家实体仍然有效
    savePlayerRuntimeState();

    // 停止核心组件
    stopCore();

    // 关闭网络服务器
    if (m_tcpServer) {
        m_tcpServer->stop();
        m_tcpServer.reset();
    }

    // 保存设置
    const auto savePath =
        m_settingsPath.empty() ? GameDirectory::defaultDirectory().serverOptionsPath() : m_settingsPath;
    auto saveResult = m_settings.saveSettings(savePath);
    if (saveResult.failed()) {
        spdlog::warn("Failed to save settings: {}", saveResult.error().toString());
    }

    // 关闭性能追踪
    mc::profiler::ProfilerManager::instance().stopTracing();
    mc::profiler::ProfilerManager::instance().shutdown();
    spdlog::info("Perfetto tracing stopped");

    m_initialized = false;
    spdlog::info("Server stopped.");
}

void StandaloneServer::pollNetwork()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "PollNetwork");
    m_tcpServer->poll();
}

void StandaloneServer::broadcastPacket(const u8* data, size_t size)
{
    m_playerManager->forEachPlayer([data, size](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            player.send(data, size);
        }
    });
}

Result<void> StandaloneServer::run()
{
    if (!m_initialized) {
        return Error(ErrorCode::InvalidArgument, "Server not initialized");
    }

    if (m_running) {
        return Error(ErrorCode::AlreadyExists, "Server already running");
    }

    spdlog::info("Starting server main loop...");
    m_running = true;

    // 在内部线程中运行主循环，stop() 会先 join 再清理，避免与 tick() 产生数据竞争
    m_serverThread = std::make_unique<std::thread>([this]() {
        try {
            _mainLoop();
        }
        catch (const std::exception& e) {
            spdlog::critical("Server crashed: {}", e.what());
            m_running = false;
        }
    });

    return Result<void>::ok();
}

void StandaloneServer::_mainLoop()
{
    using clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;

    constexpr f64 targetTickTime = 1.0 / 20.0; // 20 TPS
    constexpr auto tickDuration =
        std::chrono::duration_cast<clock::duration>(std::chrono::duration<f64>(targetTickTime));

    auto lastTickTime = clock::now();

    // 平滑 tick 耗时的指数移动平均因子
    constexpr f32 SMOOTH_FACTOR = 0.2f;
    f32 smoothedTickTimeMs = 0.0f;

    // 更新目标每tick毫秒数
    m_debugStats.targetMsPerTick.store(
        static_cast<f32>(std::chrono::duration<f32, std::milli>(tickDuration).count()), std::memory_order::relaxed);

    spdlog::info("Server is now running!");
    spdlog::info("Connect with port: {}", m_settings.serverPort.get());

    while (m_running) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "MainLoopIteration");

        const auto currentTime = clock::now();
        const auto deltaTime = currentTime - lastTickTime;

        if (deltaTime >= tickDuration) {
            // 执行游戏刻
            auto tickStart = clock::now();
            tick();
            auto tickElapsed = clock::now() - tickStart;

            lastTickTime = currentTime;

            // 追踪 TPS
            const f64 tps = 1.0 / (std::chrono::duration<f64>(deltaTime).count());
            MC_TRACE_COUNTER(TraceEvents.Server.Tick, "TPS", static_cast<i64>(tps));
            MC_TRACE_COUNTER(TraceEvents.Server.Tick, "PlayerCount", static_cast<i64>(m_playerManager->playerCount()));

            // 更新调试统计
            const f32 tickTimeMs = std::chrono::duration<f32, std::milli>(tickElapsed).count();
            if (smoothedTickTimeMs == 0.0f) {
                smoothedTickTimeMs = tickTimeMs;
            } else {
                smoothedTickTimeMs = smoothedTickTimeMs * (1.0f - SMOOTH_FACTOR) + tickTimeMs * SMOOTH_FACTOR;
            }
            m_debugStats.smoothedTickTimeMs.store(smoothedTickTimeMs, std::memory_order::relaxed);

            // 更新强制区块计数
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
        } else {
            // 等待下一刻
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

Result<void> StandaloneServer::_loadSettings(const std::string& path)
{
    m_settingsPath = std::filesystem::path(path);

    auto result = m_settings.loadSettings(path);
    if (result.failed()) {
        return result;
    }

    // 确保设置目录存在（使用当前实际设置路径，避免写到默认目录）
    const auto settingsDir = m_settingsPath.parent_path();
    if (!settingsDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(settingsDir, ec);
        if (ec) {
            spdlog::warn("Failed to create settings directory: {}", settingsDir.string());
        }
    }

    // 启用自动保存
    m_settings.enableAutoSave(m_settingsPath);

    spdlog::info("Server settings path: {}", m_settingsPath.string());

    return Result<void>::ok();
}

void StandaloneServer::_applySettings()
{
    // 设置变更回调
    m_settings.serverPort.onChange([this](i32 value) {
        spdlog::info("Server port changed to: {}", value);
        // 端口变更需要重启服务器
    });

    m_settings.maxPlayers.onChange([this](i32 value) {
        spdlog::info("Max players changed to: {}", value);
        if (m_playerManager) {
            m_playerManager->setMaxPlayers(value);
        }
    });

    m_settings.viewDistance.onChange([this](i32 value) {
        spdlog::info("View distance changed to: {}", value);
        auto* overworld = m_dimensionManager->getOverworld();
        if (overworld && overworld->world()) {
            auto config = overworld->world()->config();
            config.viewDistance = value;
            overworld->world()->setConfig(config);
        }
    });

    m_settings.logLevel.onChange([this](const std::string& value) {
        spdlog::info("Log level changed to: {}", value);
        if (value == "trace") {
            spdlog::set_level(spdlog::level::trace);
        } else if (value == "debug") {
            spdlog::set_level(spdlog::level::debug);
        } else if (value == "info") {
            spdlog::set_level(spdlog::level::info);
        } else if (value == "warn") {
            spdlog::set_level(spdlog::level::warn);
        } else if (value == "error") {
            spdlog::set_level(spdlog::level::err);
        }
    });
}

void StandaloneServer::_onClientConnect(TcpSession* session)
{
    spdlog::info("Client connected: {}:{}", session->address(), session->port());
}

void StandaloneServer::_onClientDisconnect(TcpSession* session, const std::string& reason)
{
    spdlog::info("Client disconnected: {}:{} - {}", session->address(), session->port(), reason);

    // 移除玩家
    PlayerId playerId = m_playerManager->getPlayerIdBySession(static_cast<u32>(session->id()));
    if (playerId != 0) {
        m_playerManager->removePlayer(playerId);
    }
}

// ============================================================================
// 数据包处理
// ============================================================================

void StandaloneServer::handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size)
{
    auto session = m_tcpServer->getSession(sessionId);
    if (!session) {
        spdlog::warn("Session {} not found for login request", sessionId);
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::LoginRequestPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::warn("Failed to parse login request from session {}", sessionId);
        _sendLoginResponse(session.get(), false, 0, INVALID_ENTITY_ID, "", "Invalid login request");
        session->disconnect("Invalid login request");
        return;
    }

    auto& packet = result.value();
    std::string username = packet.username();

    spdlog::info("Player '{}' attempting to join from {}:{}", username, session->address(), session->port());

    // 封禁检查（在白名单检查之前执行）
    // 1. 检查玩家名封禁
    if (m_bannedPlayerList->isNameBanned(username)) {
        auto banEntry = m_bannedPlayerList->getEntryByName(username);
        std::string banReason = banEntry.has_value() ? banEntry->reason : "You are banned from this server!";
        spdlog::info("Player '{}' rejected: name banned", username);
        _sendLoginResponse(session.get(), false, 0, INVALID_ENTITY_ID, username, banReason);
        session->disconnect("Name banned");
        return;
    }

    // 2. 检查 IP 封禁
    const std::string& ipAddress = session->address();
    if (m_bannedIpList->isBanned(ipAddress)) {
        auto banEntry = m_bannedIpList->getEntry(ipAddress);
        std::string banReason = banEntry.has_value() ? banEntry->reason : "Your IP is banned from this server!";
        spdlog::info("Player '{}' rejected: IP {} banned", username, ipAddress);
        _sendLoginResponse(session.get(), false, 0, INVALID_ENTITY_ID, username, banReason);
        session->disconnect("IP banned");
        return;
    }

    // 白名单检查
    if (m_whitelistManager->isEnabled() && !m_whitelistManager->isNameWhitelisted(username)) {
        spdlog::info("Player '{}' rejected: not in whitelist", username);
        _sendLoginResponse(
            session.get(), false, 0, INVALID_ENTITY_ID, username, "You are not whitelisted on this server!");
        session->disconnect("Not in whitelist");
        return;
    }

    // 检查玩家数量限制
    if (m_playerManager->isFull()) {
        _sendLoginResponse(session.get(), false, 0, INVALID_ENTITY_ID, username, "Server is full");
        session->disconnect("Server is full");
        return;
    }

    // 创建连接
    auto connection = std::make_shared<TcpConnection>(session);

    // 分配玩家ID
    PlayerId playerId = m_playerManager->nextPlayerId();

    // 生成离线模式 UUID（基于用户名）
    Uuid offlineUuid = util::generateOfflineUuid(username);
    std::string uuidStr = util::uuidToString(offlineUuid);

    // 添加玩家会话信息
    auto* playerData = m_playerManager->addPlayer(playerId, uuidStr, username, connection);
    if (!playerData) {
        _sendLoginResponse(session.get(), false, 0, INVALID_ENTITY_ID, username, "Failed to add player");
        session->disconnect("Failed to add player");
        return;
    }

    // 设置会话ID并建立映射
    playerData->sessionId = sessionId;
    m_playerManager->mapSessionToPlayer(sessionId, playerId);

    // 更新会话状态
    session->setState(SessionState::Playing);

    // 设置玩家初始状态
    setupInitialPlayerState(playerData, static_cast<GameMode>(m_settings.defaultGameMode.get()));

    // 创建玩家实体并加入世界（关键：玩家实体纳入 EntityManager 和 EntityTracker）
    auto* overworldDim = m_dimensionManager->getOverworld();
    MC_ASSERT_RELEASE(overworldDim != nullptr && overworldDim->world() != nullptr);
    Player* playerEntity = m_playerEntityManager.createPlayerEntity(playerId,
        username,
        *overworldDim->world(),
        this,
        connection,
        static_cast<f32>(playerData->x),
        static_cast<f32>(playerData->y),
        static_cast<f32>(playerData->z));

    if (!playerEntity) {
        spdlog::error("Failed to create player entity for {}", username);
        m_playerManager->removePlayer(playerId);
        _sendLoginResponse(session.get(), false, 0, INVALID_ENTITY_ID, username, "Failed to create player entity");
        session->disconnect("Failed to create player entity");
        return;
    }

    // 记录 entityId
    EntityId entityId = playerEntity->id();
    m_playerEntityIds[playerId] = entityId;

    // 从 OP 列表设置玩家权限等级
    i32 playerPermissionLevel = resolveOpLevel(playerData->uuid);
    playerEntity->setPermissionLevel(playerPermissionLevel);

    // 从存档加载玩家数据并恢复到实体
    auto* storage = sharedStorage();
    if (storage) {
        auto loadResult = storage->loadPlayer(uuidStr);
        if (loadResult.success() && loadResult.value().has_value()) {
            // 已有存档：用保存的数据恢复玩家状态
            const auto& saveData = loadResult.value().value();
            world::storage::PlayerDataManager::applyToPlayer(*playerEntity, saveData);
            spdlog::info("Player '{}' loaded saved data (level {}, gameMode {})",
                username,
                saveData.experienceLevel,
                static_cast<i32>(saveData.gameMode));
        } else {
            // 新玩家：使用默认初始状态（setupInitialPlayerState 已设置）
            spdlog::info("Player '{}' is new, using default state", username);
        }
    }

    // 初始化玩家物品栏
    inventoryManager().initializeInventory(playerId);

    if (playerData->gameMode == GameMode::Creative) {
        if (auto* inventory = inventoryManager().getInventory(playerId)) {
            fillCreativeModeInventory(*inventory);
        }
    }

    // 发送登录成功响应（包含 playerId 和 entityId）
    _sendLoginResponse(session.get(), true, playerId, entityId, username, "Welcome!");

    // 同步玩家权限等级到客户端（同时发送 EntityStatusPacket 和命令树）
    sendPermissionLevelChange(playerId, playerPermissionLevel);

    // 发送初始游戏状态
    sendInitialGameState(playerId, playerData->x, playerData->y, playerData->z, playerData->yaw, playerData->pitch);

    // 同步物品栏到客户端
    inventoryManager().syncToClient(playerId);

    spdlog::info("Player '{}' (ID: {}) joined the game", username, playerId);
}

void StandaloneServer::handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    network::PacketDeserializer deser(data, size);
    auto result = HotbarSelectPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse hotbar select packet: {}", result.error().message());
        return;
    }

    const i32 slot = result.value().slot();

    // 使用 InventoryManager 设置选中槽位
    inventoryManager().setSelectedSlot(playerId, slot);

    // 服务端回送确认包
    HotbarSetPacket response(slot);
    network::PacketSerializer ser;
    response.serialize(ser);

    const auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::HotbarSet, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void StandaloneServer::handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    network::PacketDeserializer deser(data, size);
    auto result = ContainerClickPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse container click packet: {}", result.error().message());
        return;
    }

    // 使用 ContainerManager 处理容器点击
    const auto& packet = result.value();
    auto clickResult = containerManager().handleClick(playerId,
        packet.containerId(),
        packet.slotIndex(),
        static_cast<u8>(packet.button()),
        static_cast<u8>(packet.action()),
        packet.cursorItem());

    if (clickResult.success()) {
        // 同步物品栏到客户端
        inventoryManager().syncToClient(playerId);
    }
}

void StandaloneServer::handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    network::PacketDeserializer deser(data, size);
    auto result = CloseContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse close container packet: {}", result.error().message());
        return;
    }

    // 使用 ContainerManager 关闭容器
    containerManager().closeContainer(playerId);

    // 同步物品栏到客户端
    inventoryManager().syncToClient(playerId);
}

// ============================================================================
// 数据包发送
// ============================================================================

void StandaloneServer::_sendLoginResponse(TcpSession* session,
    bool success,
    PlayerId playerId,
    EntityId entityId,
    const std::string& username,
    const std::string& message)
{
    bool isDebugWorld = m_dimensionManager->getOverworld() && m_dimensionManager->getOverworld()->world() &&
        m_dimensionManager->getOverworld()->world()->isDebugWorld();
    network::LoginResponsePacket response(success, playerId, entityId, username, message, isDebugWorld);
    network::PacketSerializer ser;
    response.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::LoginResponse, ser.buffer());
    session->send(fullPacket.data(), fullPacket.size());
}

// ============================================================================
// 回调设置
// ============================================================================

ItemStack StandaloneServer::getHeldItemForPlacement(PlayerId playerId)
{
    return inventoryManager().getHeldItem(playerId);
}

i32 StandaloneServer::getSelectedHotbarSlot(PlayerId playerId)
{
    return inventoryManager().getSelectedSlot(playerId);
}

void StandaloneServer::setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack)
{
    inventoryManager().setItem(playerId, slotIndex, stack);
}

void StandaloneServer::syncPlayerInventory(PlayerId playerId)
{
    inventoryManager().syncToClient(playerId);
}

bool StandaloneServer::tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos)
{
    auto openResult = containerManager().openContainer(playerId, mc::ContainerType::Crafting, pos);
    return openResult.success();
}

} // namespace mc::server
