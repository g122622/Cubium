#include "IntegratedServer.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/BlockBreakAnimPacket.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/core/TimeManager.hpp"

#include <spdlog/spdlog.h>
#include "common/util/assert/AssertAll.hpp"

namespace mc::server {

namespace {

/**
 * @brief 发送游戏数据包到本地端点
 */
template <typename PacketT>
void sendGamePacket(network::LocalEndpoint* endpoint, network::PacketType packetType, const PacketT& packet) {
    if (endpoint == nullptr || !endpoint->isConnected()) {
        return;
    }

    network::PacketSerializer payload;
    packet.serialize(payload);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(packetType, payload.buffer());
    endpoint->send(fullPacket.data(), fullPacket.size());
}

/**
 * @brief 检查方块状态是否为工作台
 */
bool isCraftingTableState(const BlockState* state) {
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

/**
 * @brief 获取菜单玩家（临时方案）
 */
Player& getMenuPlayer() {
    static Player player(0, "IntegratedServerMenu");
    return player;
}

} // namespace

IntegratedServer::IntegratedServer()
    : MinecraftServer(ServerCoreConfig{})
{
}

IntegratedServer::~IntegratedServer()
{
    if (m_running) {
        stop();
    }
}

Result<void> IntegratedServer::initialize()
{
    return initialize(IntegratedServerConfig{});
}

Result<void> IntegratedServer::initialize(const IntegratedServerConfig& config)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Server already initialized");
    }

    // 保存集成服务器特有配置
    m_integratedConfig = config;

    // 设置核心配置
    m_config.viewDistance = config.viewDistance;
    m_config.defaultGameMode = config.defaultGameMode;
    m_config.seed = static_cast<u64>(config.seed);
    m_config.maxPlayers = 1;  // 内置服务器只支持单人
    m_config.tickRate = config.tickRate;

    // 初始化游戏注册表
    initializeRegistries(false);

    spdlog::info("Initializing integrated server...");
    spdlog::info("World: {}, Seed: {}, View distance: {}",
                 config.worldName, config.seed, config.viewDistance);

    // 创建本地连接对
    m_connectionPair = std::make_unique<network::LocalConnectionPair>();
    m_connectionPair->connect();
    m_serverEndpoint = &m_connectionPair->serverEndpoint();

    // 初始化核心管理器
    initializeCoreManagers();

    // 创建世界
    ServerWorldConfig worldConfig;
    worldConfig.viewDistance = config.viewDistance;
    worldConfig.dimension = 0;  // 主世界
    worldConfig.seed = static_cast<u64>(config.seed);
    worldConfig.isDebugWorld = (config.worldType == WorldType::Debug);

    m_world = std::make_unique<ServerWorld>(worldConfig);

    // 设置 TimeManager 引用
    m_world->setTimeManager(m_timeManager.get());

    auto worldInitResult = m_world->initialize();
    if (worldInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to initialize world: " + worldInitResult.error().message());
    }

