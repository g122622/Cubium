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

#include "MinecraftServer.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/loot/LootTableLoader.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/packet/CommandTreePacket.hpp"
#include "common/network/packet/ContainerPacketHandler.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/ServerDifficultyPacket.hpp"
#include "common/network/packet/SpawnPositionPacket.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/ChunkLoadTicket.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/storage/db/ConsistencyMode.hpp"
#include "common/world/storage/save/AutoSave.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/world/spawn/DespawnManager.hpp"
#include "server/world/spawn/NaturalSpawner.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <filesystem>
#include <spdlog/spdlog.h>

namespace mc::server {

namespace {

[[nodiscard]] bool isCraftingTableState(const BlockState* state)
{
    return state != nullptr && state->blockLocation() == ResourceLocation("minecraft:crafting_table");
}

} // namespace

MinecraftServer::MinecraftServer(ServerSettings& settings)
    : m_settings(settings)
    , m_computationWorkerPool(-1, "ServerCompute")
    , m_ioWorkerPool(-1, "ServerIO")
    , m_lootTableManager()
{}

void MinecraftServer::setDifficulty(Difficulty difficulty)
{
    if (m_difficulty == difficulty) {
        return; // 难度未变化，无需同步
    }
    m_difficulty = difficulty;

    // 广播难度变更给所有玩家
    broadcastDifficultyChange();
}

void MinecraftServer::broadcastDifficultyChange()
{
    auto fullPacket = serializeDifficultyPacket();
    if (fullPacket.empty()) {
        return;
    }

    broadcastPacket(fullPacket.data(), fullPacket.size());
    spdlog::info("Difficulty changed to {}", static_cast<i32>(m_difficulty));
}

std::vector<u8> MinecraftServer::serializeDifficultyPacket()
{
    network::ServerDifficultyPacket packet(m_difficulty, false);
    auto serializeResult = packet.serialize();
    if (serializeResult.failed()) {
        spdlog::error("Failed to serialize ServerDifficultyPacket");
        return {};
    }

    return core::ConnectionManager::encapsulatePacket(network::PacketType::ServerDifficulty, serializeResult.value());
}

void MinecraftServer::setDefaultGameMode(GameMode mode)
{
    m_settings.defaultGameMode.set(static_cast<i32>(mode));
}

void MinecraftServer::setPlayerIdleTimeoutMinutes(i32 timeoutMinutes)
{
    m_playerIdleTimeoutMinutes = timeoutMinutes;
}

void MinecraftServer::broadcastServerMessage(std::string_view message)
{
    const std::string text(message);
    spdlog::info("[System] {}", text);
}

void MinecraftServer::requestStop()
{
    m_running = false;
}

PlayerInventory* MinecraftServer::playerInventory(PlayerId playerId)
{
    MC_ASSERT_RELEASE(m_inventoryManager != nullptr);
    return m_inventoryManager->getInventory(playerId);
}

const PlayerInventory* MinecraftServer::playerInventory(PlayerId playerId) const
{
    MC_ASSERT_RELEASE(m_inventoryManager != nullptr);
    return m_inventoryManager->getInventory(playerId);
}

MinecraftServer::~MinecraftServer()
{
    if (m_running) {
        shutdown();
    }
}

void MinecraftServer::shutdown()
{
    m_running = false;
    shutdownManagers();
}

void MinecraftServer::tick()
{
    MC_TRACE_EVENT("server.tick", "MinecraftServerTick");

    if (!m_running.load()) {
        return;
    }

    // 更新时间
    {
        MC_TRACE_EVENT("server.tick", "TickTime");
        m_timeManager->tick();
    }

    // 自然刷怪（在世界 tick 后、实体 tick 前执行）
    if (m_naturalSpawner && m_world) {
        MC_TRACE_EVENT("server.tick", "NaturalSpawn");
        m_naturalSpawner->tick(*m_world, true, true);
    }

    // 生物消失检查（在刷怪后执行）
    if (m_despawnManager && m_world) {
        MC_TRACE_EVENT("server.tick", "DespawnCheck");
        m_despawnManager->tick(*m_world);
    }

    // 清理断开连接的玩家
    if (m_tickCounter % CLEANUP_INTERVAL == 0) {
        MC_TRACE_EVENT("server.player", "CleanupDisconnected", "phase", "cleanup");

        std::vector<PlayerId> removedPlayers;
        m_connectionManager->cleanupDisconnectedPlayers(&removedPlayers);
        // 清理玩家相关的追踪和票据
        for (PlayerId playerId : removedPlayers) {
            m_chunkSendManager->removePlayer(playerId);
            if (m_world && m_world->chunkManager()) {
                m_world->chunkManager()->removePlayer(playerId);
            }
        }
    }

    ++m_tickCounter;

    // 更新所有维度
    if (m_dimensionManager) {
        MC_TRACE_EVENT("server.tick", "TickAllDimensions");
        m_dimensionManager->tick();
    }

    // 执行实体 tick
    tickEntities();

    // 同步实体位置
    entitySyncManager().tick();

    // 更新挖掘进度
    miningManager().tick(*m_world);

    // 处理网络事件（子类实现）
    pollNetwork();

    if (!m_running.load()) {
        return;
    }

    // 检查心跳超时
    if (m_tickCounter % KEEPALIVE_INTERVAL == 0) {
        MC_TRACE_EVENT("server.network", "CheckKeepAliveTimeout", "phase", "keepalive_timeout");

        u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
        auto timedOutPlayers = m_keepAliveManager->getTimedOutPlayers(currentTimeMs);
        for (PlayerId playerId : timedOutPlayers) {
            spdlog::warn("MinecraftServer: Player {} timed out", playerId);
            // 清理玩家相关的追踪和票据
            m_chunkSendManager->removePlayer(playerId);
            if (m_world && m_world->chunkManager()) {
                m_world->chunkManager()->removePlayer(playerId);
            }
            m_connectionManager->disconnectPlayer(playerId, "Connection timed out");
        }
    }

    // 处理区块发送队列
    chunkSendManager().processPendingSends();

    // 统一发送待处理的方块更新
    if (m_blockUpdateSyncManager) {
        m_blockUpdateSyncManager->flushPendingUpdates();
    }

    // 心跳（每 15 秒）
    tickKeepAlive();

    // 每 20 tick 同步一次时间
    u64 tick = currentTick();
    if (tick % 20 == 0) {
        sendTimeUpdate();
    }

    // 同步天气变化
    sendWeatherUpdate();
}

void MinecraftServer::initializeCoreManagers()
{
    MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeCoreManagers");

    // 创建核心管理器
    m_playerManager = std::make_unique<core::PlayerManager>(m_settings.maxPlayers.get());
    m_connectionManager = std::make_unique<core::ConnectionManager>(*m_playerManager);
    // 与 Java 版一致：世界初始白天时间从 1000 开始（清晨后）
    m_timeManager = std::make_unique<core::TimeManager>(0, 1000);
    m_teleportManager = std::make_unique<core::TeleportManager>(*m_playerManager);
    m_keepAliveManager = std::make_unique<core::KeepAliveManager>(
        *m_playerManager, m_settings.keepAliveInterval.get(), m_settings.keepAliveTimeout.get());
    m_positionTracker = std::make_unique<core::PositionTracker>(*m_playerManager, m_settings.viewDistance.get());
    m_packetHandler = std::make_unique<core::PacketHandler>(*m_playerManager,
        *m_connectionManager,
        *m_teleportManager,
        *m_keepAliveManager,
        *m_positionTracker,
        *m_timeManager,
        static_cast<GameMode>(m_settings.defaultGameMode.get()));
    m_gameModeManager = std::make_unique<core::GameModeManager>(*m_playerManager, *m_connectionManager);
    m_whitelistManager = std::make_unique<core::WhitelistManager>();
    m_bannedPlayerList = std::make_unique<core::BannedPlayerList>();
    m_bannedIpList = std::make_unique<core::BannedIpList>();
    m_opListManager = std::make_unique<core::OpListManager>();

    // 创建记分板
    m_scoreboard = std::make_unique<ServerScoreboard>(*this);

    // 创建 Boss 栏管理器
    m_bossBarManager = std::make_unique<CustomServerBossInfoManager>(*this);

    // 创建维度管理器
    m_dimensionManager = std::make_unique<ServerDimensionManager>(this);
    m_dimensionManager->setDimensionChangeCallback(
        [this](PlayerId playerId, DimensionId, DimensionId, const Vector3d& position) {
            auto* player = m_playerManager->getPlayer(playerId);
            if (!player) {
                return;
            }

            m_positionTracker->updatePosition(
                playerId, position.x, position.y, position.z, player->yaw, player->pitch, player->onGround);
            updateEntityTrackingForPlayer(playerId, position.x, position.y, position.z);
        });

    // Worker 池由服务器统一管理，初始化阶段在这里启动。
    m_computationWorkerPool.start();
    m_ioWorkerPool.start();
}

void MinecraftServer::attachWorldBindings(ServerWorld& world)
{
    world.setOnPlaySound([this](const ResourceLocation& soundEventId,
                             sound::SoundCategory category,
                             const Vector3& position,
                             f32 volume,
                             f32 pitch) { broadcastSound(soundEventId, category, position, volume, pitch); });
    world.setOnBroadcastParticle([this](client::renderer::trident::particle::ParticleTypeId type,
                                     const Vector3& pos,
                                     const Vector3& velocity,
                                     const Vector3& offset,
                                     u32 count) { broadcastParticleInRange(type, pos, velocity, offset, count); });
    world.setOnBroadcastEntityStatus([this, &world](EntityId entityId, u8 status) {
        Entity* entity = world.getEntity(entityId);
        if (entity != nullptr) {
            broadcastEntityStatusInRange(entityId, status, entity->position());
        }
    });
    world.setOnBroadcastWorldEvent(
        [this](i32 eventId, i32 x, i32 y, i32 z, i32 data) { broadcastWorldEventInRange(eventId, x, y, z, data); });
    world.setOnBroadcastExplosion([this](const Vector3& position,
                                      f32 strength,
                                      const std::vector<BlockPos>& affectedBlocks,
                                      const std::unordered_map<u64, Vector3>& playerKnockback) {
        broadcastExplosionInRange(position, strength, affectedBlocks, playerKnockback);
    });
}

void MinecraftServer::attachWorldCommandBindings(ServerWorld& world)
{
    world.setTimeManager(m_timeManager.get());
    world.setDifficultyCallback([this]() { return this->difficulty(); });
    world.setLootTableManager(&m_lootTableManager);
}

Result<void> MinecraftServer::initializeSharedStorage(const GameDirectory& gameDirectory, const std::string& levelId)
{
    world::storage::SingleLevelStorageConfig storageConfig;
    storageConfig.consistencyMode = world::storage::ConsistencyMode::Eventual;
    storageConfig.sectionCacheCapacity = 2048;
    storageConfig.enableBackup = true;

    m_globalStorage = world::storage::GlobalStorageManager(gameDirectory);
    auto storageResult = m_globalStorage.openLevel(levelId, storageConfig);
    if (storageResult.failed()) {
        spdlog::error("Failed to open world storage: {}", storageResult.error().message());
        return storageResult.error();
    }
    m_storage = storageResult.value();
    m_storage->setIoWorkerPool(&m_ioWorkerPool);
    spdlog::info("World storage opened at {}", m_storage->worldPath().string());

    world::storage::AutoSaveConfig saveConfig;
    m_storage->initializeAutoSave(saveConfig);
    m_storage->startAutoSave();
    m_scoreboard->setDataManager(m_storage->scoreboardDataManager());
    m_scoreboard->load();
    return Result<void>::ok();
}

void MinecraftServer::shutdownSharedStorage()
{
    m_storage.reset();
}

Result<size_t> MinecraftServer::saveAllWorldData()
{
    if (!m_storage || !m_storage->isOpen()) {
        return Error(ErrorCode::InvalidState, "Shared storage not open");
    }

    auto result = m_storage->saveAll();
    if (result.failed()) {
        return result.error();
    }

    spdlog::info("Saved {} cached sections and player data during shutdown", result.value());
    return result.value();
}

Result<void> MinecraftServer::initializeWorld()
{
    MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeWorld");

    // ServerWorld 在子类中创建
    if (!m_world) {
        return Error(ErrorCode::NotInitialized, "World not created");
    }

    // 初始化物理引擎
    // TODO: 创建碰撞世界适配器

    // 初始化命令注册表
    m_commandRegistry = std::make_unique<command::CommandRegistry>();

    // 初始化刷怪系统
    m_naturalSpawner = std::make_unique<::mc::world::spawn::NaturalSpawner>();
    m_despawnManager = std::make_unique<::mc::world::spawn::DespawnManager>();

    return Result<void>::ok();
}

void MinecraftServer::initializeInteractionManagers()
{
    MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeInteractionManagers");

    m_blockInteractionManager =
        std::make_unique<interaction::BlockInteractionManager>(*m_world, *m_playerManager, m_lootTableManager);

    m_miningManager = std::make_unique<interaction::MiningManager>(*m_playerManager, *m_connectionManager);

    m_containerManager = std::make_unique<interaction::ContainerManager>(*m_playerManager);

    m_inventoryManager = std::make_unique<interaction::InventoryManager>(*m_playerManager);

    m_inventoryManager->setOnInventoryUpdate([this](PlayerId playerId, const PlayerInventory& inventory) {
        PlayerInventoryPacket packet(inventory);
        network::PacketSerializer payload;
        packet.serialize(payload);

        const auto fullPacket =
            core::ConnectionManager::encapsulatePacket(network::PacketType::PlayerInventory, payload.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    });

    // 设置 InventoryManager 到其他管理器
    m_containerManager->setInventoryManager(m_inventoryManager.get());
    m_blockInteractionManager->setInventoryManager(m_inventoryManager.get());
    m_miningManager->setInventoryManager(m_inventoryManager.get());
    m_miningManager->setOnMiningComplete([this](PlayerId playerId, const BlockPos& pos) {
        MC_ASSERT_RELEASE(m_blockInteractionManager != nullptr);
        auto result = m_blockInteractionManager->handleBlockBreak(playerId, pos);
        if (result.failed()) {
            spdlog::debug("Mining completion block break failed for player {} at {}: {}",
                playerId,
                pos.toString(),
                result.error().message());
            return;
        }

        if (!result.value().blockBroken) {
            spdlog::debug("Mining completion did not break block for player {} at {}: {}",
                playerId,
                pos.toString(),
                result.value().message);
        }
    });

    // 设置服务器接口到 BlockInteractionManager（用于告示牌命令执行等）
    m_blockInteractionManager->setServer(this);

    // 初始化成就事件处理器
    // 设置服务器接口以允许从 PlayerId 获取 ServerPlayer
    m_advancementEventHandler.setServer(this);
    m_advancementEventHandler.initialize();
    spdlog::info("AdvancementEventHandler initialized");
}

void MinecraftServer::initializeSyncManagers()
{
    MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeSyncManagers");

    if (!m_world) {
        spdlog::error("Cannot initialize sync managers: world not created");
        return;
    }

    m_entitySyncManager = std::make_unique<sync::EntitySyncManager>(m_world->entityManager());

    // ChunkSendManager/BlockUpdateSyncManager/LightSyncManager 在 world 初始化后由子类创建
}

void MinecraftServer::initializeChunkSyncManagers()
{
    MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeChunkSyncManagers");

    if (!m_world || !m_world->chunkManager() || !m_world->lightManager()) {
        spdlog::warn("Cannot initialize chunk sync managers: world not ready");
        return;
    }

    m_blockUpdateSyncManager = std::make_unique<sync::BlockUpdateSyncManager>(m_world->chunkManager()->ticketManager());
    m_blockUpdateSyncManager->setOnBlockUpdate([this](PlayerId playerId, i32 x, i32 y, i32 z, u32 blockStateId) {
        network::BlockUpdatePacket packet(x, y, z, blockStateId);
        network::PacketSerializer ser;
        packet.serialize(ser);

        auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::BlockUpdate, ser.buffer());
        sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
    });

    m_chunkSendManager =
        std::make_unique<sync::ChunkSendManager>(*m_world->chunkManager(), m_world->chunkManager()->ticketManager());

    m_lightSyncManager = std::make_unique<sync::LightSyncManager>(*m_world->lightManager(), *m_world->chunkManager());
    setupChunkSendCallback();
}

void MinecraftServer::initializeRegistries(bool registerEntities)
{
    MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries");

    // 初始化方块注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Blocks");
        VanillaBlocks::initialize();
    }
    spdlog::info("Vanilla blocks initialized");

