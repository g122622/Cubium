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
#include "common/entity/inventory/ContainerTypeUtils.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/inventory/container/CrafterContainer.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/entity/inventory/container/ItemPickerMenu.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/blockentity/trial/CrafterBlockEntity.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/world/storage/core/WorldStoragePaths.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "common/world/storage/request/WorldRequests.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/TimeManager.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/network/LoginFlow.hpp"
#include "server/network/RemoteSessionManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/util/assert/AssertAll.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

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

/// 把 1.21.11 HashedStack（仅 itemId+count）还原为业务 ItemStack。
/// TODO(Phase6): HashedStack 暂无组件哈希 patch，仅还原基础物品+数量。
[[nodiscard]] ItemStack hashedStackToItemStack(const mc::network::ir::play::HashedStack& hashed)
{
    if (!hashed.present || hashed.itemId == 0 || hashed.count <= 0) {
        return ItemStack();
    }
    // wire 上的 hashed.itemId 是 vanilla registry id，须经 JavaItemIdMap 反查为项目内部 ItemId。
    const mc::ItemId internalItemId =
        mc::network::backend::java::JavaItemIdMap::instance().fromJavaRegistryId(hashed.itemId);
    auto* item = mc::ItemRegistry::instance().getItem(internalItemId);
    if (item == nullptr) {
        return ItemStack();
    }
    return ItemStack(*item, hashed.count);
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
    IntegratedServerParams params{
        .worldName = defaults::integratedServer::worldName,
        .gameDirectoryRoot = "",
        .displayName = "",
        .seed = defaults::integratedServer::seed,
        .defaultGameMode = GameMode::Survival,
        .viewDistance = defaults::integratedServer::viewDistance,
        .tickRate = defaults::integratedServer::tickRate,
        .worldType = WorldType::Default,
        .difficulty = Difficulty::Normal,
        .hardcore = false,
        .allowCommands = false,
        .isNewWorld = false,
    };
    return initialize(params);
}