    // 根据世界类型创建区块管理器
    std::unique_ptr<IChunkGenerator> chunkGenerator;
    switch (config.worldType) {
        case WorldType::Debug:
            spdlog::info("Using DebugChunkGenerator for debug world");
            chunkGenerator = std::make_unique<DebugChunkGenerator>();
            break;
        case WorldType::Flat:
            spdlog::info("Using flat world settings");
            chunkGenerator = std::make_unique<NoiseChunkGenerator>(
                static_cast<u64>(config.seed), DimensionSettings::flat());
            break;
        case WorldType::LargeBiomes:
            spdlog::info("Using large biomes world settings");
            chunkGenerator = std::make_unique<NoiseChunkGenerator>(
                static_cast<u64>(config.seed),
                DimensionSettings::overworld(),
                std::make_unique<LayerBiomeProvider>(static_cast<u64>(config.seed), true));
            break;
        case WorldType::Amplified:
            spdlog::info("Using amplified world settings");
            {
                DimensionSettings amplifiedSettings = DimensionSettings::overworld();
                amplifiedSettings.noise = NoiseSettings::amplified();
                chunkGenerator = std::make_unique<NoiseChunkGenerator>(
                    static_cast<u64>(config.seed), std::move(amplifiedSettings));
            }
            break;
        case WorldType::Default:
        default:
            spdlog::info("Using NoiseChunkGenerator for normal world");
            chunkGenerator = std::make_unique<NoiseChunkGenerator>(
                static_cast<u64>(config.seed), DimensionSettings::overworld());
            break;
    }
    auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(chunkGenerator));
    chunkManager->setViewDistance(config.viewDistance);
    chunkManager->initialize();
    m_world->setChunkManager(std::move(chunkManager));

    // 创建光照管理器
    auto lightManager = std::make_unique<WorldLightManager>(m_world.get(), true, true);
    m_world->setLightManager(std::move(lightManager));

    // 初始化世界
    auto worldResult = initializeWorld();
    if (worldResult.failed()) {
        return worldResult;
    }

    // 调试模式特殊初始化
    if (config.worldType == WorldType::Debug) {
        spdlog::info("Configuring debug world special settings...");

        // 设置游戏模式为旁观者
        m_config.defaultGameMode = GameMode::Spectator;

        // 禁用日光周期，设置时间为正午（6000）
        if (m_timeManager) {
            m_timeManager->setDayTime(6000);  // 正午
            m_timeManager->setDaylightCycleEnabled(false);
            spdlog::info("Debug world: Time set to noon (6000), daylight cycle disabled");
        }

        // 禁用天气（晴朗）
        if (m_world->weatherManager()) {
            m_world->weatherManager()->setClear(999999999);  // 长时间晴天
            spdlog::info("Debug world: Weather set to clear");
        }
    }

    // 初始化交互管理器
    initializeInteractionManagers();

    // 初始化维度管理器
    auto dimInitResult = m_dimensionManager->initialize(static_cast<u64>(config.seed), config.viewDistance);
    if (dimInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to initialize dimension manager: " + dimInitResult.error().message());
    }

    // 初始化同步管理器
    initializeSyncManagers();

    // 初始化区块同步管理器（需要 world 已初始化）
    initializeChunkSyncManagers();

    // 设置区块发送回调（子类特有）
    setupChunkSendCallback();

    // 设置世界回调（包括光照变化回调）
    setupWorldCallbacks();

    // 启动服务端线程
    m_running = true;
    m_serverThread = std::make_unique<std::thread>([this]() {
        mainLoop();
    });

    m_initialized = true;
    spdlog::info("Integrated server initialized");
    return Result<void>::ok();
}

void IntegratedServer::shutdown()
{
    stop();
}

void IntegratedServer::stop()
{
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping integrated server...");
    m_running = false;

    // 停止核心组件
    stopCore();

    // 断开连接
    if (m_connectionPair) {
        m_connectionPair->disconnect();
    }

    // 等待服务端线程结束
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    m_serverThread.reset();

    // 释放客户端连接
    m_clientConnection.reset();

    // 关闭连接对
    if (m_connectionPair) {
        m_connectionPair.reset();
    }
    m_serverEndpoint = nullptr;

    spdlog::info("Integrated server stopped");
}

network::LocalEndpoint* IntegratedServer::getClientEndpoint()
{
    if (m_connectionPair) {
        return &m_connectionPair->clientEndpoint();
    }
    return nullptr;
}

void IntegratedServer::mainLoop()
{
    using clock = std::chrono::steady_clock;
    const auto tickDuration = std::chrono::milliseconds(1000 / m_config.tickRate);

    mc::perfetto::PerfettoManager::instance().setThreadName("IntegratedServerThread");

    spdlog::info("Integrated server started ({} TPS)", m_config.tickRate);

    while (m_running.load(std::memory_order_acquire)) {
        MC_TRACE_EVENT("server.tick", "MainLoopIteration");

        auto startTime = clock::now();

        tick();

        auto elapsed = clock::now() - startTime;
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
    sendToClient(data, size);
}

// ============================================================================
// 数据包处理
// ============================================================================

void IntegratedServer::handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size)
{
    (void)sessionId;
    MC_TRACE_EVENT("server.network", "HandleLoginRequest");

    network::PacketDeserializer deser(data, size);
    auto result = network::LoginRequestPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::warn("Failed to parse login request");
        sendLoginResponse(false, 0, "", "Invalid login request");
        return;
    }

    auto& packet = result.value();
    String username = packet.username();

    spdlog::info("Player '{}' attempting to join", username);

    // 创建本地连接
    m_clientConnection = std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());

    // 添加玩家
    m_clientPlayerId = m_playerManager->nextPlayerId();
    auto* player = m_playerManager->addPlayer(m_clientPlayerId, username, m_clientConnection);

    if (!player) {
        sendLoginResponse(false, 0, username, "Failed to add player");
        return;
    }

    // 设置玩家初始状态
    setupInitialPlayerState(player, m_config.defaultGameMode);

    // 初始化物品栏
    m_clientInventory.clear();
    m_clientInventory.setSelectedSlot(0);

    if (player->gameMode == GameMode::Creative) {
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

    // 发送登录成功响应
    sendLoginResponse(true, m_clientPlayerId, username, "Welcome to singleplayer world!");

    // 发送初始游戏状态
    sendInitialGameState(m_clientPlayerId, player->x, player->y, player->z, player->yaw, player->pitch);
    sendPlayerInventory();

    spdlog::info("Player '{}' (ID: {}) joined the game", username, m_clientPlayerId);
}