    // 初始化物品注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Items");
        Items::initialize();
    }
    spdlog::info("Vanilla items initialized");

    // 初始化附魔注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Enchantments");
        item::enchant::EnchantmentRegistry::initialize();
    }
    spdlog::info("Enchantments initialized");

    // 初始化方块物品注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::BlockItems");
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
    spdlog::info("Block items initialized");

    // 初始化物品标签（必须在所有物品注册后）
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::ItemTags");
        item::tag::ItemTags::initialize();
    }
    spdlog::info("Item tags initialized");

    // 初始化发射器行为注册表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::DispenseBehaviors");
        blocks::DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
    }
    spdlog::info("Dispense item behaviors initialized");

    // 初始化战利品表管理器（从数据包加载）
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::LootTables");
        loot::LootTableLoader lootLoader(m_lootTableManager);
        auto dataPackLoadResult = lootLoader.loadFromDataPackList(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load loot tables from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            const auto& result = dataPackLoadResult.value();
            spdlog::info("Loaded {} loot tables from data packs ({} failed)", result.successCount, result.failedCount);
            for (const auto& err : result.errors) {
                spdlog::error("Loot table error: {}", err);
            }
        }
    }

    // 加载配方（从数据包加载）
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Recipes");
        RecipeLoader recipeLoader;
        auto dataPackLoadResult = recipeLoader.loadFromDataPackList(m_dataPackList);
        if (dataPackLoadResult.failed()) {
            spdlog::error("Failed to load crafting recipes from data packs: {}", dataPackLoadResult.error().toString());
        } else {
            spdlog::info("Loaded {} crafting recipes from data packs ({} failed)",
                dataPackLoadResult.value().successCount,
                dataPackLoadResult.value().failedCount);
        }
    }

    // 注册实体类型（可选）
    if (registerEntities) {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Entities");
        entity::VanillaEntities::registerAll();
    }

    // 初始化预定义日程（村民AI行为日程）
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::Schedules");
        entity::ai::brain::schedule::Schedule::initialize();
    }
    spdlog::info("Schedules initialized");

    // 初始化记忆模块类型
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::MemoryModules");
        entity::ai::brain::memory::MemoryModuleTypes::initialize();
    }
    spdlog::info("Memory module types initialized");

    // 初始化村民交易配方表
    {
        MC_TRACE_EVENT("server.initialization", "MinecraftServer::initializeRegistries::VillagerTrades");
        world::village::trade::VillagerTrades::initialize();
    }
}

