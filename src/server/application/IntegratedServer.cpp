#include "IntegratedServer.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
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
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
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
    if (m_initialized) {
        stop();
    }
}

Result<void> IntegratedServer::initialize()
{
    return initialize(IntegratedServerConfig{});
}

Result<void> IntegratedServer::initialize(const IntegratedServerConfig& config)
{
    MC_TRACE_EVENT("server.initialization", "IntegratedServer::initialize");

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
    m_world->setOnPlaySound([this](const ResourceLocation& soundEventId,
                                   sound::SoundCategory category,
                                   const Vector3& position,
                                   f32 volume,
                                   f32 pitch) {
        broadcastSound(soundEventId, category, position, volume, pitch);
    });

    // 设置 TimeManager 引用
    m_world->setTimeManager(m_timeManager.get());

    auto worldInitResult = m_world->initialize();
    if (worldInitResult.failed()) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to initialize world: " + worldInitResult.error().message());
    }

    // 根据世界类型创建区块管理器
    {
        MC_TRACE_EVENT("server.initialization", "IntegratedServer::initializeChunkManager");

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
    }

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

void IntegratedServer::requestStop()
{
    MinecraftServer::requestStop();

    if (m_connectionPair) {
        m_connectionPair->disconnect();
    }
}

void IntegratedServer::stop()
{
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
    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
    }
    m_serverThread.reset();

    // 清理玩家实体
    if (m_world) {
        m_playerEntityManager.clearAll(*m_world);
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
        sendLoginResponse(false, 0, INVALID_ENTITY_ID, "", "Invalid login request");
        return;
    }

    auto& packet = result.value();
    String username = packet.username();

    spdlog::info("Player '{}' attempting to join", username);

    // 创建本地连接
    m_clientConnection = std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());

    // 分配玩家ID
    m_clientPlayerId = m_playerManager->nextPlayerId();

    // 添加玩家会话信息
    auto* playerData = m_playerManager->addPlayer(m_clientPlayerId, username, m_clientConnection);
    if (!playerData) {
        sendLoginResponse(false, 0, INVALID_ENTITY_ID, username, "Failed to add player");
        return;
    }

    // 设置玩家初始状态
    setupInitialPlayerState(playerData, m_config.defaultGameMode);

    // 创建玩家实体并加入世界（关键：玩家实体纳入 EntityManager 和 EntityTracker）
    MC_ASSERT(m_world != nullptr);
    Player* playerEntity = m_playerEntityManager.createPlayerEntity(
        m_clientPlayerId, username, *m_world,
        static_cast<f32>(playerData->x), static_cast<f32>(playerData->y), static_cast<f32>(playerData->z)
    );

    if (!playerEntity) {
        spdlog::error("Failed to create player entity for {}", username);
        m_playerManager->removePlayer(m_clientPlayerId);
        sendLoginResponse(false, 0, INVALID_ENTITY_ID, username, "Failed to create player entity");
        return;
    }

    // 记录实体ID
    m_clientEntityId = playerEntity->id();

    // 初始化物品栏
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

    // 发送登录成功响应（包含 playerId 和 entityId）
    sendLoginResponse(true, m_clientPlayerId, m_clientEntityId, username, "Welcome to singleplayer world!");

    // 同步命令树
    sendCommandTreePacket(m_clientPlayerId);

    // 发送初始游戏状态
    sendInitialGameState(m_clientPlayerId, playerData->x, playerData->y, playerData->z, playerData->yaw, playerData->pitch);
    sendPlayerInventory();

    spdlog::info("Player '{}' (PlayerId={}, EntityId={}) joined the game",
                 username, m_clientPlayerId, m_clientEntityId);
}

void IntegratedServer::handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;  // 单玩家模式，playerId 总是 m_clientPlayerId
    MC_TRACE_EVENT("server.network", "HandleBlockPlacement");

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerTryUseItemOnBlockPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse block placement packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    const BlockPos clickedPos(packet.x(), packet.y(), packet.z());
    const BlockState* clickedState = m_world ? m_world->getBlockState(clickedPos) : nullptr;
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
        spdlog::error("Failed to parse hotbar select packet: {}", result.error().message());
        return;
    }

    m_clientInventory.setSelectedSlot(result.value().slot());
}

void IntegratedServer::handleCreativeInventoryActionPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleCreativeInventoryAction");

    auto* player = getPlayerData();
    if (!player || !player->loggedIn || player->gameMode != GameMode::Creative) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = CreativeInventoryActionPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse creative inventory action packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    const i32 slotIndex = packet.slotIndex();
    if (slotIndex < 0 || slotIndex >= PlayerInventory::TOTAL_SIZE) {
        spdlog::warn("Ignoring creative inventory action with invalid slot {}", slotIndex);
        return;
    }

    m_clientInventory.setItem(slotIndex, packet.item());
    sendPlayerInventory();
}

void IntegratedServer::handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size)
{
    (void)playerId;
    MC_TRACE_EVENT("server.network", "HandleContainerClick");
    auto* player = getPlayerData();
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
    Player& menuPlayer = getMenuPlayer();
    if (!ContainerPacketHandler::handleContainerClick(menuPlayer, packet)) {
        return;
    }

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

    network::PacketDeserializer deser(data, size);
    auto result = CloseContainerPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse close container packet: {}", result.error().message());
        return;
    }

    if (!m_openMenu || result.value().containerId() != m_openMenu->getId()) {
        return;
    }

    closeCurrentContainer(false);
    sendPlayerInventory();
}