Result<void> IntegratedServer::initialize(const IntegratedServerParams& params)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "IntegratedServer::initialize");

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

    // 世界生成 100% 数据驱动，注册表无硬编码兜底；数据包列表为空时各 worldgen loader 会
    // clear() 后加载 0 条目，致 RandomState::create 断言失败。原版 Minecraft 始终内置 vanilla
    // 数据包，此处镜像该语义：扫描到 0 包时从默认游戏目录注入原版数据包，保证 worldgen 注册表
    // 至少有原版条目可用。用户目录已放置自定义数据包时不干预（见 ensureVanillaBuiltinPack 守卫）。
    const auto vanillaDataPackDir = GameDirectory::defaultDirectory().dataPacksDir() / "Vanilla";
    if (m_dataPackList.ensureVanillaBuiltinPack(vanillaDataPackDir) > 0) {
        spdlog::info("Injected builtin vanilla data pack from '{}'", vanillaDataPackDir.string());
    }

    // 初始化游戏注册表
    initializeRegistries(false);

    spdlog::info("Initializing integrated server...");
    spdlog::info("World: {}, Seed: {}, View distance: {}", params.worldName, params.seed, params.viewDistance);

    // 创建服务端网络门面 + 本地客户端连接对（LocalTransport 零拷贝直传 ir::IrPacket）
    m_serverNetwork = std::make_unique<mc::server::net::ServerNetwork>();
    m_clientConnection = m_serverNetwork->createLocalClientSide(&m_pendingClientTransport);
    if (m_clientConnection == nullptr) {
        spdlog::error("IntegratedServer: failed to create local client connection pair");
        return Error(ErrorCode::InitializationFailed,
            "Local client connection pair creation failed",
            "IntegratedServer::initialize");
    }

    // 批2a：注入本地客户端发送钩子。基类 MinecraftServer::broadcastPacket/
    // sendPacketToPlayer 经 m_localClientSender 把包直传本地客户端（LocalTransport
    // 零拷贝），等价原 IntegratedServer 双路径中走 _sendToClientIr 的分支。
    // 显式按值拷贝 IrPacket（_sendToClientIr 按值接收并内部 move），避免钩子
    // 与远程遍历共用同一副本时的 use-after-move。m_localClientPlayerId 在
    // _onClientPlayerReady 内 set，此前钩子虽可发但 getPlayerIdForSession 的
    // sessionId==0 分支尚未命中（玩家未创建），与原行为一致。
    m_localClientSender = [this](const mc::network::ir::IrPacket& packet) {
        _sendToClientIr(mc::network::ir::IrPacket{packet});
    };

    // 创建本地客户端握手状态机（离线模式，集成服禁用压缩 threshold=-1）
    m_clientHandshake = std::make_unique<mc::server::net::ServerHandshakeStateMachine>(
        *m_clientConnection, /*isOfflineMode=*/true, /*compressionThreshold=*/-1);
    m_clientHandshake->onPlayerReady([this](const std::string& username, const std::array<u8, 16>& offlineUuid) {
        _onClientPlayerReady(username, offlineUuid);
    });

    // 初始化核心管理器
    // 【顺序约束】须先于 m_clientPlayRouter 构造：批7 起 ServerPlayRouter 持
    // ServerPlayHandler&（经 playHandler() 即 *m_playHandler 取引用），而 m_playHandler
    // 在 initializeCoreManagers 内才 make_unique。若 router 先于 init 构造，playHandler()
    // 会返回 *nullptr 形成空悬引用，运行期首个 Play 包经 router->route 解引用即崩
    // （表现：玩家 join 后第一个 AcceptTeleportation 包 ACCESS_VIOLATION read 0x0）。
    initializeCoreManagers();

    // 创建本地客户端 Play 路由器（sessionId=0）。批7：路由器改持 ServerPlayHandler& 门面。
    // 须在 initializeCoreManagers 之后：playHandler() 依赖 m_playHandler 已构造。
    m_clientPlayRouter =
        std::make_unique<mc::server::net::ServerPlayRouter>(playHandler(), m_clientPlayerId, /*sessionId=*/0);

    // 安装入站监听器：握手包交 ServerHandshake，Play 包交 ServerPlayRouter
    _installClientInboundListener();

    // 加载 OP 列表（集成服务器使用默认路径）
    // 白名单和封禁列表在集成服务器中通常不需要
    auto opsResult = m_opListManager->load("ops.json");
    if (opsResult.failed()) {
        spdlog::error("No ops.json found or failed to load: {}", opsResult.error().message());
    }

    // 新世界预写初始 level.dat：quick-play 与创建世界界面两条新世界路径不经过
    // WorldListService::createWorld，需在此补写，否则 loadLevelData 读不到 level.dat
    // 报错，且 updateRuntimeData 保存路径也依赖已存在的 level.dat 导致运行时数据无法持久化。
    if (m_params.isNewWorld) {
        // 复用 WorldStoragePaths::worldDir() 计算路径，与 WorldListService/GlobalStorageManager 一致
        world::storage::WorldStoragePaths storagePaths =
            world::storage::WorldStoragePaths::fromGameDirectory(m_gameDirectory);
        std::filesystem::path worldDir = storagePaths.worldDir(m_params.worldName);
        if (!std::filesystem::exists(worldDir / "level.dat")) {
            world::storage::CreateWorldRequest request(m_params.displayName,
                m_params.worldName,
                static_cast<u64>(m_params.seed),
                m_params.worldType,
                m_params.worldPresetId,
                m_params.defaultGameMode,
                m_params.difficulty,
                m_params.hardcore,
                m_params.allowCommands,
                m_params.viewDistance);
            auto initResult = world::storage::LevelDatCodec::writeInitial(worldDir, request);
            if (initResult.failed()) {
                spdlog::warn("Failed to write initial level.dat: {}", initResult.error().message());
            }
        }
    }

    auto storageInitResult = initializeSharedStorage(m_gameDirectory, params.worldName);
    if (storageInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
            "Failed to initialize shared world storage: " + storageInitResult.error().message());
    }

    // 初始化维度管理器。worldPresetId 由调用方显式传入 IntegratedServerParams（数据驱动装配查 WorldPresetRegistry）。
    auto dimInitResult = m_dimensionManager->initialize(
        static_cast<u64>(params.seed), params.viewDistance, params.worldType, params.worldPresetId);
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

void IntegratedServer::requestStop()
{
    MinecraftServer::requestStop();

    // 断开本地客户端连接（新网络层：m_clientConnection 由 m_serverNetwork 拥有）
    if (m_clientConnection != nullptr) {
        m_clientConnection->close();
    }

    // 通知局域网 TCP 监听器停止接受新连接：关 acceptor 由 m_serverNetwork 析构/stop 负责，
    // 此处无需额外操作（旧连接的清理在 stop() 中进行）。
}