void IntegratedServer::handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;  // 单玩家模式，playerId 总是 m_clientPlayerId
    MC_TRACE_EVENT("server.network", "HandleBlockPlacement");

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerTryUseItemOnBlockPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse block placement packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    const BlockPos clickedPos(packet.x(), packet.y(), packet.z());
    const BlockState* clickedState = m_world ? m_world->getBlockState(clickedPos.x, clickedPos.y, clickedPos.z) : nullptr;
    const Hand hand = (packet.hand() == static_cast<u8>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    const auto tryOpenCraftingMenu = [this, clickedState]() {
        if (isCraftingTableState(clickedState)) {
            openCraftingTableMenu();
            return true;
        }
        return false;
    };

    // 获取手持物品
    ItemStack heldStack = m_clientInventory.getSelectedStack();
    if (heldStack.isEmpty()) {
        // 空手右键时优先尝试交互方块（例如工作台）。
        if (!tryOpenCraftingMenu()) {
            (void)blockInteractionManager().handleBlockUse(
                m_clientPlayerId,
                clickedPos,
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
        if (!tryOpenCraftingMenu()) {
            (void)blockInteractionManager().handleBlockUse(
                m_clientPlayerId,
                clickedPos,
                hand,
                packet.hitPosition(),
                packet.face());
        }
        return;
    }

    auto placementResult = blockInteractionManager().handleBlockPlacement(
        m_clientPlayerId,
        clickedPos,
        packet.hitPosition(),
        packet.face(),
        heldStack);

    if (placementResult.success() && placementResult.value().blockPlaced) {
        sendBlockUpdate(placementResult.value().position.x,
                       placementResult.value().position.y,
                       placementResult.value().position.z,
                       placementResult.value().newBlockStateId);

        // 更新物品栏
        if (placementResult.value().itemConsumed) {
            i32 selectedSlot = m_clientInventory.getSelectedSlot();
            ItemStack updatedStack = m_clientInventory.getItem(selectedSlot);
            updatedStack.shrink(1);
            m_clientInventory.setItem(selectedSlot, updatedStack);
            sendPlayerInventory();
        }
    }
}

void IntegratedServer::handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleHotbarSelect");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = HotbarSelectPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse hotbar select packet: {}", result.error().message());
        return;
    }

    m_clientInventory.setSelectedSlot(result.value().slot());
}

void IntegratedServer::handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleContainerClick");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    if (!m_openMenu) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = ContainerClickPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse container click packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    if (packet.containerId() != m_openMenu->getId()) {
        return;
    }

    ClickType clickType = (packet.button() == 0) ? ClickType::Pick : ClickType::PickSome;
    if (packet.action() == ClickAction::QuickMove) {
        clickType = ClickType::QuickMove;
    }

    Player& menuPlayer = getMenuPlayer();
    m_openMenu->clicked(packet.slotIndex(), packet.button(), clickType, menuPlayer);

    sendContainerContent(*m_openMenu);
    sendPlayerInventory();
}

void IntegratedServer::handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleCloseContainer");
    auto* player = getPlayerData();
    if (!player || !player->loggedIn) {
        return;
    }

    auto openMenu = std::move(m_openMenu);
    m_openContainerType = mc::ContainerType::Player;

    if (!openMenu) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = CloseContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse close container packet: {}", result.error().message());
        return;
    }

    if (result.value().containerId() != openMenu->getId()) {
        return;
    }

    Player& menuPlayer = getMenuPlayer();
    openMenu->removed(menuPlayer);
    sendPlayerInventory();
}

// ============================================================================
// 数据包发送
// ============================================================================