// ============================================================================
// 数据包发送
// ============================================================================

void IntegratedServer::sendLoginResponse(bool success, PlayerId playerId, EntityId entityId,
                                          const String& username, const String& message)
{
    bool isDebugWorld = m_world && m_world->isDebugWorld();
    network::LoginResponsePacket response(success, playerId, entityId, username, message, isDebugWorld);
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
               "DataSize", data.size(),
                [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(x, z).toId())](::perfetto::EventContext ctx) {
                flow(ctx);
    });

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

bool IntegratedServer::openContainerRequest(ContainerType type, const BlockPos& pos, Player& player)
{
    (void)player;
    return openContainerMenu(type, pos);
}

bool IntegratedServer::openContainerMenu(ContainerType type, const BlockPos& pos)
{
    auto* player = getPlayerData();
    if (!player) {
        return false;
    }

    closeCurrentContainer(true);

    ContainerId containerId = m_nextContainerId++;
    std::unique_ptr<AbstractContainerMenu> menu;

    switch (type) {
        case ContainerType::CraftingTable: {
            auto craftingMenu = std::make_unique<CraftingMenu>(containerId, &m_clientInventory, nullptr);
            craftingMenu->updateResult();
            menu = std::move(craftingMenu);
            break;
        }
        case ContainerType::Chest: {
            if (m_world == nullptr) {
                return false;
            }

            BlockEntity* blockEntity = m_world->getBlockEntity(pos);
            if (blockEntity == nullptr) {
                return false;
            }

            if (blockEntity->getType() != BlockEntityType::Chest &&
                blockEntity->getType() != BlockEntityType::TrappedChest) {
                return false;
            }

            auto* chest = static_cast<blockentity::ChestEntity*>(blockEntity);
            if (chest->isDoubleChest(*m_world)) {
                auto doubleInventory = chest->getDoubleInventory(*m_world);
                if (!doubleInventory) {
                    return false;
                }

                m_openInventoryOwner = std::shared_ptr<IInventory>(std::move(doubleInventory));
                menu = blockentity::ChestContainer::createDouble(containerId, &m_clientInventory, m_openInventoryOwner.get());
            } else {
                m_openInventoryOwner.reset();
                menu = blockentity::ChestContainer::createSingle(containerId, &m_clientInventory, chest->getInventory());
            }
            break;
        }
        case ContainerType::Furnace: {
            if (m_world == nullptr) {
                return false;
            }

            BlockEntity* blockEntity = m_world->getBlockEntity(pos);
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
            menu = std::make_unique<blockentity::FurnaceContainer>(containerId, &m_clientInventory, furnace->getInventory(), furnace);
            break;
        }
        case ContainerType::Player:
        default:
            return false;
    }

    if (!menu) {
        return false;
    }

    sendOpenContainer(containerId,
                      type,
                      String(ContainerTypes::getDefaultTitle(type)),
                      menu->getSlotCount());
    sendContainerContent(*menu);

    m_openContainerType = type;
    m_openContainerPos = pos;
    m_openMenu = std::move(menu);
    getMenuPlayer().setOpenContainerMenu(m_openMenu.get());
    return true;
}

void IntegratedServer::closeCurrentContainer(bool sendClosePacket)
{
    if (!m_openMenu) {
        return;
    }

    if (m_world && m_openContainerType == ContainerType::Chest) {
        BlockEntity* blockEntity = m_world->getBlockEntity(m_openContainerPos);
        if (blockEntity != nullptr &&
            (blockEntity->getType() == BlockEntityType::Chest ||
             blockEntity->getType() == BlockEntityType::TrappedChest)) {
            static_cast<blockentity::ChestEntity*>(blockEntity)->closeContainer();
        }
    }

    Player& menuPlayer = getMenuPlayer();
    m_openMenu->removed(menuPlayer);
    if (sendClosePacket) {
        sendCloseContainer(m_openMenu->getId());
    }
    menuPlayer.clearOpenContainerMenu();
    m_openMenu.reset();
    m_openInventoryOwner.reset();
    m_openContainerType = ContainerType::Player;
    m_openContainerPos = BlockPos();
}

void IntegratedServer::openCraftingTableMenu()
{
    (void)openContainerMenu(ContainerType::CraftingTable, BlockPos());
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
                   "DataSize", data.size(),
                    [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(x, z).toId())](::perfetto::EventContext ctx) {
                          flow(ctx);
                    });  // 这里需要闭合 MC_TRACE_INSTANT 的括号
    });

    // 设置区块卸载回调
    chunkSendManager().setOnChunkUnload([this](PlayerId playerId, ChunkCoord x, ChunkCoord z) {
        // 单玩家模式，忽略 playerId
        (void)playerId;
        sendUnloadChunk(x, z);
        MC_TRACE_INSTANT("server.chunk", "ChunkUnloaded",
                   "Chunk", fmt::format("({}, {})", x, z));
    });  // 闭合 setOnChunkUnload 的 lambda 和函数调用
}

void IntegratedServer::broadcastLightUpdate(ChunkCoord x, ChunkCoord z, i32 sectionY,
                                             const std::vector<u8>& skyLight,
                                             const std::vector<u8>& blockLight,
                                             bool trustEdges)
{
    MC_TRACE_EVENT("server.lighting", "IntegratedServer::BroadcastLightUpdate",
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
    sendToClient(fullPacket.data(), fullPacket.size());
}

} // namespace mc::server
