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
#include "common/entity/inventory/container/ChestContainer.hpp"
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
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/TimeManager.hpp"
#include "server/menu/CraftingMenu.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/util/assert/AssertAll.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

namespace {

/**
 * @brief 获取菜单玩家（临时方案）
 */
Player& getMenuPlayer()
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
    return initialize(IntegratedServerParams{});
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
        spdlog::debug("No ops.json found or failed to load: {}", opsResult.error().message());
    }

    auto storageInitResult = initializeSharedStorage(params.worldName);
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
    m_world = overworld->world();
    MC_ASSERT_RELEASE(m_world != nullptr);

    attachWorldBindings(*m_world);
    attachWorldCommandBindings(*m_world);

    // 设置命令执行回调（用于命令方块矿车等实体执行命令）
    // 参考 MC 1.16.5: CommandBlockLogic.trigger() -> Commands.handleCommand()
    m_world->setOnExecuteCommand(
        [this](const std::string& command, const Vector3d& position, i32 permissionLevel) -> i32 {
            std::string cmd = command;
            if (!cmd.empty() && cmd[0] != '/') {
                cmd = "/" + cmd;
            }

            command::ServerCommandSource source(
                this, nullptr, m_world, position, Vector2f(0.0f, 0.0f), permissionLevel, 0, "@");
            auto result = m_commandRegistry->execute(cmd, source);
            if (result.failed()) {
                spdlog::debug("Command execution failed for '{}': {}", cmd, result.error().message());
                return 0;
            }

            return result.value();
        });

    auto worldResult = initializeWorld();
    if (worldResult.failed()) {
        return worldResult;
    }

    if (m_world->isDebugWorld()) {
        spdlog::info("Configuring debug world special settings...");
        m_settings.defaultGameMode.set(static_cast<i32>(GameMode::Spectator));
        if (m_timeManager) {
            m_timeManager->setDayTime(6000);
            m_timeManager->setDaylightCycleEnabled(false);
            spdlog::info("Debug world: Time set to noon (6000), daylight cycle disabled");
        }
        if (m_world->weatherManager()) {
            m_world->weatherManager()->setClear(999999999);
            spdlog::info("Debug world: Weather set to clear");
        }
    }

    setupRaidManagerCallbacks();
    initializeInteractionManagers();

    // 初始化同步管理器
    initializeSyncManagers();

    // 初始化区块同步管理器（需要 world 已初始化）
    initializeChunkSyncManagers();

    // 设置世界回调（包括光照变化回调）
    setupWorldCallbacks();

    // 启动服务端线程
    m_running = true;
    m_serverThread = std::make_unique<std::thread>([this]() { mainLoop(); });

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
    const auto tickDuration = std::chrono::milliseconds(1000 / m_settings.tickRate.get());

    mc::perfetto::PerfettoManager::instance().setThreadName("IntegratedServerThread");

    spdlog::info("Integrated server started ({} TPS)", m_settings.tickRate.get());

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
    std::string username = packet.username();

    spdlog::info("Player '{}' attempting to join", username);

    // 白名单检查（MC 1.16.5 行为：白名单启用时拒绝不在名单中的玩家）
    if (m_whitelistManager->isEnabled() && !m_whitelistManager->isNameWhitelisted(username)) {
        spdlog::info("Player '{}' rejected: not in whitelist", username);
        sendLoginResponse(false, 0, INVALID_ENTITY_ID, username, "You are not whitelisted on this server!");
        return;
    }

    // 创建本地连接
    m_clientConnection = std::make_shared<network::LocalServerConnection>(&m_connectionPair->serverEndpoint());

    // 分配玩家ID
    m_clientPlayerId = m_playerManager->nextPlayerId();

    // 生成离线模式 UUID（基于用户名）
    // 参考 MC 1.16.5: UUID.nameUUIDFromBytes(("OfflinePlayer:" + username).getBytes(UTF_8))
    Uuid offlineUuid = util::generateOfflineUuid(username);
    std::string uuidStr = util::uuidToString(offlineUuid);

    // 添加玩家会话信息
    auto* playerData = m_playerManager->addPlayer(m_clientPlayerId, uuidStr, username, m_clientConnection);
    if (!playerData) {
        sendLoginResponse(false, 0, INVALID_ENTITY_ID, username, "Failed to add player");
        return;
    }

    // 设置玩家初始状态
    setupInitialPlayerState(playerData, static_cast<GameMode>(m_settings.defaultGameMode.get()));

    // 创建玩家实体并加入世界（关键：玩家实体纳入 EntityManager 和 EntityTracker）
    MC_ASSERT(m_world != nullptr);
    Player* playerEntity = m_playerEntityManager.createPlayerEntity(m_clientPlayerId,
        username,
        *m_world,
        static_cast<f32>(playerData->x),
        static_cast<f32>(playerData->y),
        static_cast<f32>(playerData->z));

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
    sendInitialGameState(
        m_clientPlayerId, playerData->x, playerData->y, playerData->z, playerData->yaw, playerData->pitch);
    sendPlayerInventory();

    spdlog::info(
        "Player '{}' (PlayerId={}, EntityId={}) joined the game", username, m_clientPlayerId, m_clientEntityId);
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

