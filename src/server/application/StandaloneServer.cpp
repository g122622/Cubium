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
#include "server/network/RemoteSessionManager.hpp"
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "StandaloneServer::initialize");

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

    // 批9：远程会话四件套装配 + startAccept 下沉至基类 _setupRemoteSessions。worldParams 从
    // m_settings 取（独立服无 m_params）：hardcore/parseSeed/levelType==Flat。startAccept
    // 失败时基类透传原始 Error，此处按独立服场景包装日志前缀。
    auto setupResult = _setupRemoteSessions(
        "StandaloneServer",
        kStandaloneCompressionThreshold,
        [this]() -> mc::server::net::RemoteWorldParams {
            return {m_settings.hardcore.get(),
                static_cast<i64>(m_settings.parseSeed()),
                m_settings.levelType.get() == LevelType::Flat};
        },
        static_cast<u16>(m_settings.serverPort.get()),
        static_cast<u32>(m_settings.maxPlayers.get()));
    if (setupResult.failed()) {
        return Error(ErrorCode::InitializationFailed, "Failed to start server: " + setupResult.error().message());
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

    // 关闭网络门面：先清远程会话（session 持 ServerClientConnection& 引用，须先于连接销毁），
    // 再 reset ServerNetwork（关 acceptor + join accept 线程 + 析构各 ServerClientConnection
    // 含 join 接收线程）。批9：两步下沉至基类 _shutdownRemoteSessions。
    _shutdownRemoteSessions();

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

// 注：pollNetwork/broadcastPacket/_drainDisconnectedSessions 已于批2a 统一为
// MinecraftServer 基类默认实现，本子类不再 override。基类在未注入本地客户端钩子时
// （StandaloneServer 保持 nullopt）退化为纯 PlayerManager 遍历/查询，与原实现一致。

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

            // 更新调试统计（批9：EMA 平滑 tick 耗时 + 强制区块计数下沉至基类
            // _updateTickDebugStats，EMA 状态由基类成员 m_smoothedTickTimeMs 跨 tick 保留）
            const f32 tickTimeMs = std::chrono::duration<f32, std::milli>(tickElapsed).count();
            _updateTickDebugStats(tickTimeMs);
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
        // stateId 由菜单自增（对齐 vanilla ContainerSynchronizer#sendInitialData 前调
        // menu.incrementStateId()）。stateId 是 mutable 同步令牌，const 菜单引用下可自增。
        mc::network::ir::play::ContainerSetContent pkt;
        pkt.containerId = static_cast<i32>(menu.getId());
        pkt.stateId = menu.incrementStateId();
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

// 注：远程会话四件套（_onRemoteClientConnect/_onRemotePlayerReady/
// _onRemoteClientDisconnect）已于批2c 下沉至 RemoteSessionManager 门面，门面成员
// m_remoteSessionManager 于批9 上提 MinecraftServer 基类。initialize() 经基类
// _setupRemoteSessions 构造并注册到 m_serverNetwork 的 onClientConnect/onClientDisconnect；
// stop() 经基类 _shutdownRemoteSessions 先 reset manager 再 reset ServerNetwork 保销毁顺序
// （session 持 ServerClientConnection& 引用）。

// 注：handleHotbarSelect/handleContainerClick/handleCloseContainer/handleOpenPlayerInventory
// 及 5 个 inventory 查询/操作（getHeldItemForPlacement/getSelectedHotbarSlot/setInventoryItem/
// syncPlayerInventory/tryOpenCraftingContainer）已于批9 下沉至 MinecraftServer 基类默认实现
// （纯远程路径）。StandaloneServer 为纯远程独立服，无本地客户端分支，直接继承基类默认即原
// StandaloneServer 行为，不再 override。handleOpenPlayerInventoryPacket 基类默认已校验
// PlayerCommand action==OPEN_INVENTORY（TODO(Phase6): 远程玩家打开背包菜单建立暂留空，
// 集成服本地路径有 _openItemPickerMenu/_openPlayerInventoryMenu）。

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