void MinecraftServer::setupWorldCallbacks()
{
    if (!m_world || !m_world->chunkManager()) {
        return;
    }

    // 设置区块发送管理器（用于区块卸载前发送卸载包）
    m_world->chunkManager()->setChunkSendManager(m_chunkSendManager.get());

    // 设置区块加载回调 - 当区块加载/生成完成时触发
    m_world->chunkManager()->setChunkLoadedCallback([this](ChunkCoord x, ChunkCoord z) {
        // 初始化区块光照
        lightSyncManager().initializeChunkLighting(x, z);
        // 区块加载完成后，自动发送给追踪该区块的玩家
        if (m_chunkSendManager) {
            m_chunkSendManager->sendChunkToTrackingPlayers(x, z);
        }
        // // TODO 通知村庄管理器区块加载
        // if (m_villageManager) {
        //     m_villageManager->onChunkLoaded(x, z);
        // }
    });

    // 设置区块卸载回调 - 当区块即将卸载时触发
    m_world->chunkManager()->setChunkUnloadedCallback([this](ChunkCoord x, ChunkCoord z) {
        // // TODO 通知村庄管理器区块卸载（用于清理 POI 等）
        // if (m_villageManager) {
        //     m_villageManager->onChunkUnloaded(x, z);
        // }
    });

    // 设置追踪变化回调 - 当玩家进入/离开区块视距时触发
    m_world->chunkManager()->ticketManager().setTrackingChangeCallback(
        [this](PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking) {
            if (m_chunkSendManager) {
                m_chunkSendManager->onPlayerTrackingChange(player, x, z, isTracking);
            }
        });

    // 设置实体生成回调
    m_world->chunkManager()->setEntitySpawnCallback([this](const std::vector<SpawnedEntityData>& entities) {
        for (const auto& entityData : entities) {
            const auto* entityType = entity::EntityRegistry::instance().getType(entityData.entityTypeId);
            if (!entityType || !entityType->canSummon()) {
                continue;
            }
            auto entity = entityType->create(m_world);
            if (!entity) {
                continue;
            }
            entity->setPosition(Vector3(entityData.x, entityData.y, entityData.z));
            if (m_world->physicsEngine()) {
                entity->setPhysicsEngine(m_world->physicsEngine());
            }
            const EntityId spawnedId = m_world->spawnEntity(std::move(entity));
            MC_UNUSED(spawnedId);
        }
    });

    // 设置光照变化回调：同步数据到 ChunkSection + 广播给客户端
    m_world->setOnLightChanged([this](LightType type, const SectionPos& pos) {
        // 用MC_TRACE_EVENT会导致编译器死循环，故用MC_TRACE_INSTANT
        MC_TRACE_INSTANT("server.lighting",
            "ServerWorld::OnLightChangedCallback.START",
            "Type",
            (type == LightType::SKY) ? "Sky" : "Block",
            "Section",
            fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
            [flow = ::perfetto::Flow::ProcessScoped(pos.toLong())](::perfetto::EventContext ctx) { flow(ctx); });

        // 同步光照数据到 ChunkSection
        lightSyncManager().markLightChanged(type, pos);

        // 广播光照更新给客户端
        auto* lightManager = m_world->lightManager();
        if (!lightManager) {
            return;
        }

        std::vector<u8> skyLight;
        std::vector<u8> blockLight;

        // 获取光照数据
        if (type == LightType::SKY && lightManager->getSkyLightEngine()) {
            auto* data = lightManager->getData(LightType::SKY, pos);
            if (data) {
                skyLight = data->toByteArray();
            }
        } else if (type == LightType::BLOCK && lightManager->getBlockLightEngine()) {
            auto* data = lightManager->getData(LightType::BLOCK, pos);
            if (data) {
                blockLight = data->toByteArray();
            }
        }

        // 发送光照更新包
        if (!skyLight.empty() || !blockLight.empty()) {
            broadcastLightUpdate(pos.x, pos.z, pos.y, skyLight, blockLight, false);
        }

        MC_TRACE_INSTANT("server.lighting",
            "ServerWorld::OnLightChangedCallback.END",
            "Type",
            (type == LightType::SKY) ? "Sky" : "Block",
            "Section",
            fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
            [flow = ::perfetto::Flow::ProcessScoped(pos.toLong())](::perfetto::EventContext ctx) { flow(ctx); });
    });

    m_world->setOnOpenContainer([this](ContainerType type, const BlockPos& pos, Player& player) {
        return openContainerRequest(type, pos, player);
    });

    // 设置方块变化回调：写入后记录到同步管理器，统一在 tick 末发送。
    m_world->setOnBlockChanged([this](const BlockPos& pos, u32 blockStateId) {
        MC_ASSERT_RELEASE(m_blockUpdateSyncManager != nullptr);
        m_blockUpdateSyncManager->queueBlockUpdate(pos, blockStateId);
    });

    // 设置方块破坏回调 - 播放破坏声音
    m_blockInteractionManager->setOnBlockBreak([this](PlayerId playerId, const BlockPos& pos, const BlockState& state) {
        // MC_TRACE_SERVER_SOUND_EVENT("OnBlockBreak_Callback", "playerId", playerId,
        //                             "x", pos.x, "y", pos.y, "z", pos.z);

        // 获取方块的破坏声音
        const auto& soundType = state.getSoundType();
        Vector3 position(
            static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);

        // MC_TRACE_SERVER_SOUND_EVENT("OnBlockBreak_BroadcastSound",
        //                             "sound", soundType.getBreakSound().toString().c_str(),
        //                             "volume", soundType.getVolume(),
        //                             "pitch", soundType.getPitch());

        // 广播声音给范围内的玩家（16格范围）
        broadcastSoundInRange(soundType.getBreakSound(),
            sound::SoundCategory::Blocks,
            position,
            16.0f * soundType.getVolume(), // 距离 = 16 * volume
            soundType.getVolume(),
            soundType.getPitch());

        // 发送方块更新给所有追踪该区块的玩家
        if (m_chunkSendManager) {
            // 广播方块更新给所有玩家（他们会收到区块更新）
            // 注意：方块更新已经通过 m_world.setBlockState 触发
        }
    });

    // 设置方块放置回调 - 播放放置声音
    m_blockInteractionManager->setOnBlockPlace([this](PlayerId playerId, const BlockPos& pos, const BlockState& state) {
        // 获取方块的放置声音
        const auto& soundType = state.getSoundType();
        Vector3 position(
            static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);

        // 广播声音给范围内的玩家（16格范围）
        broadcastSoundInRange(soundType.getPlaceSound(),
            sound::SoundCategory::Blocks,
            position,
            16.0f * soundType.getVolume(),
            soundType.getVolume(),
            soundType.getPitch());
    });
}