void IntegratedServer::stop()
{
    // IntegratedServer 停止流程整体区间。stopCore()/shutdownManagers()/saveAllWorldData()
    // 等子阶段在各自函数内部有独立 trace，形成 slice 嵌套。
    // 注意：集成服运行在客户端进程内，trace 会话生命周期由客户端统一管理，
    // 本函数不关闭 Perfetto（与 StandaloneServer::stop() 不同）。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "IntegratedServer::stop");

    if (!m_initialized) {
        return;
    }

    spdlog::info("Stopping integrated server...");
    m_running = false;

    // 断开本地连接（新网络层：m_clientConnection 由 m_serverNetwork 拥有，关闭即触发对端 EOF）
    if (m_clientConnection != nullptr) {
        m_clientConnection->close();
    }

    // 等待服务端线程结束
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "IntegratedServer::stop::JoinServerThread");
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
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "IntegratedServer::stop::ClearPlayerEntities");
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

    m_lanPublished = false;
    m_lanPort = 0;

    // 释放本地客户端握手/Play 路由器（先于网络门面销毁）
    m_clientPlayRouter.reset();
    m_clientHandshake.reset();
    m_pendingClientTransport.reset();

    // 先清远程会话（session 持 ServerClientConnection& 引用，须先于连接销毁），
    // 再关闭服务端网络门面（含本地客户端 ServerClientConnection + LAN Wire 连接 +
    // acceptor + accept 线程）。批9：两步下沉至基类 _shutdownRemoteSessions（对未发布
    // LAN 的空 m_remoteSessionManager reset 幂等）。
    m_clientConnection = nullptr;
    _shutdownRemoteSessions();
    // 批2a：复位本地客户端钩子，避免关服后续路径误经悬垂 _sendToClientIr 调用
    // （钩子捕获 this，连接已销毁后 _sendToClientIr 内 m_clientConnection==nullptr
    // 本会安全跳过，此处显式复位消除该依赖）。
    m_localClientSender = nullptr;
    m_localClientPlayerId = std::nullopt;
    m_clientPlayerId = 0;
    m_clientEntityId = INVALID_ENTITY_ID;
    m_initialized = false;

    spdlog::info("Integrated server stopped");
}

std::unique_ptr<mc::network::transport::ILocalTransport> IntegratedServer::takeClientTransport()
{
    return std::move(m_pendingClientTransport);
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

    mc::profiler::ProfilerManager::instance().setThreadName("IntegratedServerThread");

    spdlog::info("Integrated server started ({} TPS)", m_settings.tickRate.get());

    // 更新目标每tick毫秒数
    m_debugStats.targetMsPerTick.store(static_cast<f32>(tickDuration.count()), std::memory_order::relaxed);

    while (m_running.load(std::memory_order::acquire)) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "MainLoopIteration");

        auto startTime = clock::now();

        tick();

        auto elapsed = clock::now() - startTime;
        const f32 tickTimeMs = std::chrono::duration<f32, std::milli>(elapsed).count();

        // 批9：EMA 平滑 tick 耗时 + 强制区块计数下沉至基类 _updateTickDebugStats
        // （与 StandaloneServer 共用同一实现，EMA 状态由基类成员 m_smoothedTickTimeMs 跨 tick
        // 保留）。本子类节流策略为可变 TPS（每轮必 tick + sleep 剩余），与 StandaloneServer
        // 固定 20 TPS 的 deltaTime 累积门控本质不同，主体循环保留子类。
        _updateTickDebugStats(tickTimeMs);

        auto sleepTime = tickDuration - elapsed;
        MC_TRACE_COUNTER(TraceEvents.Server.Tick, "ServerTickTime", static_cast<i64>(elapsed.count()));

        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

void IntegratedServer::tick()
{
    // 先驱动基类世界/实体/网络 tick（含熔炉方块实体 tick 更新燃烧/熔炼状态）
    MinecraftServer::tick();
    // 基类 tick 完成后同步打开容器的动态数据（熔炉进度下推客户端）
    _tickOpenContainer();
}

// 注：pollNetwork/broadcastPacket 已于批2a 统一为 MinecraftServer 基类默认实现，
// 本子类不再 override。本地客户端广播/发送经 initialize() 注入的 m_localClientSender
// 钩子（绑 _sendToClientIr）走 LocalTransport 零拷贝。

// ============================================================================
// 数据包处理
// ============================================================================

