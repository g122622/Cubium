#include "MinecraftServer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkLoadTicket.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/packet/GameStateChangePacket.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

MinecraftServer::MinecraftServer(const ServerCoreConfig& config)
    : m_config(config)
    , m_lootTableManager()
{
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
    MC_TRACE_EVENT("server.tick", "MinecraftServerTick", "phase", "server_tick");

    if (!m_running.load()) {
        return;
    }

    // 执行核心 tick
    tickCore();

    // 更新所有维度
    if (m_dimensionManager) {
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

    // 处理区块发送队列
    chunkSendManager().processPendingSends();

    // 更新区块管理器
    if (m_world && m_world->chunkManager()) {
        m_world->chunkManager()->tick();
    }

    // 更新光照
    tickLighting();

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
    // 创建核心管理器
    m_playerManager = std::make_unique<core::PlayerManager>(m_config);
    m_connectionManager = std::make_unique<core::ConnectionManager>(*m_playerManager);
    // 与 Java 版一致：世界初始白天时间从 1000 开始（清晨后）
    m_timeManager = std::make_unique<core::TimeManager>(0, 1000);
    m_teleportManager = std::make_unique<core::TeleportManager>(*m_playerManager);
    m_keepAliveManager = std::make_unique<core::KeepAliveManager>(*m_playerManager, m_config);
    m_positionTracker = std::make_unique<core::PositionTracker>(*m_playerManager, m_config);
    m_packetHandler = std::make_unique<core::PacketHandler>(
        *m_playerManager,
        *m_connectionManager,
        *m_teleportManager,
        *m_keepAliveManager,
        *m_positionTracker,
        *m_timeManager,
        m_config);
    m_gameModeManager = std::make_unique<core::GameModeManager>(*m_playerManager, *m_connectionManager);

    // 创建维度管理器
    m_dimensionManager = std::make_unique<ServerDimensionManager>(this);
}

Result<void> MinecraftServer::initializeWorld()
{
    // ServerWorld 在子类中创建
    if (!m_world) {
        return Error(ErrorCode::NotInitialized, "World not created");
    }

    // 初始化物理引擎
    // TODO: 创建碰撞世界适配器

    // 初始化命令注册表
    m_commandRegistry = std::make_unique<command::CommandRegistry>();

    return Result<void>::ok();
}

void MinecraftServer::initializeInteractionManagers()
{
    m_blockInteractionManager = std::make_unique<interaction::BlockInteractionManager>(
        *m_world, *m_playerManager, m_lootTableManager);

    m_miningManager = std::make_unique<interaction::MiningManager>(
        *m_playerManager, *m_connectionManager);

    m_containerManager = std::make_unique<interaction::ContainerManager>(
        *m_playerManager);

    m_inventoryManager = std::make_unique<interaction::InventoryManager>(
        *m_playerManager);

    m_containerManager->setInventoryManager(m_inventoryManager.get());
}

void MinecraftServer::initializeSyncManagers()
{
    if (!m_world) {
        spdlog::error("Cannot initialize sync managers: world not created");
        return;
    }

    m_entitySyncManager = std::make_unique<sync::EntitySyncManager>(
        m_world->entityManager());

    // ChunkSendManager 和 LightSyncManager 在 world 初始化后由子类创建
}

void MinecraftServer::initializeChunkSyncManagers()
{
    if (!m_world || !m_world->chunkManager() || !m_world->lightManager()) {
        spdlog::warn("Cannot initialize chunk sync managers: world not ready");
        return;
    }

    m_chunkSendManager = std::make_unique<sync::ChunkSendManager>(
        *m_world->chunkManager(),
        m_world->chunkManager()->ticketManager());

    m_lightSyncManager = std::make_unique<sync::LightSyncManager>(
        *m_world->lightManager(), *m_world->chunkManager());
}

void MinecraftServer::initializeRegistries(bool registerEntities)
{
    // 初始化方块注册表
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

    // 初始化物品注册表
    Items::initialize();
    spdlog::info("Vanilla items initialized");

    // 初始化附魔注册表
    item::enchant::EnchantmentRegistry::initialize();
    spdlog::info("Enchantments initialized");

    // 初始化方块物品注册表
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    spdlog::info("Block items initialized");

    // 加载配方
    RecipeLoader recipeLoader;
    auto recipeLoadResult = recipeLoader.loadFromDirectory("data/minecraft/recipes");
    if (recipeLoadResult.failed()) {
        spdlog::warn("Failed to load crafting recipes: {}", recipeLoadResult.error().toString());
    } else {
        spdlog::info("Loaded {} crafting recipes ({} failed)",
                     recipeLoadResult.value().successCount,
                     recipeLoadResult.value().failedCount);
    }

    // 注册实体类型（可选）
    if (registerEntities) {
        entity::VanillaEntities::registerAll();
    }

    // 初始化预定义日程（村民AI行为日程）
    entity::ai::brain::schedule::Schedule::initialize();
    spdlog::info("Schedules initialized");

    // 初始化记忆模块类型
    entity::ai::brain::memory::MemoryModuleTypes::initialize();
    spdlog::info("Memory module types initialized");

    // 初始化村民交易配方表
    world::village::trade::VillagerTrades::initialize();
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
            auto entity = entityType->create(m_world.get());
            if (!entity) {
                continue;
            }
            entity->setPosition(Vector3(entityData.x, entityData.y, entityData.z));
            if (m_world->physicsEngine()) {
                entity->setPhysicsEngine(m_world->physicsEngine());
            }
            m_world->entityManager().addEntity(std::move(entity));
        }
    });

    // 设置光照变化回调：同步数据到 ChunkSection + 广播给客户端
    m_world->setOnLightChanged([this](LightType type, const SectionPos& pos) {
        MC_TRACE_INSTANT("server.lighting", "OnLightChanged",
                   "Type", (type == LightType::SKY) ? "Sky" : "Block",
                   "Section", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
                   [flow = ::perfetto::Flow::ProcessScoped(pos.toLong())](::perfetto::EventContext ctx) {
                       flow(ctx);
        });

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
    });

    // 设置方块破坏回调 - 播放破坏声音
    m_blockInteractionManager->setOnBlockBreak(
        [this](PlayerId playerId, const BlockPos& pos, const BlockState& state) {
            // MC_TRACE_SERVER_SOUND_EVENT("OnBlockBreak_Callback", "playerId", playerId,
            //                             "x", pos.x, "y", pos.y, "z", pos.z);

            // 获取方块的破坏声音
            const auto& soundType = state.getSoundType();
            Vector3 position(static_cast<f32>(pos.x) + 0.5f,
                           static_cast<f32>(pos.y) + 0.5f,
                           static_cast<f32>(pos.z) + 0.5f);

            // MC_TRACE_SERVER_SOUND_EVENT("OnBlockBreak_BroadcastSound",
            //                             "sound", soundType.getBreakSound().toString().c_str(),
            //                             "volume", soundType.getVolume(),
            //                             "pitch", soundType.getPitch());

            // 广播声音给范围内的玩家（16格范围）
            broadcastSoundInRange(
                soundType.getBreakSound(),
                sound::SoundCategory::Blocks,
                position,
                16.0f * soundType.getVolume(),  // 距离 = 16 * volume
                soundType.getVolume(),
                soundType.getPitch()
            );

            // 发送方块更新给所有追踪该区块的玩家
            if (m_chunkSendManager) {
                // 广播方块更新给所有玩家（他们会收到区块更新）
                // 注意：方块更新已经通过 m_world.setBlock 触发
            }
        });

    // 设置方块放置回调 - 播放放置声音
    m_blockInteractionManager->setOnBlockPlace(
        [this](PlayerId playerId, const BlockPos& pos, const BlockState& state) {
            // 获取方块的放置声音
            const auto& soundType = state.getSoundType();
            Vector3 position(static_cast<f32>(pos.x) + 0.5f,
                           static_cast<f32>(pos.y) + 0.5f,
                           static_cast<f32>(pos.z) + 0.5f);

            // 广播声音给范围内的玩家（16格范围）
            broadcastSoundInRange(
                soundType.getPlaceSound(),
                sound::SoundCategory::Blocks,
                position,
                16.0f * soundType.getVolume(),
                soundType.getVolume(),
                soundType.getPitch()
            );
        });
}

