#include "StandaloneServer.hpp"
#include "minecraft-reborn/version.h"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/network/TcpConnection.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/PacketHandler.hpp"

#include <spdlog/spdlog.h>
#include <thread>

namespace mc::server {

namespace {

[[nodiscard]] bool isCraftingTableState(const BlockState* state)
{
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

} // namespace

StandaloneServer::StandaloneServer()
    : MinecraftServer(ServerCoreConfig{})
{
}

StandaloneServer::~StandaloneServer()
{
    if (m_running) {
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
    String settingsPath = params.settingsPath.value_or(
        ServerSettings::getDefaultPath().string());
    auto settingsResult = loadSettings(settingsPath);
    if (settingsResult.failed()) {
        spdlog::warn("Failed to load settings from {}: {}. Using defaults.",
                     settingsPath, settingsResult.error().toString());
    }

    // 应用命令行覆盖
    if (params.port.has_value()) {
        m_settings.serverPort.set(*params.port);
    }
    if (params.bindAddress.has_value()) {
        m_settings.bindAddress.set(*params.bindAddress);
    }
    if (params.maxPlayers.has_value()) {
        m_settings.maxPlayers.set(static_cast<i32>(*params.maxPlayers));
    }
    if (params.worldName.has_value()) {
        m_settings.worldName.set(*params.worldName);
    }
    if (params.seed.has_value()) {
        m_settings.levelSeed.set(std::to_string(*params.seed));
    }

    // 应用设置到系统
    applySettings();

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

    spdlog::info("=== Minecraft Reborn Server ===");
    spdlog::info("Version: {}.{}.{}", MC_VERSION_MAJOR, MC_VERSION_MINOR, MC_VERSION_PATCH);
    spdlog::info("Initializing standalone server...");

    // 初始化性能追踪
    mc::perfetto::TraceConfig traceConfig;
    traceConfig.outputPath = "server_trace.perfetto-trace";
    traceConfig.bufferSizeKb = 65536; // 64MB
    mc::perfetto::PerfettoManager::instance().initialize(traceConfig);
    mc::perfetto::PerfettoManager::instance().startTracing();

    // 设置进程和主线程名称
    mc::perfetto::PerfettoManager::instance().setProcessName("MinecraftServer");
    mc::perfetto::PerfettoManager::instance().setThreadName("ServerMainThread");
    spdlog::info("Perfetto tracing initialized");

    // 初始化游戏注册表（包括实体类型）
    initializeRegistries(true);

    // 设置核心配置
    m_config.viewDistance = m_settings.viewDistance.get();
    m_config.maxPlayers = m_settings.maxPlayers.get();
    m_config.seed = static_cast<u64>(std::stoll(m_settings.levelSeed.get()));
    m_config.tickRate = 20;
    m_config.defaultGameMode = GameMode::Survival;

    // 初始化核心管理器
    initializeCoreManagers();

    // 创建世界
    ServerWorldConfig worldConfig;
    worldConfig.viewDistance = m_settings.viewDistance.get();
    worldConfig.dimension = 0;  // 主世界
    worldConfig.seed = static_cast<u64>(std::stoll(m_settings.levelSeed.get()));

    m_world = std::make_unique<ServerWorld>(worldConfig);

    // 设置 TimeManager 引用
    m_world->setTimeManager(m_timeManager.get());

    auto worldInitResult = m_world->initialize();
    if (worldInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to initialize world: " + worldInitResult.error().message());
    }

    // 创建区块管理器
    auto chunkGenerator = std::make_unique<NoiseChunkGenerator>(
        static_cast<u64>(std::stoll(m_settings.levelSeed.get())), DimensionSettings::overworld());
    auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(chunkGenerator));
    chunkManager->setViewDistance(m_settings.viewDistance.get());
    chunkManager->initialize();
    m_world->setChunkManager(std::move(chunkManager));

    // 创建光照管理器
    auto lightManager = std::make_unique<WorldLightManager>(m_world.get(), true, true);
    m_world->setLightManager(std::move(lightManager));

    // 初始化世界
    auto worldResult = initializeWorld();
    if (worldResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to initialize world: " + worldResult.error().message());
    }

    // 初始化交互管理器
    initializeInteractionManagers();

    // 容器网络回调：将 ContainerManager 事件转发为客户端协议包。
    containerManager().setOnContainerOpen([this](PlayerId playerId,
                                                 ContainerId containerId,
                                                 mc::ContainerType type,
                                                 const String& title,
                                                 i32 slotCount) {
        const String resolvedTitle = title.empty()
            ? String(ContainerTypes::getDefaultTitle(type))
            : title;

        network::PacketSerializer ser;
        auto packet = ContainerPacketHandler::createOpenContainerPacket(
            containerId,
            ContainerTypes::toNetworkType(type),
            resolvedTitle,
            slotCount);
        packet.serialize(ser);

        auto fullPacket = core::ConnectionManager::encapsulatePacket(
            network::PacketType::OpenContainer,
            ser.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    });

    containerManager().setOnContainerClose([this](PlayerId playerId, ContainerId containerId) {
        network::PacketSerializer ser;
        CloseContainerPacket packet(containerId);
        packet.serialize(ser);

        auto fullPacket = core::ConnectionManager::encapsulatePacket(
            network::PacketType::CloseContainer,
            ser.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    });

    containerManager().setOnContainerUpdate([this](PlayerId playerId, const AbstractContainerMenu& menu) {
        network::PacketSerializer ser;
        auto packet = ContainerPacketHandler::createContentPacket(menu);
        packet.serialize(ser);

        auto fullPacket = core::ConnectionManager::encapsulatePacket(
            network::PacketType::ContainerContent,
            ser.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    });

    // 初始化维度管理器
    auto dimInitResult = m_dimensionManager->initialize(
        static_cast<u64>(std::stoll(m_settings.levelSeed.get())),
        m_settings.viewDistance.get());
    if (dimInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to initialize dimension manager: " + dimInitResult.error().message());
    }

    // 初始化同步管理器
    initializeSyncManagers();

    // 初始化区块同步管理器
    initializeChunkSyncManagers();

    // 设置区块发送回调
    setupChunkSendCallback();

    // 设置世界回调（包括光照变化回调）
    setupWorldCallbacks();

    // 初始化网络服务器
    m_tcpServer = std::make_unique<TcpServer>();

    // 设置网络回调
    m_tcpServer->setOnConnect([this](TcpSession* session) {
        onClientConnect(session);
    });

    m_tcpServer->setOnDisconnect([this](TcpSession* session, const String& reason) {
        onClientDisconnect(session, reason);
    });

    m_tcpServer->setOnPacket([this](TcpSession* session, const u8* data, size_t size) {
        dispatchPacket(static_cast<u32>(session->id()), data, size);
    });

    // 启动服务器
    TcpServerConfig serverConfig;
    serverConfig.port = static_cast<u16>(m_settings.serverPort.get());
    serverConfig.maxConnections = static_cast<u32>(m_settings.maxPlayers.get());

    auto serverResult = m_tcpServer->start(serverConfig);
    if (serverResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to start server: " + serverResult.error().message());
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
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping server...");
    m_running = false;

    // 停止核心组件
    stopCore();

    // 关闭网络服务器
    if (m_tcpServer) {
        m_tcpServer->stop();
        m_tcpServer.reset();
    }

    // 保存设置
    const auto savePath = m_settingsPath.empty()
        ? ServerSettings::getDefaultPath()
        : m_settingsPath;
    auto saveResult = m_settings.saveSettings(savePath);
    if (saveResult.failed()) {
        spdlog::warn("Failed to save settings: {}", saveResult.error().toString());
    }

    // 关闭性能追踪
    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();
    spdlog::info("Perfetto tracing stopped");

    spdlog::info("Server stopped.");
}

void StandaloneServer::pollNetwork()
{
    MC_TRACE_EVENT("server.network", "PollNetwork");
    if (m_tcpServer) {
        m_tcpServer->poll();
    }
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

    try {
        mainLoop();
    } catch (const std::exception& e) {
        spdlog::critical("Server crashed: {}", e.what());
        m_running = false;
        return Error(ErrorCode::Unknown, e.what());
    }

    return Result<void>::ok();
}

void StandaloneServer::mainLoop()
{
    using clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;

    constexpr f64 targetTickTime = 1.0 / 20.0; // 20 TPS
    constexpr auto tickDuration = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<f64>(targetTickTime));

    auto lastTickTime = clock::now();

    spdlog::info("Server is now running!");
    spdlog::info("Connect with port: {}", m_settings.serverPort.get());

    while (m_running) {
        MC_TRACE_EVENT("server.tick", "MainLoopIteration");

        const auto currentTime = clock::now();
        const auto deltaTime = currentTime - lastTickTime;

        if (deltaTime >= tickDuration) {
            // 执行游戏刻
            tick();

            lastTickTime = currentTime;

            // 追踪 TPS
            const f64 tps = 1.0 / (std::chrono::duration<f64>(deltaTime).count());
            MC_TRACE_COUNTER("server.tick", "TPS", static_cast<i64>(tps));
            MC_TRACE_COUNTER("server.tick", "PlayerCount", static_cast<i64>(m_playerManager->playerCount()));

            // 每秒输出一次统计信息
            u64 tickCount = currentTick();
            if (tickCount % 20 == 0) {
                (void)tps;  // Avoid unused variable warning when SPDLOG_TRACE is disabled
                SPDLOG_TRACE("TPS: {:.1f}, Tick: {}", tps, tickCount);
            }
        } else {
            // 等待下一刻
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

Result<void> StandaloneServer::loadSettings(const String& path)
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

void StandaloneServer::applySettings()
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
        if (m_world) {
            auto config = m_world->config();
            config.viewDistance = value;
            m_world->setConfig(config);
        }
    });

    m_settings.logLevel.onChange([this](const String& value) {
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

void StandaloneServer::onClientConnect(TcpSession* session)
{
    spdlog::info("Client connected: {}:{}",
                 session->address(), session->port());
}

void StandaloneServer::onClientDisconnect(TcpSession* session, const String& reason)
{
    spdlog::info("Client disconnected: {}:{} - {}",
                 session->address(), session->port(), reason);

    // 移除玩家
    PlayerId playerId = m_playerManager->getPlayerIdBySession(
        static_cast<u32>(session->id()));
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
        sendLoginResponse(session.get(), false, 0, "", "Invalid login request");
        session->disconnect("Invalid login request");
        return;
    }

    auto& packet = result.value();
    String username = packet.username();

    spdlog::info("Player '{}' attempting to join from {}:{}",
                 username, session->address(), session->port());

    // 检查玩家数量限制
    if (m_playerManager->isFull()) {
        sendLoginResponse(session.get(), false, 0, username, "Server is full");
        session->disconnect("Server is full");
        return;
    }

    // 创建连接
    auto connection = std::make_shared<TcpConnection>(session);

    // 分配玩家ID并添加
    PlayerId playerId = m_playerManager->nextPlayerId();
    auto* player = m_playerManager->addPlayer(playerId, username, connection);

    if (!player) {
        sendLoginResponse(session.get(), false, 0, username, "Failed to add player");
        session->disconnect("Failed to add player");
        return;
    }

    // 设置会话ID并建立映射
    player->sessionId = sessionId;
    m_playerManager->mapSessionToPlayer(sessionId, playerId);

    // 更新会话状态
    session->setState(SessionState::Playing);

    // 设置玩家初始状态
    setupInitialPlayerState(player, m_config.defaultGameMode);

    // 初始化玩家物品栏
    inventoryManager().initializeInventory(playerId);

    // 发送登录成功响应
    sendLoginResponse(session.get(), true, playerId, username, "Welcome!");

    // 同步命令树
    sendCommandTreePacket(playerId);

    // 发送初始游戏状态
    sendInitialGameState(playerId, player->x, player->y, player->z, player->yaw, player->pitch);

    // 同步物品栏到客户端
    inventoryManager().syncToClient(playerId);

    spdlog::info("Player '{}' (ID: {}) joined the game", username, playerId);
}

void StandaloneServer::handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) return;

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerTryUseItemOnBlockPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse block placement packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    BlockPos pos(packet.x(), packet.y(), packet.z());
    const BlockState* clickedState = m_world ? m_world->getBlockState(pos) : nullptr;
    const Hand hand = (packet.hand() == static_cast<u8>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    const auto tryOpenCraftingContainer = [this, playerId, pos, clickedState]() {
        if (!isCraftingTableState(clickedState)) {
            return false;
        }

        auto openResult = containerManager().openContainer(playerId, mc::ContainerType::CraftingTable, pos);
        return openResult.success();
    };

    // 获取手持物品
    ItemStack heldStack = inventoryManager().getHeldItem(playerId);

    if (heldStack.isEmpty()) {
        if (!tryOpenCraftingContainer()) {
            (void)blockInteractionManager().handleBlockUse(
                playerId,
                pos,
                hand,
                packet.hitPosition(),
                packet.face());
        }
        return;
    }

    const Item* heldItem = heldStack.getItem();
    const bool holdingBlockItem =
        (heldItem != nullptr) &&
        (BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId()) != nullptr);

    if (!holdingBlockItem) {
        if (!tryOpenCraftingContainer()) {
            (void)blockInteractionManager().handleBlockUse(
                playerId,
                pos,
                hand,
                packet.hitPosition(),
                packet.face());
        }
        return;
    }

    auto interactionResult = blockInteractionManager().handleBlockPlacement(
        playerId, pos, packet.hitPosition(), packet.face(), heldStack);

    if (interactionResult.success() && interactionResult.value().blockPlaced) {
        // 更新物品栏
        if (interactionResult.value().itemConsumed) {
            i32 selectedSlot = inventoryManager().getSelectedSlot(playerId);
            ItemStack updatedStack = heldStack;
            updatedStack.shrink(1);
            inventoryManager().setItem(playerId, selectedSlot, updatedStack);
            inventoryManager().syncToClient(playerId);
        }
    }
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

    // 使用 InventoryManager 设置选中槽位
    inventoryManager().setSelectedSlot(playerId, result.value().slot());
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
    auto clickResult = containerManager().handleClick(
        playerId,
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

void StandaloneServer::sendLoginResponse(TcpSession* session, bool success,
                                          PlayerId playerId, const String& username,
                                          const String& message)
{
    network::LoginResponsePacket response(success, playerId, username, message);
    network::PacketSerializer ser;
    response.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::LoginResponse, ser.buffer());
    session->send(fullPacket.data(), fullPacket.size());
}

// ============================================================================
// 回调设置
// ============================================================================

void StandaloneServer::setupChunkSendCallback()
{
    // 设置区块发送回调 - 当区块数据准备好时发送给所有追踪的玩家
    chunkSendManager().setOnChunkSend([this](PlayerId playerId, ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (player && player->loggedIn && player->hasConnection()) {
            DimensionId dimension = 0;
            if (m_dimensionManager) {
                const DimensionId resolvedDimension = m_dimensionManager->getPlayerDimension(playerId);
                if (resolvedDimension >= 0) {
                    dimension = resolvedDimension;
                }
            }
            network::ChunkDataPacket packet(x, z, dimension, data);
            network::PacketSerializer ser;
            packet.serialize(ser);

            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::ChunkData, ser.buffer());
            player->send(fullPacket.data(), fullPacket.size());
            // spdlog::debug("StandaloneServer: Sent chunk ({}, {}) to player {}", x, z, playerId);
        }
    });

    // 设置区块卸载回调
    chunkSendManager().setOnChunkUnload([this](PlayerId playerId, ChunkCoord x, ChunkCoord z) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (player && player->loggedIn && player->hasConnection()) {
            DimensionId dimension = 0;
            if (m_dimensionManager) {
                const DimensionId resolvedDimension = m_dimensionManager->getPlayerDimension(playerId);
                if (resolvedDimension >= 0) {
                    dimension = resolvedDimension;
                }
            }
            network::UnloadChunkPacket packet(x, z, dimension);
            network::PacketSerializer ser;
            packet.serialize(ser);

            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::UnloadChunk, ser.buffer());
            player->send(fullPacket.data(), fullPacket.size());
            // spdlog::debug("StandaloneServer: Sent unload chunk ({}, {}) to player {}", x, z, playerId);
        }
    });
}

void StandaloneServer::broadcastLightUpdate(ChunkCoord x, ChunkCoord z, i32 sectionY,
                                             const std::vector<u8>& skyLight,
                                             const std::vector<u8>& blockLight,
                                             bool trustEdges)
{
    MC_TRACE_EVENT("server.lighting", "BroadcastLightUpdate",
               "Section", fmt::format("({}, {}, {})", x, sectionY, z),
               "SkyLightSize", skyLight.size(),
               "BlockLightSize", blockLight.size(),
               [flow = ::perfetto::Flow::ProcessScoped(SectionPos(x, sectionY, z).toLong())](::perfetto::EventContext ctx) {
                   flow(ctx);
    });

    network::LightUpdatePacket packet(x, z, sectionY,
                                       std::vector<u8>(skyLight),
                                       std::vector<u8>(blockLight),
                                       trustEdges);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::LightUpdate, ser.buffer());

    // 发送给所有在线玩家
    m_playerManager->forEachPlayer([&fullPacket](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            player.send(fullPacket.data(), fullPacket.size());
        }
    });
}

} // namespace mc::server