void IntegratedServer::_onClientPlayerReady(const std::string& username, const std::array<u8, 16>& offlineUuidArr)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "IntegratedServer::_onClientPlayerReady");

    spdlog::info("Player '{}' attempting to join (handshake complete)", username);

    // 白名单检查（白名单启用时拒绝不在名单中的玩家）
    if (m_whitelistManager->isEnabled() && !m_whitelistManager->isNameWhitelisted(username)) {
        spdlog::info("Player '{}' rejected: not in whitelist", username);
        // TODO(Phase6): 离线模式下握手已完成，拒绝需发 login::Disconnect 后断连。
        //     集成服单机默认不启用白名单，此处暂不处理。
        return;
    }

    // 共享玩家创建逻辑（分配 playerId、addPlayer、createPlayerEntity、权限、存档、play::Login、
    // 初始游戏状态）。本地客户端特有：路由器 setPlayerId + 创造背包初始化 + 物品栏同步。
    // 批6：登录流程整簇下沉至 LoginFlow 门面，经基类 loginFlow() 访问器进入。
    const bool isFlat = (m_params.worldType == WorldType::Flat);
    auto creation = loginFlow().createPlayerForConnection(
        *m_clientConnection, username, offlineUuidArr, m_params.hardcore, m_params.seed, isFlat);
    if (!creation.success) {
        return;
    }
    m_clientPlayerId = creation.playerId;
    m_clientEntityId = creation.entityId;
    // 批2a：回填基类本地客户端钩子的 playerId，使基类 broadcastPacket 跳过本地客户端
    // 避免双发、sendPacketToPlayer/getPlayerIdForSession 的本地分支命中。
    m_localClientPlayerId = m_clientPlayerId;
    if (m_clientPlayRouter != nullptr) {
        m_clientPlayRouter->setPlayerId(m_clientPlayerId);
    }

    // 初始化物品栏（本地客户端特有：创造模式给镐+全方块；生存留空）
    auto* playerData = m_playerManager->getPlayer(m_clientPlayerId);
    if (playerData != nullptr) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Player, "IntegratedServer::_onClientPlayerReady::InitInventory");

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
        _sendPlayerInventory();
    }
}

void IntegratedServer::_installClientInboundListener()
{
    if (m_clientConnection == nullptr) {
        return;
    }
    m_clientConnection->onPacket([this](const mc::network::ir::IrPacket& packet) {
        // 先交握手状态机：返回 true=握手范围内已消费；false=Play 包交路由器
        if (m_clientHandshake != nullptr) {
            auto r = m_clientHandshake->handleInbound(packet);
            if (!r.success()) {
                spdlog::error("IntegratedServer: handshake inbound failed: {}", r.error().toString());
                return;
            }
            if (r.value()) {
                return; // 握手/Configuration 包已消费
            }
        }
        // Play 阶段包交路由器（sessionId=0 本地客户端）
        if (m_clientPlayRouter != nullptr) {
            auto r = m_clientPlayRouter->handle(packet);
            if (!r.success()) {
                spdlog::error("IntegratedServer: play router failed: {}", r.error().toString());
            }
        }
    });
}

void IntegratedServer::handleHotbarSelectPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "HandleHotbarSelect");

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::SetCarriedItem>(&play);
    if (evt == nullptr) {
        return;
    }
    const i32 slot = evt->slot;

    // 远程 TCP 玩家：走 InventoryManager 多玩家路径（批9 下沉至基类 _handleHotbarSelectRemote）
    if (playerId != m_clientPlayerId) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (!player || !player->loggedIn) {
            return;
        }
        _handleHotbarSelectRemote(playerId, slot);
        return;
    }

    // 本地客户端
    auto* player = _getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    m_clientInventory.setSelectedSlot(slot);
}

void IntegratedServer::handleContainerClickPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "HandleContainerClick");

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ContainerClick>(&play);
    if (evt == nullptr) {
        return;
    }

    const ItemStack cursorItem = hashedStackToItemStack(evt->carriedItem);

    // 远程 TCP 玩家：走 ContainerManager 多玩家路径（批9 下沉至基类 _handleContainerClickRemote）
    if (playerId != m_clientPlayerId) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (!player || !player->loggedIn) {
            return;
        }
        _handleContainerClickRemote(playerId, *evt, cursorItem);
        return;
    }

    // 本地客户端：内联容器点击三突变（与 ContainerManager::handleClick 等价），
    // 不再重建旧 ContainerClickPacket。本地路径用 m_openMenu（非 ContainerManager 的多玩家 map）。
    // TODO(Phase6): 容器点击协议应直接对齐 1.21.11 ContainerClick(stateId+changedSlots)。
    auto* player = _getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    if (!m_openMenu || evt->containerId != m_openMenu->getId()) {
        return;
    }
    m_openMenu->setCarriedItem(cursorItem);
    const ClickType clickType = ContainerTypes::toClickType(static_cast<ClickAction>(evt->clickType), evt->buttonNum);
    Player& menuPlayer = _getMenuPlayer();
    m_openMenu->clicked(evt->slotNum, evt->buttonNum, clickType, menuPlayer);

    _sendContainerContent(*m_openMenu);
    _sendPlayerInventory();
}

