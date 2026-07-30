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
#include "common/entity/inventory/ContainerTypeUtils.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/inventory/PlayerEnderChestInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/entity/inventory/container/CartographyContainer.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/inventory/container/CrafterContainer.hpp"
#include "common/entity/inventory/container/EnchantmentContainer.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
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
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "minecraft-reborn/version.h"

#include <array>
#include <mutex>
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

    // 世界生成 100% 数据驱动，注册表无硬编码兜底；数据包列表为空时各 worldgen loader 会
    // clear() 后加载 0 条目，致 RandomState::create 断言失败。原版 Minecraft 始终内置 vanilla
    // 数据包，此处镜像该语义：扫描到 0 包时从默认游戏目录注入原版数据包。用户目录已放置自定义
    // 数据包时不干预（见 ensureVanillaBuiltinPack 守卫）。
    const auto vanillaDataPackDir = GameDirectory::defaultDirectory().dataPacksDir() / "Vanilla";
    if (m_dataPackList.ensureVanillaBuiltinPack(vanillaDataPackDir) > 0) {
        spdlog::info("Injected builtin vanilla data pack from '{}'", vanillaDataPackDir.string());
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
    traceConfig.bufferSizeKb = 65536 * 8;
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

    // 容器回调装配延迟到 initializeInteractionManagers() 之后（见 _setupContainerCallbacks），
    // 因 containerManager() 依赖 m_containerManager 在该处创建。

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

    auto dimInitResult = m_dimensionManager->initialize(m_settings.parseSeed(),
        m_settings.viewDistance.get(),
        overworldType,
        resource::ResourceLocation("minecraft", "normal"));
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

    // 装配容器回调：须在 initializeInteractionManagers() 创建 m_containerManager 之后，
    // 否则 containerManager() 解引用空指针崩溃。
    _setupContainerCallbacks();

    // 初始化同步管理器
    initializeSyncManagers();

    // 初始化区块同步管理器
    initializeChunkSyncManagers();

    // 设置世界回调（包括光照变化回调）
    setupWorldCallbacks();

    // 初始化网络门面：TCP accept → Wire 模式 ServerClientConnection → 新 pipeline::Connection。
    // 远程 TCP 入站经接收线程 enqueueInbound，主线程 pollNetwork→tick→drainInbound 派发，
    // 消除跨线程进入非线程安全世界状态的隐患。
    m_serverNetwork = std::make_unique<mc::server::net::ServerNetwork>();
    m_serverNetwork->onClientConnect(
        [this](mc::server::net::ServerClientConnection& conn) { _onRemoteClientConnect(conn); });
    m_serverNetwork->onClientDisconnect(
        [this](mc::server::net::ServerClientConnection& conn) { _onRemoteClientDisconnect(conn); });

    auto acceptResult = m_serverNetwork->startAccept(
        static_cast<u16>(m_settings.serverPort.get()), static_cast<u32>(m_settings.maxPlayers.get()));
    if (acceptResult.failed()) {
        return Error(ErrorCode::InitializationFailed, "Failed to start server: " + acceptResult.error().message());
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

    // 关闭网络门面：先清 session（持 ServerClientConnection& 引用，须先于连接销毁），
    // 再 reset ServerNetwork（关 acceptor + join accept 线程 + 析构各 ServerClientConnection
    // 含 join 接收线程）。
    m_remoteSessions.clear();
    m_serverNetwork.reset();

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
    // 主线程驱动：tick pump Local（无）+ drain Wire 入站队列 + 派发延迟断开。
    if (m_serverNetwork) {
        m_serverNetwork->tick();
        _drainDisconnectedSessions();
    }
}

void StandaloneServer::broadcastPacket(const mc::network::ir::IrPacket& packet)
{
    m_playerManager->forEachPlayer([&packet](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            player.send(mc::network::ir::IrPacket{packet});
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

void StandaloneServer::_setupContainerCallbacks()
{
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

        // 1.21.11 OpenScreen：containerId + menuType + title(JSON 文本)。
        mc::network::ir::play::OpenScreen pkt;
        pkt.containerId = static_cast<i32>(containerId);
        pkt.menuType = ContainerTypes::toNetworkType(type);
        pkt.title = resolvedTitle;
        sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(pkt)},
            });
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

            // 1.21.11 ContainerClose：containerId（服务端确认关闭）。
            mc::network::ir::play::ContainerClose pkt;
            pkt.containerId = static_cast<i32>(containerId);
            sendPacketToPlayer(playerId,
                mc::network::ir::IrPacket{
                    mc::network::protocol::ConnectionProtocol::Play,
                    mc::network::ir::PlayPacket{std::move(pkt)},
                });
        });

    containerManager().setOnContainerUpdate([this](PlayerId playerId, const AbstractContainerMenu& menu) {
        // 1.21.11 ContainerSetContent：containerId + stateId + items + carriedItem。
        mc::network::ir::play::ContainerSetContent pkt;
        pkt.containerId = static_cast<i32>(menu.getId());
        pkt.stateId = 0;
        const i32 slotCount = menu.getSlotCount();
        pkt.items.reserve(static_cast<size_t>(slotCount));
        for (i32 slot = 0; slot < slotCount; ++slot) {
            const auto* slotPtr = menu.getSlot(slot);
            if (slotPtr != nullptr) {
                pkt.items.push_back(mc::network::ir::toItemStackView(slotPtr->getItem()));
            } else {
                pkt.items.push_back(mc::network::ir::play::ItemStackView{});
            }
        }
        pkt.carriedItem = mc::network::ir::play::ItemStackView{};
        sendPacketToPlayer(playerId,
            mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{std::move(pkt)},
            });
    });
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