void MinecraftServer::shutdownManagers()
{
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
    m_world.reset();
    m_gameModeManager.reset();
    m_packetHandler.reset();
    m_positionTracker.reset();
    m_keepAliveManager.reset();
    m_teleportManager.reset();
    m_timeManager.reset();
    m_connectionManager.reset();
    m_playerManager.reset();
}

void MinecraftServer::tickCore()
{
    MC_TRACE_EVENT("server.tick", "CoreTick", "phase", "core_tick");

    // 更新时间
    {
        MC_TRACE_EVENT("server.tick", "TickTime", "phase", "time");
        m_timeManager->tick();
    }

    // 更新天气
    if (m_world && m_world->weatherManager()) {
        MC_TRACE_EVENT("server.tick", "TickWeather", "phase", "weather");
        m_world->weatherManager()->tick();
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

    // 检查心跳超时
    if (m_tickCounter % KEEPALIVE_INTERVAL == 0) {
        MC_TRACE_EVENT("server.network", "CheckKeepAliveTimeout", "phase", "keepalive_timeout");
        u64 currentTickMs = currentTick() * 50;  // 50ms per tick
        auto timedOutPlayers = m_keepAliveManager->getTimedOutPlayers(currentTickMs);
        for (PlayerId playerId : timedOutPlayers) {
            spdlog::info("MinecraftServer: Player {} timed out", playerId);
            // 清理玩家相关的追踪和票据
            m_chunkSendManager->removePlayer(playerId);
            if (m_world && m_world->chunkManager()) {
                m_world->chunkManager()->removePlayer(playerId);
            }
            m_connectionManager->disconnectPlayer(playerId, "Connection timed out");
        }
    }

    ++m_tickCounter;
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

void MinecraftServer::tickLighting()
{
    if (!m_world) return;

    auto* lightMgr = m_world->lightManager();
    if (!lightMgr) {
        MC_TRACE_INSTANT("server.lighting", "tickLighting", "Status", "NoLightManager");
        return;
    }

    bool hasWork = lightMgr->hasLightWork();
    if (!hasWork) {
        return;
    }

    MC_TRACE_EVENT("server.lighting", "tickLighting", "hasWork", hasWork);
    lightMgr->tick(32768, true, true);
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
    network::TimeUpdatePacket packet(
        time.gameTime(),
        time.dayTime(),
        time.daylightCycleEnabled()
    );

    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::TimeUpdate, ser.buffer());
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
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
            broadcastPacket(fullPacket.data(), fullPacket.size());
        }
        m_lastSentRainStrength = rainStrength;
    }

    if (thunderChanged) {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
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
                auto fullPacket = core::ConnectionManager::encapsulatePacket(
                    network::PacketType::GameStateChange, result.value());
                broadcastPacket(fullPacket.data(), fullPacket.size());
            }
        } else if (weatherType == weather::WeatherType::Rain ||
                   weatherType == weather::WeatherType::Thunder) {
            auto packet = network::GameStateChangePacket::beginRain();
            auto result = packet.serialize();
            if (result.success()) {
                auto fullPacket = core::ConnectionManager::encapsulatePacket(
                    network::PacketType::GameStateChange, result.value());
                broadcastPacket(fullPacket.data(), fullPacket.size());
            }
        }
    }
}