void IntegratedServer::handleCloseContainerPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "HandleCloseContainer");

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ContainerClose>(&play);
    if (evt == nullptr) {
        return;
    }

    // 远程 TCP 玩家：走 ContainerManager 多玩家路径（批9 下沉至基类 _handleCloseContainerRemote）
    if (playerId != m_clientPlayerId) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (!player || !player->loggedIn) {
            return;
        }
        _handleCloseContainerRemote(playerId);
        return;
    }

    // 本地客户端
    auto* player = _getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    if (!m_openMenu || evt->containerId != m_openMenu->getId()) {
        return;
    }

    _closeCurrentContainer(false);
    _sendPlayerInventory();
}

void IntegratedServer::handleOpenPlayerInventoryPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "HandleOpenPlayerInventory");

    // 入站为 PlayerCommand{action=OPEN_INVENTORY}，无额外负载需校验
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::PlayerCommand>(&play);
    if (evt == nullptr || evt->action != 5) { // OPEN_INVENTORY
        return;
    }

    // 远程 TCP 玩家：暂沿用 ContainerManager 现有行为（Player 类型未注册菜单工厂）
    if (playerId != m_clientPlayerId) {
        return;
    }

    // 本地客户端：在 containerId=0 上建立容器菜单，使后续
    // ContainerClick 包能被正确受理（修复历史点击静默丢弃问题）。
    // 创造模式建立 ItemPickerMenu（承载创造取物 clone 协议），生存/冒险模式建立
    // InventoryCraftingMenu（普通背包合成）。
    auto* player = _getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    if (player->gameMode == GameMode::Creative) {
        _openItemPickerMenu();
    } else {
        _openPlayerInventoryMenu();
    }
}

// ============================================================================
// 数据包发送
// ============================================================================

void IntegratedServer::_sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
        "IntegratedServer::_sendTeleport",
        "x",
        x,
        "y",
        y,
        "z",
        z,
        "teleportId",
        teleportId);

    // play::PlayerPosition（S→C，id=70）。旧协议绝对传送；IR relatives=0 表全绝对。
    mc::network::ir::play::PlayerPosition pp;
    pp.teleportId = static_cast<i32>(teleportId);
    pp.x = x;
    pp.y = y;
    pp.z = z;
    pp.deltaX = 0.0;
    pp.deltaY = 0.0;
    pp.deltaZ = 0.0;
    pp.yRot = yaw;
    pp.xRot = pitch;
    pp.relatives = 0;
    _sendToClientIr(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(pp)}});
}

void IntegratedServer::_sendPlayerInventory()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "IntegratedServer::_sendPlayerInventory");

    // play::ContainerSetContent（containerId=0 玩家主物品栏，S→C，id=18）
    mc::network::ir::play::ContainerSetContent content;
    content.containerId = 0;
    content.stateId = 0; // TODO(Phase6): 状态 id 同步（当前无 stateId 追踪）
    const i32 invSize = m_clientInventory.getContainerSize();
    content.items.reserve(invSize);
    for (i32 i = 0; i < invSize; ++i) {
        content.items.push_back(mc::network::ir::toItemStackView(m_clientInventory.getItem(i)));
    }
    // carriedItem 为空
    content.carriedItem = mc::network::ir::play::ItemStackView{};
    _sendToClientIr(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(content)}});
}

void IntegratedServer::_sendContainerContent(const AbstractContainerMenu& menu)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Network, "IntegratedServer::_sendContainerContent", "containerId", menu.getId());

    if (m_clientConnection == nullptr || !m_clientConnection->isConnected()) {
        return;
    }

    // play::ContainerSetContent（containerId=menu.id）。slots 来自菜单容器视图。
    mc::network::ir::play::ContainerSetContent content;
    content.containerId = static_cast<i32>(menu.getId());
    content.stateId = 0;
    const i32 slotCount = menu.getSlotCount();
    content.items.reserve(slotCount);
    for (i32 i = 0; i < slotCount; ++i) {
        const auto* slot = menu.getSlot(i);
        if (slot != nullptr) {
            content.items.push_back(mc::network::ir::toItemStackView(slot->getItem()));
        } else {
            content.items.push_back(mc::network::ir::play::ItemStackView{});
        }
    }
    content.carriedItem = mc::network::ir::play::ItemStackView{};
    _sendToClientIr(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(content)}});
}