void MinecraftServer::setupChunkSendCallback()
{
    chunkSendManager().setOnChunkSend(
        [this](PlayerId playerId, ChunkCoord x, ChunkCoord z, const std::vector<u8>& data) {
            sendChunkDataToPlayer(playerId, x, z, data);
            MC_TRACE_INSTANT("server.chunk",
                "ChunkSent",
                "Chunk",
                fmt::format("({}, {})", x, z),
                "DataSize",
                data.size(),
                [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(x, z).toId())](
                    ::perfetto::EventContext ctx) { flow(ctx); });
        });

    chunkSendManager().setOnChunkUnload([this](PlayerId playerId, ChunkCoord x, ChunkCoord z) {
        sendUnloadChunkToPlayer(playerId, x, z);
        MC_TRACE_INSTANT("server.chunk", "ChunkUnloaded", "Chunk", fmt::format("({}, {})", x, z));
    });
}

void MinecraftServer::setupRaidManagerCallbacks()
{
    if (!m_world || !m_world->raidManager()) {
        return;
    }

    auto* raidManager = m_world->raidManager();
    world::village::raid::RaidCallbacks callbacks;

    callbacks.onRaidStarted = [this](const world::village::raid::Raid& raid, BlockPos center) {
        broadcastSound(SoundEvents::EVENT_RAID_HORN,
            sound::SoundCategory::Neutral,
            Vector3(static_cast<f32>(center.x) + 0.5f, static_cast<f32>(center.y), static_cast<f32>(center.z) + 0.5f),
            64.0f,
            1.0f);

        spdlog::info("Raid {} started at village center ({}, {}, {})", raid.id(), center.x, center.y, center.z);
    };

    callbacks.onRaidVictory =
        [this](const world::village::raid::Raid& raid, const std::vector<Uuid>& heroes, i32 badOmenLevel) {
            constexpr i32 HERO_EFFECT_DURATION = 48000;

            for (const auto& heroUuid : heroes) {
                const std::string heroUuidStr = util::uuidToString(heroUuid);

                m_playerManager->forEachPlayer([this, &heroUuidStr, badOmenLevel, &raid](ServerPlayerData& playerData) {
                    Player* player = playerEntityManager().getPlayerEntity(playerData.playerId, *m_world);
                    if (player == nullptr || player->uuid() != heroUuidStr) {
                        return;
                    }

                    entity::effect::EffectInstance heroEffect(entity::effect::EffectType::HeroOfTheVillage,
                        HERO_EFFECT_DURATION,
                        badOmenLevel - 1,
                        false,
                        true,
                        true);
                    player->addEffect(std::move(heroEffect));

                    spdlog::info(
                        "Player '{}' (UUID: {}) received Hero of the Village effect (level {}) for raid {} victory",
                        playerData.username,
                        heroUuidStr,
                        badOmenLevel,
                        raid.id());
                });
            }

            BlockPos center = raid.center();
            spdlog::info("Raid {} victory at ({}, {}, {}) - {} heroes rewarded",
                raid.id(),
                center.x,
                center.y,
                center.z,
                heroes.size());
        };

    callbacks.onRaidLoss = [](const world::village::raid::Raid& raid) {
        BlockPos center = raid.center();
        spdlog::info("Raid {} failed at ({}, {}, {})", raid.id(), center.x, center.y, center.z);
    };

    callbacks.onWaveStarted = [](const world::village::raid::Raid& raid, i32 wave, BlockPos spawnPos) {
        spdlog::info("Raid {} wave {} started at ({}, {}, {})", raid.id(), wave, spawnPos.x, spawnPos.y, spawnPos.z);
    };

    raidManager->setCallbacks(std::move(callbacks));
}

bool MinecraftServer::openContainerRequest(ContainerType type, const BlockPos& pos, Player& player)
{
    return containerManager().openContainer(player.playerId(), type, pos).success();
}