void StandaloneServer::_onRemoteClientConnect(mc::server::net::ServerClientConnection& conn)
{
    const u32 sessionId = conn.sessionId();
    spdlog::info("Remote client connected: sessionId={}", sessionId);

    // 建会话簿记：握手状态机（离线模式 + 独立服压缩阈值）+ Play 路由器（playerId 占位 0）。
    // 路由器 playerId 在 _onRemotePlayerReady 握手完成后回填。
    auto session = std::make_unique<mc::server::net::RemoteClientSession>(
        conn, /*isOfflineMode=*/true, kStandaloneCompressionThreshold, *this, /*playerId=*/0, sessionId);

    // 握手完成回调：进入 Play 时创建玩家实体并回填 playerId。
    session->handshake().onPlayerReady(
        [this, sessionId](const std::string& username, const std::array<u8, 16>& offlineUuid) {
            _onRemotePlayerReady(sessionId, username, offlineUuid);
        });

    // Status（服务器列表 ping）信息提供者：从设置 + 玩家管理器取值，构造 StatusResponse JSON。
    session->handshake().onStatusRequest([this]() -> mc::server::net::StatusInfo {
        return mc::server::net::StatusInfo{m_settings.motd.get(),
            std::string("1.21.11"),
            mc::network::backend::java::kJavaProtocolVersion,
            m_settings.maxPlayers.get(),
            static_cast<i32>(m_playerManager->playerCount()),
            m_settings.onlineMode.get()};
    });

    // 装配 Wire 入站派发分支（主线程 drainInbound 调用）：
    //   handshake.handleInbound 返回 true=握手/Configuration 已消费；false=Play 包交路由器。
    mc::server::net::RemoteClientSession* sessionRaw = session.get();
    conn.setInboundHandler([sessionRaw](const mc::network::ir::IrPacket& packet) {
        auto hResult = sessionRaw->handshake().handleInbound(packet);
        if (!hResult.success()) {
            spdlog::error("StandaloneServer: handshake inbound failed: {}", hResult.error().toString());
            return;
        }
        if (hResult.value()) {
            return; // 握手/Configuration 包已消费
        }
        auto pResult = sessionRaw->playRouter().handle(packet);
        if (!pResult.success()) {
            spdlog::error("StandaloneServer: play router failed: {}", pResult.error().toString());
        }
    });

    // 接收线程仅入队，不跑游戏逻辑（跨线程安全）。
    mc::server::net::ServerClientConnection* connPtr = &conn;
    conn.onPacket([connPtr](const mc::network::ir::IrPacket& packet) { connPtr->enqueueInbound(packet); });

    {
        std::lock_guard<std::mutex> lock(m_remoteSessionsMutex);
        m_remoteSessions[sessionId] = std::move(session);
    }
}

void StandaloneServer::_onRemotePlayerReady(
    u32 sessionId, const std::string& username, const std::array<u8, 16>& offlineUuid)
{
    // 查 session 取连接；session 在 _onRemoteClientConnect 已登记。
    mc::server::net::ServerClientConnection* conn = nullptr;
    mc::server::net::RemoteClientSession* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_remoteSessionsMutex);
        auto it = m_remoteSessions.find(sessionId);
        if (it == m_remoteSessions.end()) {
            spdlog::warn("StandaloneServer: onPlayerReady for unknown sessionId={}", sessionId);
            return;
        }
        session = it->second.get();
        conn = session->connection();
    }
    if (conn == nullptr) {
        return;
    }

    // 远程玩家世界参数从 m_settings 取（独立服无 m_params）：hardcore、seed(parseSeed)、isFlat。
    const bool isFlat = (m_settings.levelType.get() == LevelType::Flat);
    const i64 seed = static_cast<i64>(m_settings.parseSeed());
    const bool hardcore = m_settings.hardcore.get();
    auto creation = createPlayerForConnection(*conn, username, offlineUuid, hardcore, seed, isFlat);
    if (!creation.success) {
        spdlog::warn("StandaloneServer: failed to create player entity for '{}'", username);
        return;
    }
    // 回填路由器 playerId（构造时占位 0），此后 Play 包按真实 playerId 派发。
    session->playRouter().setPlayerId(creation.playerId);
    m_playerEntityIds[creation.playerId] = creation.entityId;
}