void IntegratedServer::_sendOpenContainer(
    ContainerId containerId, mc::ContainerType type, const std::string& title, i32 slotCount)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
        "IntegratedServer::_sendOpenContainer",
        "containerId",
        containerId,
        "type",
        static_cast<i32>(type));

    (void)slotCount; // slotCount 不再发送到客户端
    if (m_clientConnection == nullptr || !m_clientConnection->isConnected()) {
        return;
    }

    // play::OpenScreen（S→C，id=57）
    mc::network::ir::play::OpenScreen screen;
    screen.containerId = static_cast<i32>(containerId);
    screen.menuType = static_cast<i32>(ContainerTypes::toNetworkType(type));
    screen.title = title; // JSON 文本组件（客户端按 JSON 解析）
    _sendToClientIr(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(screen)}});
}

void IntegratedServer::_sendCloseContainer(ContainerId containerId)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Network, "IntegratedServer::_sendCloseContainer", "containerId", containerId);

    if (m_clientConnection == nullptr || !m_clientConnection->isConnected()) {
        return;
    }

    // play::ContainerClose（S→C，id=17）
    mc::network::ir::play::ContainerClose close;
    close.containerId = static_cast<i32>(containerId);
    _sendToClientIr(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(close)}});
}

void IntegratedServer::_sendToClientIr(mc::network::ir::IrPacket packet)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "IntegratedServer::_sendToClientIr");

    if (m_clientConnection != nullptr && m_clientConnection->isConnected()) {
        auto r = m_clientConnection->send(std::move(packet));
        if (!r.success()) {
            spdlog::warn("IntegratedServer: send to client failed: {}", r.error().toString());
        }
    }
}

void IntegratedServer::_sendBlockBreakAnim(EntityInstanceId breakerId, i32 x, i32 y, i32 z, i8 stage)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network,
        "IntegratedServer::_sendBlockBreakAnim",
        "breakerId",
        breakerId,
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "stage",
        stage);

    // play::BlockDestruction（S→C，id=5）：id=破坏者实体 + blockPosPacked + progress
    mc::network::ir::play::BlockDestruction bd;
    bd.id = static_cast<i32>(breakerId);
    bd.blockPosPacked = BlockPos(x, y, z).asLong();
    bd.progress = static_cast<u8>(stage);
    _sendToClientIr(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(bd)}});
}

void IntegratedServer::_sendWindowProperty(ContainerId containerId, i16 property, i16 value)
{
    if (m_clientConnection == nullptr || !m_clientConnection->isConnected()) {
        return;
    }

    // play::ContainerSetData（S→C，id=19，原 WindowProperty）
    mc::network::ir::play::ContainerSetData data;
    data.containerId = static_cast<i32>(containerId);
    data.property = property;
    data.value = value;
    _sendToClientIr(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(data)}});
}

i32 IntegratedServer::_registerFurnaceIntListener(AbstractContainerMenu& menu)
{
    auto* furnaceMenu = dynamic_cast<blockentity::FurnaceContainer*>(&menu);
    if (furnaceMenu == nullptr) {
        return -1;
    }
    const ContainerId containerId = furnaceMenu->getId();
    return furnaceMenu->addIntListener([this, containerId](i32 property, i32 value) {
        _sendWindowProperty(containerId, static_cast<i16>(property), static_cast<i16>(value));
    });
}

void IntegratedServer::_tickOpenContainer()
{
    if (!m_openMenu) {
        return;
    }
    auto* furnaceMenu = dynamic_cast<blockentity::FurnaceContainer*>(m_openMenu.get());
    if (furnaceMenu == nullptr) {
        return;
    }
    // 从熔炉方块实体刷新燃烧/熔炼进度到菜单 tracked int 的独立存储，
    // detectAndSendChanges 检测变化经 int 监听器发 WindowPropertyPacket 下推客户端。
    furnaceMenu->syncProgressFromEntity();
    furnaceMenu->detectAndSendChanges();
}

i32 IntegratedServer::resolveOpLevel(const std::string& uuid) const noexcept
{
    const i32 base = static_cast<i32>(m_opListManager->getLevel(uuid));

    // 主机身份动态判定：m_clientPlayerId 登录后非 0，playerManager 持有其 UUID。
    // 复用 isSingleplayerOwner 的 playerId 判定，再比对 uuid（本方法接收 uuid 而非 playerId）。
    bool isOwner = false;
    if (m_clientPlayerId != 0 && m_playerManager != nullptr) {
        const auto* host = m_playerManager->getPlayer(m_clientPlayerId);
        isOwner = (host != nullptr && host->uuid == uuid);
    }

    return core::applyOwnerCheatsBoost(base, isOwner, m_params.allowCommands);
}