void IntegratedServer::sendLoginResponse(
    bool success, PlayerId playerId, EntityId entityId, const std::string& username, const std::string& message)
{
    bool isDebugWorld = m_world && m_world->isDebugWorld();
    network::LoginResponsePacket response(success, playerId, entityId, username, message, isDebugWorld);
    network::PacketSerializer ser;
    response.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::LoginResponse, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Teleport, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendPlayerInventory()
{
    PlayerInventoryPacket packet(m_clientInventory);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlayerInventory, ser.buffer());
    sendToClient(fullPacket.data(), fullPacket.size());
}

void IntegratedServer::sendContainerContent(const AbstractContainerMenu& menu)
{
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

void IntegratedServer::sendOpenContainer(
    ContainerId containerId, mc::ContainerType type, const std::string& title, i32 slotCount)
{
    (void)slotCount; // slotCount is no longer sent in the packet (MC 1.16.5 protocol)
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

void IntegratedServer::sendCloseContainer(ContainerId containerId)
{
    if (m_serverEndpoint == nullptr || !m_serverEndpoint->isConnected()) {
        return;
    }

    network::PacketSerializer payload;
    CloseContainerPacket packet(containerId);
    packet.serialize(payload);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::CloseContainer, payload.buffer());
    m_serverEndpoint->send(fullPacket.data(), fullPacket.size());
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

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::BlockBreakAnim, result.value());
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
        case ContainerType::Crafting: {
            auto craftingMenu = std::make_unique<CraftingMenu>(containerId, &m_clientInventory, nullptr);
            craftingMenu->updateResult();
            menu = std::move(craftingMenu);
            break;
        }
        case ContainerType::Generic9x3:
        case ContainerType::Generic9x6: {
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
            menu = std::make_unique<blockentity::FurnaceContainer>(
                containerId, &m_clientInventory, furnace->getInventory(), furnace);
            break;
        }
        case ContainerType::Player:
        default:
            return false;
    }

    if (!menu) {
        return false;
    }

    sendOpenContainer(containerId, type, std::string(ContainerTypes::getDefaultTitle(type)), menu->getSlotCount());
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

    if (m_world &&
        (m_openContainerType == ContainerType::Generic9x3 || m_openContainerType == ContainerType::Generic9x6 ||
            m_openContainerType == ContainerType::ShulkerBox)) {
        BlockEntity* blockEntity = m_world->getBlockEntity(m_openContainerPos);
        if (blockEntity != nullptr &&
            (blockEntity->getType() == BlockEntityType::Chest ||
                blockEntity->getType() == BlockEntityType::TrappedChest)) {
            static_cast<blockentity::ChestEntity*>(blockEntity)->closeContainer(nullptr);
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
    (void)openContainerMenu(ContainerType::Crafting, BlockPos());
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
    sendPlayerInventory();
}

bool IntegratedServer::tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos)
{
    MC_UNUSED(playerId);
    MC_UNUSED(pos);
    openCraftingTableMenu();
    return true;
}

} // namespace mc::server