void MinecraftServer::sendInitialWeatherStateToPlayer(PlayerId playerId)
{
    if (!m_world || !m_world->weatherManager()) return;

    auto& weatherMgr = *m_world->weatherManager();
    f32 rainStrength = weatherMgr.rainStrength();
    f32 thunderStrength = weatherMgr.thunderStrength();

    {
        auto packet = network::GameStateChangePacket::rainStrength(rainStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }

    {
        auto packet = network::GameStateChangePacket::thunderStrength(thunderStrength);
        auto result = packet.serialize();
        if (result.success()) {
            auto fullPacket = core::ConnectionManager::encapsulatePacket(
                network::PacketType::GameStateChange, result.value());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }
}

void MinecraftServer::sendKeepAliveToAll()
{
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

void MinecraftServer::setWorld(std::unique_ptr<ServerWorld> world)
{
    m_world = std::move(world);
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
        spdlog::debug("Failed to parse player move from player {}", playerId);
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

    m_positionTracker->updatePosition(playerId, player->x, player->y, player->z, player->yaw, player->pitch, player->onGround);

    // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
    // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
    if (m_world && m_world->chunkManager()) {
        m_world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
        m_world->chunkManager()->processTicketUpdates();
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
                            const entity::effect::EffectInstance* effect = player->getEffect(entity::effect::EffectType::BadOmen);
                            if (effect != nullptr) {
                                i32 level = effect->getEffectLevel();
                                // 移除不祥之兆效果
                                player->removeEffect(entity::effect::EffectType::BadOmen);
                                return level;
                            }
                        }
                        return 0;
                    },
                    enteredVillage
                );
            }
        }
    }
}