bool IntegratedServer::openContainerRequest(ContainerType type, const BlockPos& pos, Player& player)
{
    // 远程 TCP 玩家：走 ContainerManager 多玩家路径
    if (player.playerId() != m_clientPlayerId) {
        return containerManager().openContainer(player.playerId(), type, pos).success();
    }

    // 本地客户端
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

    // 熔炉菜单注册 tracked int 监听器：进度变化时发 WindowPropertyPacket 下推客户端
    if (m_openMenu) {
        m_furnaceIntListenerId = _registerFurnaceIntListener(*m_openMenu);
        // 立即同步一次，使打开首帧火焰/箭头进度正确（不等首个 tick）
        _tickOpenContainer();
    }

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

    // 移除熔炉 tracked int 监听器（若已注册）
    if (m_furnaceIntListenerId >= 0 && m_openMenu) {
        m_openMenu->removeIntListener(m_furnaceIntListenerId);
        m_furnaceIntListenerId = -1;
    }

    m_openMenu.reset();
    m_openInventoryOwner.reset();
    m_openContainerType = ContainerType::Player;
    m_openContainerPos = BlockPos();
}

void IntegratedServer::_openCraftingTableMenu()
{
    (void)_openContainerMenu(ContainerType::Crafting, BlockPos());
}

void IntegratedServer::_openPlayerInventoryMenu()
{
    // 关闭任何已打开的容器（如箱子/工作台），避免菜单状态串扰
    _closeCurrentContainer(true);

    // 绑定菜单玩家到 m_clientInventory 并同步游戏模式（见 _openItemPickerMenu 说明），
    // 避免 AbstractContainerMenu 点击逻辑解引用空玩家。
    Player& menuPlayer = _getMenuPlayer();
    if (auto* playerData = _getPlayerData()) {
        menuPlayer.setGameMode(playerData->gameMode);
    }
    m_clientInventory.setPlayer(&menuPlayer);

    // 玩家背包使用固定 containerId=0（PLAYER_CONTAINER_ID），不复用自增计数器
    auto menu = std::make_unique<InventoryCraftingMenu>(mc::inventory::PLAYER_CONTAINER_ID, &m_clientInventory);

    // 不发 OpenContainerPacket：客户端已本地构造 InventoryScreen。
    // 仅同步容器内容 + 玩家背包，使客户端菜单槽位与服务端一致。
    _sendContainerContent(*menu);
    _sendPlayerInventory();

    m_openContainerType = ContainerType::Player;
    m_openContainerPos = BlockPos();
    m_openMenu = std::move(menu);
    menuPlayer.setOpenContainerMenu(m_openMenu.get());
}

void IntegratedServer::_openItemPickerMenu()
{
    // 关闭任何已打开的容器，避免菜单状态串扰
    _closeCurrentContainer(true);

    // 绑定菜单玩家到 m_clientInventory：AbstractContainerMenu 的点击逻辑经
    // m_playerInventory->getPlayer() 取玩家（如 _handleClickPick 解引用玩家调
    // ArmorSlot::mayPickup），m_clientInventory 默认构造 m_player 为空会导致空引用
    // 崩溃。同时同步菜单玩家游戏模式为创造，使 ArmorSlot 等的 isCreative() 判定正确。
    Player& menuPlayer = _getMenuPlayer();
    menuPlayer.setGameMode(GameMode::Creative);
    m_clientInventory.setPlayer(&menuPlayer);

    // 创造模式背包复用 containerId=0（PLAYER_CONTAINER_ID），但建立 ItemPickerMenu
    // 承载创造取物协议：调色板点击经 ContainerClick(ClickAction::Clone) 进
    // menu->clicked 的虚拟槽分支，服务端从本地创造物品池 clone 到光标，再经
    // ContainerSetContent 包的 carried 字段回传客户端。
    auto menu = std::make_unique<ItemPickerMenu>(mc::inventory::PLAYER_CONTAINER_ID, &m_clientInventory);

    _sendContainerContent(*menu);
    _sendPlayerInventory();

    m_openContainerType = ContainerType::Player;
    m_openContainerPos = BlockPos();
    m_openMenu = std::move(menu);
    menuPlayer.setOpenContainerMenu(m_openMenu.get());
}

void IntegratedServer::onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory)
{
    MC_UNUSED(playerId);
    MC_UNUSED(inventory);
}