void MinecraftServer::shutdownManagers()
{
    if (m_storage && m_storage->isOpen()) {
        auto saveResult = saveAllWorldData();
        if (saveResult.failed()) {
            spdlog::error("Failed to save world during shutdown: {}", saveResult.error().message());
        }
    }

    // 关闭成就事件处理器
    m_advancementEventHandler.shutdown();

    m_blockUpdateSyncManager.reset();
    m_lightSyncManager.reset();
    m_chunkSendManager.reset();
    m_entitySyncManager.reset();
    m_inventoryManager.reset();
    m_containerManager.reset();
    m_miningManager.reset();
    m_blockInteractionManager.reset();
    m_commandRegistry.reset();
    if (m_dimensionManager) {
        m_dimensionManager->shutdown();
    }
    m_dimensionManager.reset();
    m_world = nullptr;
    shutdownSharedStorage();
    m_gameModeManager.reset();
    m_packetHandler.reset();
    m_positionTracker.reset();
    m_keepAliveManager.reset();
    m_teleportManager.reset();
    m_timeManager.reset();
    m_connectionManager.reset();
    m_playerManager.reset();
}

void MinecraftServer::tickEntities()
{
    if (!m_world) return;

    MC_TRACE_EVENT("server.tick", "EntityTick", "phase", "entities");
    m_world->entityManager().tick();

    // 物品拾取处理
    m_world->itemPickupManager().tick(*this);

    // 实体追踪更新
    m_world->entityTracker().tick(*this);
}

void MinecraftServer::tickKeepAlive()
{
    u64 tick = currentTick();
    if (tick - m_lastKeepAliveTick >= KEEPALIVE_INTERVAL) {
        m_lastKeepAliveTick = tick;
        MC_TRACE_EVENT("server.network", "SendKeepAlive", "phase", "keepalive_send");
        sendKeepAliveToAll();
    }
}

void MinecraftServer::sendTimeUpdate()
{
    const auto& time = timeManager().gameTimeObj();
    network::TimeUpdatePacket packet(time.gameTime(), time.dayTime(), time.daylightCycleEnabled());

    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::TimeUpdate, ser.buffer());
    broadcastPacket(fullPacket.data(), fullPacket.size());
}

void MinecraftServer::sendWeatherUpdate()
{
    MC_TRACE_EVENT("server.tick", "sendWeatherUpdate", "phase", "weather_sync");

    if (!m_world || !m_world->weatherManager()) return;

    auto& weatherMgr = *m_world->weatherManager();
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    constexpr f32 STRENGTH_THRESHOLD = 0.001f;
    bool rainChanged = std::abs(rainStrength - m_lastSentRainStrength) > STRENGTH_THRESHOLD;
    bool thunderChanged = std::abs(thunderStrength - m_lastSentThunderStrength) > STRENGTH_THRESHOLD;

    if (rainChanged) {
        auto packet = network::GameStateChangePacket::rainStrength(rainStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            broadcastPacket(fullPacket.data(), fullPacket.size());
        }
        m_lastSentRainStrength = rainStrength;
    }

    if (thunderChanged) {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            broadcastPacket(fullPacket.data(), fullPacket.size());
        }
        m_lastSentThunderStrength = thunderStrength;
    }

    if (weatherMgr.hasWeatherChanged()) {
        auto weatherType = weatherMgr.weatherType();
        if (weatherType == weather::WeatherType::Clear) {
            auto packet = network::GameStateChangePacket::endRain();
            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket =
                    core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
                broadcastPacket(fullPacket.data(), fullPacket.size());
            }
        } else if (weatherType == weather::WeatherType::Rain || weatherType == weather::WeatherType::Thunder) {
            auto packet = network::GameStateChangePacket::beginRain();
            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket =
                    core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
                broadcastPacket(fullPacket.data(), fullPacket.size());
            }
        }
    }
}