void MinecraftServer::handleTeleportConfirmPacket(PlayerId playerId, const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);
    auto result = network::TeleportConfirmPacket::deserialize(deser);

    if (result.failed()) {
        spdlog::debug("Failed to parse teleport confirm from player {}", playerId);
        return;
    }

    auto& packet = result.value();

    if (m_teleportManager->confirmTeleport(playerId, packet.teleportId())) {
        auto* player = m_playerManager->getPlayer(playerId);
        if (!player) {
            return;
        }

        // 更新区块管理器的玩家位置（触发区块加载票据和追踪变化）
        // 区块发送由 ChunkLoadTicketManager 的追踪变化回调自动处理
        if (m_world && m_world->chunkManager()) {
            m_world->chunkManager()->updatePlayerPosition(playerId, player->x, player->z);
            m_world->chunkManager()->processTicketUpdates();
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
        return;
    }

    auto& packet = result.value();
    String message = packet.message();

    if (!message.empty() && message[0] == '/') {
        // 执行命令
        mc::command::ServerCommandSource source(this, nullptr, m_world.get(),
                                               Vector3d(player->x, player->y, player->z),
                                               Vector2f(player->yaw, player->pitch),
                                               4, playerId, player->username);
        auto cmdResult = m_commandRegistry->execute(message, source);
        if (cmdResult.failed()) {
            spdlog::warn("Command '{}' failed for {}: {}",
                         message, player->username, cmdResult.error().toString());
        } else {
            spdlog::info("Command '{}' executed for {} with result {}",
                         message, player->username, cmdResult.value());
        }
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, message);
}

void MinecraftServer::handleBlockInteractionPacket(PlayerId playerId, const u8* data, size_t size)
{
    network::PacketDeserializer deser(data, size);
    auto result = network::BlockInteractionPacket::deserialize(deser);
    if (result.failed()) {
        spdlog::debug("Failed to parse block interaction packet: {}", result.error().message());
        return;
    }

    const auto& packet = result.value();
    BlockPos pos(packet.x(), packet.y(), packet.z());

    // 处理挖掘状态
    miningManager().handleBlockInteraction(playerId, pos, packet.action());

    // 处理方块破坏
    if (packet.action() == network::BlockInteractionAction::StopDestroyBlock) {
        MC_TRACE_INSTANT("server.lighting",
            "HandleBlockBreak",
            "pos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
            "playerId", playerId,
            [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });

        auto interactionResult = blockInteractionManager().handleBlockBreak(playerId, pos);
        if (interactionResult.success() && interactionResult.value().blockBroken) {
            // 发送方块更新给该玩家
            network::BlockUpdatePacket updatePacket(pos.x, pos.y, pos.z, interactionResult.value().newBlockStateId);
            network::PacketSerializer ser;
            updatePacket.serialize(ser);
            auto fullPacket = core::ConnectionManager::encapsulatePacket(network::PacketType::BlockUpdate, ser.buffer());
            sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
        }
    }
}

// ============================================================================
// 数据包发送辅助方法
// ============================================================================