ItemStack IntegratedServer::getHeldItemForPlacement(PlayerId playerId)
{
    // 远程 TCP 玩家：走 InventoryManager（批9 下沉至基类默认实现，显式转发零重复）
    if (playerId != m_clientPlayerId) {
        return MinecraftServer::getHeldItemForPlacement(playerId);
    }
    return m_clientInventory.getSelectedStack();
}

i32 IntegratedServer::getSelectedHotbarSlot(PlayerId playerId)
{
    // 远程 TCP 玩家：走 InventoryManager（批9 下沉至基类默认实现，显式转发零重复）
    if (playerId != m_clientPlayerId) {
        return MinecraftServer::getSelectedHotbarSlot(playerId);
    }
    return m_clientInventory.getSelectedSlot();
}

void IntegratedServer::setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack)
{
    // 远程 TCP 玩家：走 InventoryManager（批9 下沉至基类默认实现，显式转发零重复）
    if (playerId != m_clientPlayerId) {
        MinecraftServer::setInventoryItem(playerId, slotIndex, stack);
        return;
    }
    m_clientInventory.setItem(slotIndex, stack);
}

void IntegratedServer::syncPlayerInventory(PlayerId playerId)
{
    // 远程 TCP 玩家：走 InventoryManager（批9 下沉至基类默认实现，显式转发零重复）
    if (playerId != m_clientPlayerId) {
        MinecraftServer::syncPlayerInventory(playerId);
        return;
    }
    _sendPlayerInventory();
}

bool IntegratedServer::tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos)
{
    // 远程 TCP 玩家：走 ContainerManager（批9 下沉至基类默认实现，显式转发零重复）
    if (playerId != m_clientPlayerId) {
        return MinecraftServer::tryOpenCraftingContainer(playerId, pos);
    }

    // 本地客户端
    MC_UNUSED(pos);
    _openCraftingTableMenu();
    return true;
}

// ============================================================================
// 局域网发布（TCP 监听器）与远程玩家处理
// ============================================================================

Result<void> IntegratedServer::publishToLan(i32 port, bool allowCheats)
{
    // 端口范围校验
    if (port < 1 || port > 65535) {
        return Error(ErrorCode::InvalidArgument, "Port must be between 1 and 65535");
    }

    // 已发布检查
    if (m_lanPublished) {
        return Error(ErrorCode::AlreadyExists, "Server already published to LAN");
    }

    // 单一 m_serverNetwork 同时供 Local（已在 initialize() 建好本地客户端连接对）与 LAN Wire。
    // 批9：远程会话四件套装配 + startAccept 下沉至基类 _setupRemoteSessions。worldParams 从
    // m_params 取（集成服世界参数权威来源）：hardcore/seed/worldType==Flat。本地客户端
    // sessionId=0 在 initialize() 内联接线，不经此 manager。startAccept 失败时基类透传原始
    // Error，此处按 LAN 场景包装日志前缀。
    const u32 maxConnections = static_cast<u32>(std::max(m_integratedSettings.maxPlayers.get(), 8));
    auto setupResult = _setupRemoteSessions(
        "IntegratedServer",
        kLanCompressionThreshold,
        [this]() -> mc::server::net::RemoteWorldParams {
            return {m_params.hardcore, m_params.seed, m_params.worldType == WorldType::Flat};
        },
        static_cast<u16>(port),
        maxConnections);
    if (setupResult.failed()) {
        return Error(ErrorCode::InitializationFailed, "Failed to start LAN server: " + setupResult.error().message());
    }

    // 运行时切换作弊开关（不修改 level.dat 中的 allowCommands）
    // 这样关闭局域网发布后，作弊状态会恢复到原始设置
    m_params.allowCommands = allowCheats;

    m_lanPublished = true;
    m_lanPort = port;

    spdlog::info(
        "Integrated server published to LAN on port {} (cheats: {})", port, allowCheats ? "enabled" : "disabled");
    return Result<void>::ok();
}

// 注：远程会话四件套（_onRemoteClientConnect/_onRemotePlayerReady/
// _onRemoteClientDisconnect）已于批2c 下沉至 RemoteSessionManager 门面，门面成员
// m_remoteSessionManager 于批9 上提 MinecraftServer 基类。publishToLan() 经基类
// _setupRemoteSessions 装配门面并注册到 m_serverNetwork 的
// onClientConnect/onClientDisconnect；stop() 经基类 _shutdownRemoteSessions 先 reset
// manager 再 reset ServerNetwork 保销毁顺序（session 持 ServerClientConnection& 引用）。

} // namespace mc::server