void MinecraftServer::sendInitialWeatherStateToPlayer(PlayerId playerId)
{
    MC_TRACE_EVENT("server.player", "SendInitialWeatherState", "phase", "weather_sync");

    if (!m_world || !m_world->weatherManager()) return;

    auto& weatherMgr = *m_world->weatherManager();
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    {
        auto packet = network::GameStateChangePacket::rainStrength(rainStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }

    {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket =
                core::ConnectionManager::encapsulatePacket(network::PacketType::GameStateChange, result.value());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }
}

void MinecraftServer::sendInitialDifficultyToPlayer(PlayerId playerId)
{
    MC_TRACE_EVENT("server.player", "SendInitialDifficulty", "phase", "difficulty_sync");

    auto fullPacket = serializeDifficultyPacket();
    if (fullPacket.empty()) {
        spdlog::error("Failed to serialize ServerDifficultyPacket for player {}", playerId);
        return;
    }

    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::sendKeepAliveToAll()
{
    MC_TRACE_EVENT("server.player", "SendKeepAlive", "phase", "keepalive_sync");

    u64 timestamp = util::TimeUtils::getCurrentTimeMs();
    u64 tick = currentTick();

    m_playerManager->forEachPlayer([this, timestamp, tick](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            network::KeepAlivePacket packet;
            packet.setTimestamp(timestamp);

            auto result = packet.serialize();
            if (result.success()) {
                sendPacketToPlayer(player.playerId, result.value().data(), result.value().size());
            }

            m_keepAliveManager->recordKeepAliveSent(player.playerId, timestamp, tick);
        }
    });
}

void MinecraftServer::initializeCreativeInventory(PlayerInventory& inventory)
{
    inventory.clear();
    inventory.setSelectedSlot(0);
    fillCreativeModeInventory(inventory);
}

void MinecraftServer::sendChunkDataToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z, const std::vector<u8>& data)
{
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

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::ChunkData, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
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

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::UnloadChunk, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastLightUpdate(ChunkCoord x,
    ChunkCoord z,
    i32 sectionY,
    const std::vector<u8>& skyLight,
    const std::vector<u8>& blockLight,
    bool trustEdges)
{
    MC_TRACE_EVENT("server.lighting",
        "BroadcastLightUpdate",
        "Section",
        fmt::format("({}, {}, {})", x, sectionY, z),
        "SkyLightSize",
        skyLight.size(),
        "BlockLightSize",
        blockLight.size(),
        [flow = ::perfetto::Flow::ProcessScoped(SectionPos(x, sectionY, z).toLong())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    network::LightUpdatePacket packet(
        x, z, sectionY, std::vector<u8>(skyLight), std::vector<u8>(blockLight), trustEdges);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::LightUpdate, ser.buffer());
    m_playerManager->forEachPlayer([&fullPacket](ServerPlayerData& player) {
        if (player.loggedIn && player.hasConnection()) {
            player.send(fullPacket.data(), fullPacket.size());
        }
    });
}

u64 MinecraftServer::currentTick() const
{
    return m_timeManager ? m_timeManager->currentTick() : m_tickCounter;
}

ServerChunkManager& MinecraftServer::chunkManager()
{
    MC_ASSERT(m_world != nullptr);
    return *m_world->chunkManager();
}

const ServerChunkManager& MinecraftServer::chunkManager() const
{
    MC_ASSERT(m_world != nullptr);
    return *m_world->chunkManager();
}

WorldLightManager* MinecraftServer::lightManager()
{
    return m_world ? m_world->lightManager() : nullptr;
}

const WorldLightManager* MinecraftServer::lightManager() const
{
    return m_world ? m_world->lightManager() : nullptr;
}

mc::EntityManager& MinecraftServer::entityManager()
{
    MC_ASSERT(m_world != nullptr);
    return m_world->entityManager();
}

const mc::EntityManager& MinecraftServer::entityManager() const
{
    MC_ASSERT(m_world != nullptr);
    return m_world->entityManager();
}

EntityTracker& MinecraftServer::entityTracker()
{
    MC_ASSERT(m_world != nullptr);
    return m_world->entityTracker();
}

const EntityTracker& MinecraftServer::entityTracker() const
{
    MC_ASSERT(m_world != nullptr);
    return m_world->entityTracker();
}

PhysicsEngine* MinecraftServer::physicsEngine()
{
    return m_world ? m_world->physicsEngine() : nullptr;
}

const PhysicsEngine* MinecraftServer::physicsEngine() const
{
    return m_world ? m_world->physicsEngine() : nullptr;
}

WeatherManager& MinecraftServer::weatherManager()
{
    MC_ASSERT(m_world != nullptr && m_world->weatherManager() != nullptr);
    return *m_world->weatherManager();
}

const WeatherManager& MinecraftServer::weatherManager() const
{
    MC_ASSERT(m_world != nullptr && m_world->weatherManager() != nullptr);
    return *m_world->weatherManager();
}

ItemPickupManager& MinecraftServer::itemPickupManager()
{
    MC_ASSERT(m_world != nullptr);
    return m_world->itemPickupManager();
}

const ItemPickupManager& MinecraftServer::itemPickupManager() const
{
    MC_ASSERT(m_world != nullptr);
    return m_world->itemPickupManager();
}

// ============================================================================
// 数据包处理方法
// ============================================================================

void MinecraftServer::handlePlayerMovePacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerMovePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse player move from player {}", playerId);
        return;
    }

    auto& packet = result.value();
    const auto& pos = packet.position();

    // 保存旧位置用于村庄进入检测
    BlockPos prevPos(static_cast<i32>(player->x), static_cast<i32>(player->y), static_cast<i32>(player->z));

    // 计算新区块坐标
    ChunkCoord newChunkX = static_cast<ChunkCoord>(std::floor(pos.x / 16.0));
    ChunkCoord newChunkZ = static_cast<ChunkCoord>(std::floor(pos.z / 16.0));

    // 检查玩家是否移动到了新区块
    ChunkCoord oldChunkX = static_cast<ChunkCoord>(std::floor(player->x / 16.0f));
    ChunkCoord oldChunkZ = static_cast<ChunkCoord>(std::floor(player->z / 16.0f));
    bool chunkChanged = (newChunkX != oldChunkX || newChunkZ != oldChunkZ);

    switch (packet.type()) {
        case network::PlayerMovePacket::MoveType::Full:
            player->x = static_cast<f32>(pos.x);
            player->y = static_cast<f32>(pos.y);
            player->z = static_cast<f32>(pos.z);
            player->yaw = pos.yaw;
            player->pitch = pos.pitch;
            break;
        case network::PlayerMovePacket::MoveType::Position:
            player->x = static_cast<f32>(pos.x);
            player->y = static_cast<f32>(pos.y);
            player->z = static_cast<f32>(pos.z);
            break;
        case network::PlayerMovePacket::MoveType::Rotation:
            player->yaw = pos.yaw;
            player->pitch = pos.pitch;
            break;
        case network::PlayerMovePacket::MoveType::GroundOnly:
            player->onGround = pos.onGround;
            break;
    }

    if (m_world) {
        m_world->entityManager().forEachEntity([playerId, player](Entity* entity) {
            auto* playerEntity = dynamic_cast<Player*>(entity);
            if (playerEntity == nullptr || playerEntity->playerId() != playerId) {
                return true;
            }

            playerEntity->setPosition(player->x, player->y, player->z);
            playerEntity->setRotation(player->yaw, player->pitch);
            playerEntity->setOnGround(player->onGround);
            return false;
        });
    }

    m_positionTracker->updatePosition(
        playerId, player->x, player->y, player->z, player->yaw, player->pitch, player->onGround);
    updateEntityTrackingForPlayer(playerId, player->x, player->y, player->z);

    // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
    // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
    if (m_world && m_world->chunkManager()) {
        m_world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
        m_world->chunkManager()->processTicketUpdatesSync();
    }

    // 村庄进入检测（用于触发袭击）
    // 仅在位置实际改变时检测
    if (m_world && packet.type() != network::PlayerMovePacket::MoveType::Rotation &&
        packet.type() != network::PlayerMovePacket::MoveType::GroundOnly) {
        auto* villageManager = m_world->villageManager();
        auto* raidManager = m_world->raidManager();
        if (villageManager && raidManager) {
            BlockPos currentPos(static_cast<i32>(player->x), static_cast<i32>(player->y), static_cast<i32>(player->z));
            world::village::Village* enteredVillage = villageManager->checkPlayerEnterVillage(currentPos, prevPos);
            if (enteredVillage) {
                // 玩家进入了村庄，使用回调检查不祥之兆并触发袭击
                raidManager->onPlayerEnterVillageWithCallback(
                    [player](BlockPos) -> i32 {
                        if (player->hasEffect(entity::effect::EffectType::BadOmen)) {
                            const entity::effect::EffectInstance* effect =
                                player->getEffect(entity::effect::EffectType::BadOmen);
                            if (effect != nullptr) {
                                i32 level = effect->getEffectLevel();
                                // 移除不祥之兆效果
                                player->removeEffect(entity::effect::EffectType::BadOmen);
                                return level;
                            }
                        }
                        return 0;
                    },
                    enteredVillage);
            }
        }
    }
}

void MinecraftServer::handleTeleportConfirmPacket(PlayerId playerId, const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse teleport confirm from player {}", playerId);
        return;
    }

    auto& packet = result.value();

    if (m_teleportManager->confirmTeleport(playerId, packet.teleportId())) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (!player) {
            return;
        }

        updateEntityTrackingForPlayer(playerId, player->x, player->y, player->z);

        // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
        // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
        if (m_world && m_world->chunkManager()) {
            m_world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
            m_world->chunkManager()->processTicketUpdatesSync();
        }
    }
}

void MinecraftServer::handleKeepAlivePacket(PlayerId playerId, const u8* data, size_t size)
{
    network::KeepAlivePacket packet;
    auto result = packet.deserialize(data, size);

    if (result.success()) {
        u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
        m_keepAliveManager->handleKeepAliveResponse(playerId, packet.timestamp(), currentTimeMs);
    } else {
        spdlog::error("Failed to parse keep alive packet from player {}: {}", playerId, result.error().message());
    }
}

void MinecraftServer::handleChatMessagePacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::ChatMessagePacket::deserialize(deser);

    if (result.failed()) {
        spdlog::error("Failed to parse chat message packet from player {}: {}", playerId, result.error().message());
        return;
    }

    auto& packet = result.value();
    std::string message = packet.message();

    if (!message.empty() && message[0] == '/') {
        // 执行命令
        mc::command::ServerCommandSource source(this,
            nullptr,
            m_world,
            Vector3d(player->x, player->y, player->z),
            Vector2f(player->yaw, player->pitch),
            4,
            playerId,
            player->username);
        auto cmdResult = m_commandRegistry->execute(message, source);
        if (cmdResult.failed()) {
            spdlog::warn("Command '{}' failed for {}: {}", message, player->username, cmdResult.error().toString());
        } else {
            spdlog::info("Command '{}' executed for {} with result {}", message, player->username, cmdResult.value());
        }
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, message);
}

void MinecraftServer::updateEntityTrackingForPlayer(PlayerId playerId, f64 x, f64 y, f64 z)
{
    MC_TRACE_EVENT(
        "server.world", "MinecraftServer::updateEntityTrackingForPlayer", "playerId", playerId, "x", x, "y", y, "z", z);

    if (!m_world) {
        return;
    }

    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    m_world->entityTracker().updatePlayerTracking(
        *this, playerId, Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)));
}