void MinecraftServer::sendTeleportPacket(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId)
{
    network::TeleportPacket packet(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::Teleport, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::sendBlockUpdatePacket(PlayerId playerId, i32 x, i32 y, i32 z, u32 blockStateId)
{
    network::BlockUpdatePacket packet(x, y, z, blockStateId);
    network::PacketSerializer ser;
    packet.serialize(ser);

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::BlockUpdate, ser.buffer());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

void MinecraftServer::setupInitialPlayerState(ServerPlayerData* player, GameMode gameMode)
{
    if (!player) return;

    // 设置初始位置
    player->x = 0.0f;
    player->y = 90.0f;
    player->z = 0.0f;

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
    network::TimeUpdatePacket timePacket(
        time.gameTime(),
        time.dayTime(),
        time.daylightCycleEnabled()
    );
    network::PacketSerializer timeSer;
    timePacket.serialize(timeSer);
    auto fullTimePacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::TimeUpdate, timeSer.buffer());
    sendPacketToPlayer(playerId, fullTimePacket.data(), fullTimePacket.size());

    // 发送初始天气状态
    sendInitialWeatherStateToPlayer(playerId);
}

void MinecraftServer::stopCore()
{
    spdlog::info("Stopping server core...");

    // 停止 Worker 线程
    if (m_world && m_world->chunkManager()) {
        m_world->chunkManager()->stopWorkers();
    }

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

    MC_TRACE_EVENT("server.network", "DispatchPacket",
                   "sessionId", sessionId,
                   "packetType", static_cast<int>(packetType),
                   "payloadSize", payloadSize);

    switch (packetType) {
        case network::PacketType::LoginRequest:
            handleLoginRequestPacket(sessionId, payload, payloadSize);
            break;

        case network::PacketType::PlayerMove: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handlePlayerMovePacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::BlockInteraction: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleBlockInteractionPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::PlayerTryUseItemOnBlock: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleBlockPlacementPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::HotbarSelect: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleHotbarSelectPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::ContainerClick: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleContainerClickPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::CloseContainer: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleCloseContainerPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::TeleportConfirm: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleTeleportConfirmPacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::KeepAlive: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleKeepAlivePacket(playerId, payload, payloadSize);
            }
            break;
        }

        case network::PacketType::ChatMessage: {
            PlayerId playerId = getPlayerIdForSession(sessionId);
            if (playerId != 0) {
                handleChatMessagePacket(playerId, payload, payloadSize);
            }
            break;
        }

        default:
            spdlog::debug("Unhandled packet type: {}", static_cast<int>(packetType));
            break;
    }
}

// ============================================================================
// 声音广播方法
// ============================================================================

void MinecraftServer::broadcastSound(const ResourceLocation& soundEventId,
                                    sound::SoundCategory category,
                                    const Vector3& position,
                                    f32 volume,
                                    f32 pitch) {
    spdlog::debug("[Sound] Broadcasting sound: {} at ({}, {}, {})",
                  soundEventId.toString(), position.x, position.y, position.z);

    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::warn("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::PlaySound, result.value());
    broadcastPacket(fullPacket.data(), fullPacket.size());
}

void MinecraftServer::broadcastSoundInRange(const ResourceLocation& soundEventId,
                                            sound::SoundCategory category,
                                            const Vector3& position,
                                            f32 range,
                                            f32 volume,
                                            f32 pitch) {
    spdlog::debug("[Sound] Broadcasting sound in range: {} at ({}, {}, {}) range={}",
                  soundEventId.toString(), position.x, position.y, position.z, range);

    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::warn("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::PlaySound, result.value());

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

    spdlog::debug("[Sound] Sound {} sent to {} players", soundEventId.toString(), playersNotified);
}

void MinecraftServer::sendSoundToPlayer(PlayerId playerId,
                                        const ResourceLocation& soundEventId,
                                        sound::SoundCategory category,
                                        const Vector3& position,
                                        f32 volume,
                                        f32 pitch) {
    spdlog::debug("[Sound] Sending sound {} to player {}", soundEventId.toString(), playerId);

    glm::vec3 pos(position.x, position.y, position.z);
    sound::PlaySoundPacket packet(soundEventId, category, pos, volume, pitch);

    auto result = packet.serialize();
    if (result.failed()) {
        spdlog::warn("Failed to serialize PlaySoundPacket: {}", result.error().message());
        return;
    }

    auto fullPacket = core::ConnectionManager::encapsulatePacket(
        network::PacketType::PlaySound, result.value());
    sendPacketToPlayer(playerId, fullPacket.data(), fullPacket.size());
}

} // namespace mc::server