void IntegratedServer::sendLoginResponse(bool success, PlayerId playerId,
                                          const String& username, const String& message)
{
    bool isDebugWorld = m_world && m_world->isDebugWorld();
    network::LoginResponsePacket response(success, playerId, username, message, isDebugWorld);
    network::PacketSerializer ser;
    response.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::LoginResponse, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::Teleport, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId)
{
    network::BlockUpdatePacket packet(x, y, z, blockStateId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::BlockUpdate, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendPlayerInventory()
{
    PlayerInventoryPacket packet(m_clientInventory);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::PlayerInventory, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendChunkData(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data)
{
    MC_TRACE_EVENT("server.chunk", "sendChunkData",
               "Chunk", fmt::format("({}, {})", x, z),
               "DataSize", data.size());
    DimensionId dimension = 0;
    if (m_dimensionManager) {
        const DimensionId resolvedDimension = m_dimensionManager->getPlayerDimension(m_clientPlayerId);
        if (resolvedDimension >= 0) {
            dimension = resolvedDimension;
        }
    }
    network::ChunkDataPacket packet(x, z, dimension, data);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::ChunkData, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendUnloadChunk(ChunkCoord x, ChunkCoord z)
{
    DimensionId dimension = 0;
    if (m_dimensionManager) {
        const DimensionId resolvedDimension = m_dimensionManager->getPlayerDimension(m_clientPlayerId);
        if (resolvedDimension >= 0) {
            dimension = resolvedDimension;
        }
    }
    network::UnloadChunkPacket packet(x, z, dimension);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::UnloadChunk, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendContainerContent(const AbstractContainerMenu& menu)
{
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::ContainerContent,
                   ContainerPacketHandler::createContentPacket(menu));
}

void IntegratedServer::sendOpenContainer(ContainerId containerId, mc::ContainerType type, const String& title, i32 slotCount)
{
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::OpenContainer,
                   ContainerPacketHandler::createOpenContainerPacket(containerId,
                                                                     ContainerTypes::toNetworkType(type),
                                                                     title,
                                                                     slotCount));
}

void IntegratedServer::sendCloseContainer(ContainerId containerId)
{
    sendGamePacket(m_serverEndpoint,
                   network::PacketType::CloseContainer,
                   CloseContainerPacket(containerId));
}

void IntegratedServer::sendToClient(const u8* data, size_t size)
{
    if (m_serverEndpoint && m_serverEndpoint->isConnected()) {
        m_serverEndpoint->send(data, size);
    }
}

void IntegratedServer::sendBlockBreakAnim(EntityId breakerId, i32 x, i32 y, i32 z, i8 stage)
{
    network::BlockBreakAnimPacket packet;
    packet.setBreakerEntityId(breakerId);
    packet.setPosition(BlockPos(x, y, z));
    packet.setStage(stage);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize BlockBreakAnim packet: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::BlockBreakAnim, result.value());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::openCraftingTableMenu()
{
    auto* player = getPlayerData();
    if (!player) return;

    if (m_openMenu) {
        Player& menuPlayer = getMenuPlayer();
        m_openMenu->removed(menuPlayer);
        sendCloseContainer(m_openMenu->getId());
    }

    ContainerId containerId = m_nextContainerId++;

    auto menu = std::make_unique<CraftingMenu>(containerId, &m_clientInventory, nullptr);
    menu->updateResult();

    sendOpenContainer(containerId,
                      mc::ContainerType::CraftingTable,
                      String(ContainerTypes::getDefaultTitle(mc::ContainerType::CraftingTable)),
                      menu->getSlotCount());
    sendContainerContent(*menu);

    m_openContainerType = mc::ContainerType::CraftingTable;
    m_openMenu = std::move(menu);
}

void IntegratedServer::setupChunkSendCallback()
{
    // 设置区块发送回调 - 当区块数据准备好时发送给客户端
    chunkSendManager().setOnChunkSend([this](PlayerId playerId, ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
        // 单玩家模式，忽略 playerId
        (void)playerId;
        sendChunkData(x, z, data);
        MC_TRACE_INSTANT("server.chunk", "ChunkSent",
                   "Chunk", fmt::format("({}, {})", x, z),
                   "DataSize", data.size());

    });

    // 设置区块卸载回调
    chunkSendManager().setOnChunkUnload([this](PlayerId playerId, ChunkCoord x, ChunkCoord z) {
        // 单玩家模式，忽略 playerId
        (void)playerId;
        sendUnloadChunk(x, z);
        MC_TRACE_INSTANT("server.chunk", "ChunkUnloaded",
                   "Chunk", fmt::format("({}, {})", x, z));
    });
}

void IntegratedServer::broadcastLightUpdate(ChunkCoord x, ChunkCoord z, i32 sectionY,
                                             const std::vector<u8>& skyLight,
                                             const std::vector<u8>& blockLight,
                                             bool trustEdges)
{
    SectionPos sectionPos(x, sectionY, z);
    MC_TRACE_INSTANT("server.lighting", "BroadcastLightUpdate",
               "Section", fmt::format("({}, {}, {})", x, sectionY, z),
               "SkyLightSize", skyLight.size(),
               "BlockLightSize", blockLight.size(),
               [flow = ::perfetto::Flow::ProcessScoped(sectionPos.toLong())](::perfetto::EventContext ctx) {
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
    sendToClient(fullPacket.data(), fullPacket.size());
}

} // namespace mc::server