void MinecraftServer::handleBlockInteractionPacket(PlayerId playerId, const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);
    auto result = network::BlockInteractionPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse block interaction packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    BlockPos pos(packet.x(), packet.y(), packet.z());

    MC_TRACE_EVENT("server.world",
        "MinecraftServer::handleBlockInteractionPacket",
        "pos",
        pos.toString(),
        "playerId",
        playerId,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 处理挖掘状态
    miningManager().handleBlockInteraction(playerId, pos, packet.action());

    if (packet.action() == network::BlockInteractionAction::StopDestroyBlock) {
        if (!miningManager().tryCompleteMining(playerId, pos)) {
            spdlog::warn("Ignored premature StopDestroyBlock from player {} at {}", playerId, pos.toString());
        }
    }
}

void MinecraftServer::handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto result = network::PlayerTryUseItemOnBlockPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::error("Failed to parse block placement packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    const BlockPos pos(packet.x(), packet.y(), packet.z());
    const BlockState* clickedState = m_world ? m_world->getBlockState(pos) : nullptr;
    const Hand hand = (packet.hand() == static_cast<u8>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    const auto tryOpenCrafting = [this, playerId, pos, clickedState]() {
        return isCraftingTableState(clickedState) && tryOpenCraftingContainer(playerId, pos);
    };

    ItemStack heldStack = getHeldItemForPlacement(playerId);
    if (heldStack.isEmpty()) {
        if (!tryOpenCrafting()) {
            (void)blockInteractionManager().handleBlockUse(playerId, pos, hand, packet.hitPosition(), packet.face());
        }
        return;
    }

    const Item* heldItem = heldStack.getItem();
    const bool holdingBlockItem =
        heldItem != nullptr && BlockItemRegistry::instance().getBlockItemByItemId(heldItem->itemId()) != nullptr;

    if (!holdingBlockItem) {
        if (!tryOpenCrafting()) {
            (void)blockInteractionManager().handleBlockUse(playerId, pos, hand, packet.hitPosition(), packet.face());
        }
        return;
    }

    auto interactionResult =
        blockInteractionManager().handleBlockPlacement(playerId, pos, packet.hitPosition(), packet.face(), heldStack);

    if (interactionResult.success() && interactionResult.value().blockPlaced &&
        interactionResult.value().itemConsumed) {
        const i32 selectedSlot = getSelectedHotbarSlot(playerId);
        ItemStack updatedStack = heldStack;
        updatedStack.shrink(1);
        setInventoryItem(playerId, selectedSlot, updatedStack);
        syncPlayerInventory(playerId);
    }
}

void MinecraftServer::handleCreativeInventoryActionPacket(PlayerId playerId, const u8* data, size_t size)
{
    auto* player = m_playerManager->getPlayer(playerId);
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

    setInventoryItem(playerId, slotIndex, packet.item());
    syncPlayerInventory(playerId);
}

// ============================================================================
// 数据包发送辅助方法
// ============================================================================

void MinecraftServer::sendTeleportPacket(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Teleport, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory)
{
    MC_UNUSED(playerId);
    MC_UNUSED(inventory);
}

void MinecraftServer::setupInitialPlayerState(ServerPlayerData* player, GameMode gameMode)
{
    if (!player) return;

    // 获取世界出生点
    Vector3d spawnPoint(0.0, 64.0, 0.0); // 默认值
    if (m_world) {
        spawnPoint = m_world->worldSpawnPoint();
    }

    // 设置初始位置
    player->x = static_cast<f32>(spawnPoint.x);
    player->y = static_cast<f32>(spawnPoint.y);
    player->z = static_cast<f32>(spawnPoint.z);

    // 设置游戏状态
    player->loggedIn = true;
    player->gameMode = gameMode;
}

void MinecraftServer::sendInitialGameState(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch)
{
    // 发送传送
    u32 teleportId = m_teleportManager->requestTeleport(playerId, x, y, z, yaw, pitch);
    sendTeleportPacket(playerId, x, y, z, yaw, pitch, teleportId);

    // 立即发送时间，避免客户端在首次周期同步前短暂显示默认时间(0)
    const auto& time = timeManager().gameTimeObj();
    network::TimeUpdatePacket timePacket(time.gameTime(), time.dayTime(), time.daylightCycleEnabled());
    network::PacketSerializer timeSer;
    timePacket.serialize(timeSer);
    auto fullTimePacket = core::ConnectionManager::encapsulatePacket(network::PacketType::TimeUpdate, timeSer.buffer());
    sendPacketToPlayer(playerId, fullTimePacket.data(), fullTimePacket.size());

    // 发送世界出生点（指南针指向位置）
    Vector3d worldSpawn = m_world->worldSpawnPoint();
    network::SpawnPositionPacket spawnPosPacket(BlockPos(static_cast<BlockCoord>(worldSpawn.x),
        static_cast<BlockCoord>(worldSpawn.y),
        static_cast<BlockCoord>(worldSpawn.z)));
    auto spawnPosResult = spawnPosPacket.serialize();
    if (spawnPosResult.success()) {
        auto fullSpawnPacket =
            core::ConnectionManager::encapsulatePacket(network::PacketType::SpawnPosition, spawnPosResult.value());
        sendPacketToPlayer(playerId, fullSpawnPacket.data(), fullSpawnPacket.size());
    }

    // 发送初始天气状态
    sendInitialWeatherStateToPlayer(playerId);

    // 发送初始难度状态
    sendInitialDifficultyToPlayer(playerId);

    updateEntityTrackingForPlayer(playerId, x, y, z);
}

void MinecraftServer::sendCommandTreePacket(PlayerId playerId)
{
    MC_ASSERT_RELEASE(m_commandRegistry != nullptr);

    network::CommandTreePacket packet(m_commandRegistry->getCommandTreeJson());
    auto serializeResult = packet.serialize();
    MC_ASSERT_RELEASE(serializeResult.success());

    auto fullPacket =
        core::ConnectionManager::encapsulatePacket(network::PacketType::CommandTree, serializeResult.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::stopCore()
{
    spdlog::info("Stopping server core...");

    // 停止 Worker 线程池，避免后续关服过程中继续提交任务
    m_computationWorkerPool.shutdown();
    m_ioWorkerPool.shutdown();

    // 断开所有玩家
    if (m_connectionManager) {
        m_connectionManager->disconnectAll("Server shutting down");
    }

    // 关闭管理器
    shutdownManagers();

    spdlog::info("Server core stopped.");
}

void MinecraftServer::dispatchPacket(u32 sessionId, const u8* data, size_t size)
{
    if (size < network::PACKET_HEADER_SIZE) {
        spdlog::warn("Packet too small: {} bytes", size);
        return;
    }

    network::PacketDeserializer deser(data, size);
    auto sizeResult = deser.readU32();
    auto typeResult = deser.readU16();

    if (sizeResult.failed() || typeResult.failed()) {
        spdlog::warn("Failed to read packet header");
        return;
    }

    network::PacketType packetType = static_cast<network::PacketType>(typeResult.value());
    const u8* payload = data + network::PACKET_HEADER_SIZE;
    size_t payloadSize = size - network::PACKET_HEADER_SIZE;

    MC_TRACE_EVENT("server.network",
        "DispatchPacketToHandler",
        "sessionId",
        sessionId,
        "packetType",
        static_cast<int>(packetType),
        "payloadSize",
        payloadSize);

    switch (packetType) {
        case network::PacketType::LoginRequest: {
            MC_TRACE_EVENT(
                "server.network", "HandleLoginRequestPacket", "sessionId", sessionId, "payloadSize", payloadSize);
            handleLoginRequestPacket(sessionId, payload, payloadSize);
            break;
        }

        case network::PacketType::PlayerMove: {
            MC_TRACE_EVENT(
                "server.network", "HandlePlayerMovePacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handlePlayerMovePacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::BlockInteraction: {
            MC_TRACE_EVENT(
                "server.network", "HandleBlockInteractionPacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleBlockInteractionPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::PlayerTryUseItemOnBlock: {
            MC_TRACE_EVENT("server.network",
                "HandlePlayerTryUseItemOnBlockPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleBlockPlacementPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::HotbarSelect: {
            MC_TRACE_EVENT(
                "server.network", "HandleHotbarSelectPacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleHotbarSelectPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::CreativeInventoryAction: {
            MC_TRACE_EVENT("server.network",
                "HandleCreativeInventoryActionPacket",
                "sessionId",
                sessionId,
                "payloadSize",
                payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleCreativeInventoryActionPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::ContainerClick: {
            MC_TRACE_EVENT(
                "server.network", "HandleContainerClickPacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleContainerClickPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::CloseContainer: {
            MC_TRACE_EVENT(
                "server.network", "HandleCloseContainerPacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleCloseContainerPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::TeleportConfirm: {
            MC_TRACE_EVENT(
                "server.network", "HandleTeleportConfirmPacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleTeleportConfirmPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::KeepAlive: {
            MC_TRACE_EVENT(
                "server.network", "HandleKeepAlivePacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleKeepAlivePacket(playerId, data, size);
            }
            break;
        }

        case network::PacketType::ChatMessage: {
            MC_TRACE_EVENT(
                "server.network", "HandleChatMessagePacket", "sessionId", sessionId, "payloadSize", payloadSize);
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleChatMessagePacket(playerId, payload, payloadSize);
            }
            break;
        }

        default:
            spdlog::warn("Unhandled packet type: {}", static_cast<int>(packetType));
            break;
    }
}

// ============================================================================
// 声音广播方法
// ============================================================================

void MinecraftServer::broadcastSound(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    // spdlog::debug("[Sound] Broadcasting sound: {} at ({}, {}, {})",
    //               soundEventId.toString(), position.x, position.y, position.z);

    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::warn("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlaySound, result.value());
    broadcastPacket(fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastSoundInRange(const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 range,
    f32 volume,
    f32 pitch)
{
    // spdlog::debug("[Sound] Broadcasting sound in range: {} at ({}, {}, {}) range={}",
    //               soundEventId.toString(), position.x, position.y, position.z, range);

    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlaySound, result.value());

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_playerManager->forEachPlayer([this, &position, range, &fullPacket, &playersNotified](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - position.x;
        f32 dy = player.y - position.y;
        f32 dz = player.z - position.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
            playersNotified++;
        }
    });

    // spdlog::debug("[Sound] Sound {} sent to {} players", soundEventId.toString(), playersNotified);
}

void MinecraftServer::sendSoundToPlayer(PlayerId playerId,
    const ResourceLocation& soundEventId,
    sound::SoundCategory category,
    const Vector3& position,
    f32 volume,
    f32 pitch)
{
    // spdlog::debug("[Sound] Sending sound {} to player {}", soundEventId.toString(), playerId);

    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::PlaySound, result.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

// ============================================================================
// 粒子广播
// ============================================================================

void MinecraftServer::broadcastParticleInRange(client::renderer::trident::particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count,
    f32 range)
{
    network::ParticlePacket packet(type, pos, velocity, offset, count);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());

    // 只发送给范围内的玩家
    u32 playersNotified = 0;
    m_playerManager->forEachPlayer([this, &pos, range, &fullPacket, &playersNotified](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 dx = player.x - pos.x;
        f32 dy = player.y - pos.y;
        f32 dz = player.z - pos.z;
        f32 distSq = dx * dx + dy * dy + dz * dz;
        f32 rangeSq = range * range;

        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
            playersNotified++;
        }
    });

    // spdlog::debug("[Particle] Particle type {} sent to {} players",
    //               static_cast<u16>(type), playersNotified);
}

void MinecraftServer::sendParticleToPlayer(PlayerId playerId,
    client::renderer::trident::particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
{
    network::ParticlePacket packet(type, pos, velocity, offset, count);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ParticlePacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Particle, result.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastParticleInRange(u32 type,
    f64 x,
    f64 y,
    f64 z,
    f32 velocityX,
    f32 velocityY,
    f32 velocityZ,
    f32 offsetX,
    f32 offsetY,
    f32 offsetZ,
    u32 count,
    f32 range)
{
    // 转换为强类型枚举并调用现有的实现
    auto particleType = static_cast<client::renderer::trident::particle::ParticleTypeId>(type);
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    Vector3 velocity(velocityX, velocityY, velocityZ);
    Vector3 offset(offsetX, offsetY, offsetZ);

    broadcastParticleInRange(particleType, pos, velocity, offset, count, range);
}

void MinecraftServer::broadcastEntityStatusInRange(EntityId entityId, u8 status, const Vector3& pos, f32 range)
{
    network::EntityStatusPacket packet;
    packet.setEntityId(static_cast<u32>(entityId));
    packet.setStatus(static_cast<network::EntityStatusPacket::Status>(status));

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize EntityStatusPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::EntityStatus, result.value());

    f32 rangeSq = range * range;
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

// ============================================================================
// 世界事件广播
// ============================================================================

void MinecraftServer::broadcastWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data)
{
    sound::WorldEventPacket packet(eventId, x, y, z, data);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize WorldEventPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::WorldEvent, result.value());
    broadcastPacket(fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastWorldEventInRange(i32 eventId, i32 x, i32 y, i32 z, i32 data, f32 range)
{
    sound::WorldEventPacket packet(eventId, x, y, z, data);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize WorldEventPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::WorldEvent, result.value());

    f32 rangeSq = range * range;
    Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    m_playerManager->forEachPlayer([this, &pos, rangeSq, &fullPacket](ServerPlayerData& player) {
        if (!player.loggedIn || !player.hasConnection()) {
            return;
        }

        f32 distSq = math::distanceSq(player.x, player.y, player.z, pos.x, pos.y, pos.z);
        if (distSq <= rangeSq) {
            sendPacketToPlayer(player.playerId, fullPacket.data(), fullPacket.size());
        }
    });
}

// ============================================================================
// 爆炸广播
// ============================================================================

void MinecraftServer::broadcastExplosionInRange(const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback,
    f32 range)
{
    // 参考 MC 1.16.5: 发送给爆炸点 64 格范围内的玩家
    // 每个玩家收到的击退向量不同，需要为每个玩家单独构建数据包

    f32 rangeSq = range * range;

    m_playerManager->forEachPlayer(
        [this, &position, strength, &affectedBlocks, &playerKnockback, rangeSq](ServerPlayerData& player) {
            if (!player.loggedIn || !player.hasConnection()) {
                return;
            }

            // 检查玩家是否在范围内
            f32 dx = player.x - position.x;
            f32 dy = player.y - position.y;
            f32 dz = player.z - position.z;
            f32 distSq = dx * dx + dy * dy + dz * dz;

            if (distSq <= rangeSq) {
                // 为每个玩家创建单独的爆炸包（击退向量不同）
                sendExplosionToPlayer(player.playerId, position, strength, affectedBlocks, playerKnockback);
            }
        });
}

void MinecraftServer::sendExplosionToPlayer(PlayerId playerId,
    const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback)
{
    // 创建爆炸包，包含该玩家的击退向量
    network::ExplosionPacket packet(position, strength, affectedBlocks, playerKnockback, static_cast<u64>(playerId));

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::error("Failed to serialize ExplosionPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::Explosion, result.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

} // namespace mc::server