void StandaloneServer::_drainDisconnectedSessions()
{
    // 断开的连接已由 ServerNetwork::tick() 在主线程回调 _onRemoteClientDisconnect。
    // 此处无额外延迟队列需处理（与 IntegratedServer LAN 不同，独立服单网络无本地连接）。
}

void StandaloneServer::_onRemoteClientDisconnect(mc::server::net::ServerClientConnection& conn)
{
    const u32 sessionId = conn.sessionId();
    spdlog::info("Remote client disconnected: sessionId={}", sessionId);

    // 移除会话簿记
    mc::server::net::RemoteClientSession* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_remoteSessionsMutex);
        auto it = m_remoteSessions.find(sessionId);
        if (it != m_remoteSessions.end()) {
            session = it->second.get();
            m_remoteSessions.erase(it);
        }
    }
    (void)session;

    // 移除玩家（若已创建实体）：按 sessionId 查 playerId。
    PlayerId playerId = m_playerManager->getPlayerIdBySession(sessionId);
    if (playerId != 0) {
        // 清理实体ID映射
        m_playerEntityIds.erase(playerId);

        // 清理玩家实体
        auto* world = getPlayerWorld(playerId);
        if (world != nullptr) {
            m_playerEntityManager.removePlayerEntity(playerId, *world);
        }

        // 移除玩家会话信息
        m_playerManager->removePlayer(playerId);

        // 清理物品栏
        inventoryManager().cleanupInventory(playerId);
    }
}

// ============================================================================
// 数据包处理
// ============================================================================

void StandaloneServer::handleHotbarSelectPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    // 1.21.11 SetCarriedItem（C→S）：玩家切换快捷栏选中槽位。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::SetCarriedItem>(&play);
    if (evt == nullptr) {
        return;
    }
    const i32 slot = evt->slot;

    inventoryManager().setSelectedSlot(playerId, slot);

    // 服务端回送确认包（1.21.11 SetHeldSlot）。
    mc::network::ir::play::SetHeldSlot resp;
    resp.slot = slot;
    sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(resp)},
        });
}

void StandaloneServer::handleContainerClickPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    // 1.21.11 ContainerClick（C→S）：stateId + slot + button + clickType + carriedItem(HashedStack)。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ContainerClick>(&play);
    if (evt == nullptr) {
        return;
    }

    // TODO(Phase6): carriedItem 当前是 HashedStack，多玩家远程路径下需桥接回 ItemStack。
    //   集成服本地路径已有 hashedStackToItemStack；此处独立服暂用空 cursor。
    ItemStack cursorItem;

    auto clickResult = containerManager().handleClick(playerId,
        static_cast<mc::ContainerId>(evt->containerId),
        evt->slotNum,
        static_cast<u8>(evt->buttonNum),
        static_cast<u8>(evt->clickType),
        cursorItem);

    if (clickResult.success()) {
        // 同步物品栏到客户端
        inventoryManager().syncToClient(playerId);
    }
}

void StandaloneServer::handleCloseContainerPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    // 1.21.11 ContainerClose（C→S）：containerId。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ContainerClose>(&play);
    if (evt == nullptr) {
        return;
    }

    // 使用 ContainerManager 关闭容器
    containerManager().closeContainer(playerId);

    // 同步物品栏到客户端
    inventoryManager().syncToClient(playerId);
}

void StandaloneServer::handleOpenPlayerInventoryPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    // 1.21.11 PlayerCommand{action=OPEN_INVENTORY}（C→S）。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerCommand>(&play);
    if (evt == nullptr || evt->action != 5) { // OPEN_INVENTORY
        return;
    }

    // TODO(Phase6): 独立服远程玩家打开背包的菜单建立（Player 类型未注册菜单工厂）。
    //   集成服本地路径有 _openItemPickerMenu/_openPlayerInventoryMenu，远程路径暂留空。
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

Result<void> StandaloneServer::publishToLan(i32 port, bool allowCheats)
{
    (void)port;
    (void)allowCheats;
    // 独立服务器启动时已经通过 TCP 监听对外提供服务，无法再次发布到局域网
    return Error(ErrorCode::Unsupported,
        "Dedicated server cannot publish to LAN: already listening on TCP port " +
            std::to_string(m_settings.serverPort.get()));
}

} // namespace mc::server
