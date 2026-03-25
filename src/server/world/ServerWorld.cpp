#include "ServerWorld.hpp"
#include "ServerChunkManager.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/entity/Entity.hpp"
#include "common/entity/EntityRegistry.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/util/NibbleArray.hpp"
#include <chrono>
#include <spdlog/spdlog.h>
#include <cmath>
#include "common/util/assert/AssertAll.hpp"

namespace mc::server {

// 使用 mc 命名空间中的类
using mc::WorldLightManager;
using mc::IChunk;
using mc::IChunkLightProvider;
using mc::LightType;
using mc::SectionPos;
using mc::ChunkPos;
using mc::ChunkSection;
using mc::NibbleArray;

// ============================================================================
// ServerWorld 实现
// ============================================================================

ServerWorld::ServerWorld()
    : m_chunkSyncManager()
{
    // 默认创建区块管理器，允许在未显式 initialize() 时也可同步生成区块
    auto generator = std::make_unique<NoiseChunkGenerator>(
        m_config.seed,
        DimensionSettings::overworld()
    );
    m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
}

ServerWorld::ServerWorld(const ServerWorldConfig& config)
    : m_config(config)
    , m_chunkSyncManager()
{
    m_chunkSyncManager.setDefaultViewDistance(config.viewDistance);

    // 默认创建区块管理器，允许在未显式 initialize() 时也可同步生成区块
    auto generator = std::make_unique<NoiseChunkGenerator>(
        m_config.seed,
        DimensionSettings::overworld()
    );
    m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
}

ServerWorld::~ServerWorld() {
    shutdown();
}

Result<void> ServerWorld::initialize() {
    spdlog::info("Initializing server world with seed {}...", m_config.seed);

    if (m_initialized) {
        return Result<void>::ok();
    }

    // 若尚未创建区块管理器，按当前配置创建
    if (!m_chunkManager) {
        auto generator = std::make_unique<NoiseChunkGenerator>(
            m_config.seed,
            DimensionSettings::overworld()
        );
        m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
    }

    // 设置区块加载回调（用于初始化光照引擎）
    m_chunkManager->setChunkLoadedCallback([this](ChunkCoord x, ChunkCoord z) {
        initializeChunkLighting(x, z);
    });

    auto result = m_chunkManager->initialize();
    if (result.failed()) {
        return result;
    }

    m_chunkSyncManager.setDefaultViewDistance(m_config.viewDistance);

    // 创建碰撞缓存
    m_collisionCache = std::make_unique<physics::CollisionCache>();

    // 创建物理引擎（ServerWorld 实现了 ICollisionWorld）
    m_physicsEngine = std::make_unique<PhysicsEngine>(*this);

    // 创建Tick管理器
    m_tickManager = std::make_unique<world::tick::TickManager>(*this);

    // 创建光照管理器（主世界有天空光照）
    m_lightManager = std::make_unique<WorldLightManager>(this, true, true);

    // 创建天气管理器
    m_weatherManager = std::make_unique<WeatherManager>();
    m_weatherManager->initialize(m_config.seed);
    m_weatherManager->setWorld(this);

    m_initialized = true;

    spdlog::info("Server world initialized with physics engine, tick manager, light manager, and weather system");
    return Result<void>::ok();
}

void ServerWorld::shutdown() {
    spdlog::info("Shutting down server world...");

    m_initialized = false;

    // 清除天气管理器
    m_weatherManager.reset();

    // 清除光照管理器
    m_lightManager.reset();

    // 清除Tick管理器
    m_tickManager.reset();

    // 清除物理引擎和碰撞缓存
    m_physicsEngine.reset();
    m_collisionCache.reset();

    if (m_chunkManager) {
        m_chunkManager->shutdown();
        m_chunkManager.reset();
    }

    std::lock_guard<std::mutex> playerLock(m_playerMutex);
    m_players.clear();

    spdlog::info("Server world shut down");
}

void ServerWorld::setConfig(const ServerWorldConfig& config) {
    m_config = config;
    m_chunkSyncManager.setDefaultViewDistance(config.viewDistance);

    if (m_chunkManager) {
        m_chunkManager->setViewDistance(config.viewDistance);
    }

    std::lock_guard<std::mutex> lock(m_playerMutex);
    for (auto& [playerId, player] : m_players) {
        (void)playerId;
        if (player.chunkTracker) {
            player.chunkTracker->setViewDistance(config.viewDistance);
        }
    }
}

// ============================================================================
// 区块管理
// ============================================================================

ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z) {
    if (m_chunkManager) {
        return m_chunkManager->getChunk(x, z);
    }
    return nullptr;
}

const ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z) const {
    if (m_chunkManager) {
        return m_chunkManager->getChunk(x, z);
    }
    return nullptr;
}

bool ServerWorld::hasChunk(ChunkCoord x, ChunkCoord z) const {
    if (m_chunkManager) {
        return m_chunkManager->hasChunk(x, z);
    }
    return false;
}

ChunkData* ServerWorld::getChunkSync(ChunkCoord x, ChunkCoord z) {
    if (m_chunkManager) {
        return m_chunkManager->getChunkSync(x, z);
    }
    return nullptr;
}

void ServerWorld::unloadChunk(ChunkCoord x, ChunkCoord z) {
    if (m_chunkManager) {
        m_chunkManager->unloadChunk(x, z);
    }
}

// ============================================================================
// 玩家管理
// ============================================================================

void ServerWorld::addPlayer(PlayerId playerId, const String& username, network::ConnectionPtr connection) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto& player = m_players[playerId];
    player.playerId = playerId;
    player.username = username;
    player.connection = connection;
    player.chunkTracker = std::make_shared<network::PlayerChunkTracker>(playerId);
    player.chunkTracker->setViewDistance(m_config.viewDistance);

    // 更新区块同步管理器
    (void)m_chunkSyncManager.getTracker(playerId);
    m_chunkSyncManager.updatePlayerPosition(playerId, player.x, player.z);

    spdlog::info("Player {} ({}) joined the world", username, playerId);
}

void ServerWorld::removePlayer(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    // 通知其他玩家
    network::PlayerDespawnPacket despawnPacket(playerId);
    network::PacketSerializer ser;
    despawnPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::PlayerDespawn));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    for (auto& [pid, pdata] : m_players) {
        if (pid != playerId) {
            pdata.send(fullPacket.data(), fullPacket.size());
        }
    }

    // 从区块管理器移除玩家
    if (m_chunkManager) {
        m_chunkManager->removePlayer(playerId);
    }

    String username = it->second.username;
    m_players.erase(it);
    m_chunkSyncManager.removeTracker(playerId);

    spdlog::info("Player {} ({}) left the world", username, playerId);
}

ServerPlayerData* ServerWorld::getPlayer(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

const ServerPlayerData* ServerWorld::getPlayer(PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    auto it = m_players.find(playerId);
    return it != m_players.end() ? &it->second : nullptr;
}

bool ServerWorld::hasPlayer(PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    return m_players.find(playerId) != m_players.end();
}

size_t ServerWorld::playerCount() const {
    std::lock_guard<std::mutex> lock(m_playerMutex);
    return m_players.size();
}

// ============================================================================
// 位置更新
// ============================================================================

void ServerWorld::updatePlayerPosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround) {
    // 准备区块更新数据（在锁外执行耗时操作）
    std::vector<mc::ChunkPos> chunksToLoad;
    std::vector<mc::ChunkPos> chunksToUnload;
    bool needsChunkUpdate = false;

    {
        std::lock_guard<std::mutex> lock(m_playerMutex);

        auto it = m_players.find(playerId);
        if (it == m_players.end()) return;

        auto& player = it->second;

        // 转换 f64 网络坐标到 f32 内部坐标
        f32 fx = static_cast<f32>(x);
        f32 fy = static_cast<f32>(y);
        f32 fz = static_cast<f32>(z);

        ChunkCoord oldChunkX = blockToChunk(player.x);
        ChunkCoord oldChunkZ = blockToChunk(player.z);
        ChunkCoord newChunkX = blockToChunk(fx);
        ChunkCoord newChunkZ = blockToChunk(fz);

        player.x = fx;
        player.y = fy;
        player.z = fz;
        player.yaw = yaw;
        player.pitch = pitch;
        player.onGround = onGround;

        // 检查是否跨越区块边界
        if (oldChunkX != newChunkX || oldChunkZ != newChunkZ) {
            // 更新区块同步（使用 f64 坐标）
            m_chunkSyncManager.updatePlayerPosition(playerId, x, z);

            // 更新区块管理器中的玩家位置（使用 f64）
            if (m_chunkManager) {
                m_chunkManager->updatePlayerPosition(playerId, x, z);
            }

            // 计算需要发送/卸载的区块
            m_chunkSyncManager.calculateUpdates(playerId, chunksToLoad, chunksToUnload);
            needsChunkUpdate = true;
        }
    }

    // 在锁外发送区块（避免死锁）
    if (needsChunkUpdate && m_initialized) {
        // 发送新区块
        for (const auto& pos : chunksToLoad) {
            sendChunkToPlayer(playerId, pos.x, pos.z);
        }

        // 卸载旧区块
        for (const auto& pos : chunksToUnload) {
            sendUnloadChunkToPlayer(playerId, pos.x, pos.z);
        }
    }

    // 广播玩家移动给其他玩家
    // TODO: 实现玩家移动广播
}

void ServerWorld::confirmTeleport(PlayerId playerId, u32 teleportId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    auto& player = it->second;

    if (player.waitingTeleportConfirm && player.pendingTeleportId == teleportId) {
        player.waitingTeleportConfirm = false;
        spdlog::debug("Player {} confirmed teleport {}", playerId, teleportId);
    }
}

// ============================================================================
// 传送
// ============================================================================

void ServerWorld::teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    auto& player = it->second;

    player.x = static_cast<f32>(x);
    player.y = static_cast<f32>(y);
    player.z = static_cast<f32>(z);
    player.yaw = yaw;
    player.pitch = pitch;

    // 生成新的传送ID
    static u32 nextTeleportId = 1;
    u32 teleportId = nextTeleportId++;

    player.pendingTeleportId = teleportId;
    player.waitingTeleportConfirm = true;

    // 发送传送包
    network::TeleportPacket teleportPacket(x, y, z, yaw, pitch, teleportId);
    network::PacketSerializer ser;
    teleportPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::Teleport));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    player.send(fullPacket.data(), fullPacket.size());

    spdlog::debug("Teleporting player {} to ({}, {}, {}), teleportId={}",
                  playerId, x, y, z, teleportId);
}

bool ServerWorld::setPlayerGameMode(PlayerId playerId, GameMode mode) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) {
        return false;
    }

    it->second.gameMode = mode;
    return true;
}

// ============================================================================
// 区块同步
// ============================================================================

void ServerWorld::sendInitialChunks(PlayerId playerId) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    std::vector<mc::ChunkPos> chunksToLoad;
    std::vector<mc::ChunkPos> chunksToUnload;

    m_chunkSyncManager.calculateUpdates(playerId, chunksToLoad, chunksToUnload);

    // 发送所有需要的区块
    for (const auto& pos : chunksToLoad) {
        sendChunkToPlayer(playerId, pos.x, pos.z);
    }
}

void ServerWorld::sendChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z) {
    // 异步获取或生成区块
    m_chunkManager->getChunkAsync(x, z,
        [this, playerId, x, z](bool success, ChunkData* chunk) {
            if (!success || !chunk) {
                spdlog::warn("Failed to generate chunk ({}, {}) for player {}", x, z, playerId);
                return;
            }

            // 序列化区块
            auto result = network::ChunkSerializer::serializeChunk(*chunk);
            if (result.failed()) {
                spdlog::error("Failed to serialize chunk ({}, {}): {}", x, z, result.error().message());
                return;
            }

            // 标记为已发送
            m_chunkSyncManager.markChunkSent(playerId, x, z);

            // 发送区块数据包
            network::ChunkDataPacket chunkPacket(x, z, std::move(result.value()));
            network::PacketSerializer ser;
            chunkPacket.serialize(ser);

            network::PacketSerializer fullPacket;
            fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
            fullPacket.writeU16(static_cast<u16>(network::PacketType::ChunkData));
            fullPacket.writeU16(0);
            fullPacket.writeU16(0);
            fullPacket.writeU16(0);
            fullPacket.writeBytes(ser.buffer());

            std::lock_guard<std::mutex> lock(m_playerMutex);
            auto it = m_players.find(playerId);
            if (it != m_players.end()) {
                it->second.send(fullPacket.data(), fullPacket.size());
            }

            spdlog::debug("Sent chunk ({}, {}) to player {}", x, z, playerId);
        });
}

void ServerWorld::sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z) {
    m_chunkSyncManager.markChunkUnloaded(playerId, x, z);

    network::UnloadChunkPacket unloadPacket(x, z);
    network::PacketSerializer ser;
    unloadPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::UnloadChunk));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    std::lock_guard<std::mutex> lock(m_playerMutex);
    auto it = m_players.find(playerId);
    if (it != m_players.end()) {
        it->second.send(fullPacket.data(), fullPacket.size());
    }
}

// ============================================================================
// 方块操作
// ============================================================================

void ServerWorld::broadcastBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId) {
    network::BlockUpdatePacket blockPacket(x, y, z, blockStateId);
    network::PacketSerializer ser;
    blockPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::BlockUpdate));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    broadcastPacket(fullPacket.buffer());
}

// ============================================================================
// 发送数据包
// ============================================================================

void ServerWorld::sendPacket(PlayerId playerId, const std::vector<u8>& data) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    auto it = m_players.find(playerId);
    if (it == m_players.end()) return;

    it->second.send(data.data(), data.size());
}

void ServerWorld::broadcastPacket(const std::vector<u8>& data) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    for (const auto& [playerId, player] : m_players) {
        player.send(data.data(), data.size());
    }
}

void ServerWorld::broadcastPacketExcept(PlayerId excludePlayerId, const std::vector<u8>& data) {
    std::lock_guard<std::mutex> lock(m_playerMutex);

    for (const auto& [playerId, player] : m_players) {
        if (playerId == excludePlayerId) continue;
        player.send(data.data(), data.size());
    }
}

// ============================================================================
// 更新循环
// ============================================================================

void ServerWorld::tick() {
    m_currentTick++;

    // 更新游戏时间
    m_gameTime.tick();

    // 每 20 ticks（1秒）输出玩家所在位置的亮度信息
    if (m_currentTick % 20 == 0) {
        spdlog::info("[LightTick] Tick {}: Checking player light levels...", m_currentTick);

        std::lock_guard<std::mutex> lock(m_playerMutex);
        for (const auto& [playerId, playerData] : m_players) {
            if (!playerData.loggedIn) continue;

            // 获取玩家脚部方块坐标
            i32 blockX = static_cast<i32>(std::floor(playerData.x));
            i32 blockY = static_cast<i32>(std::floor(playerData.y));
            i32 blockZ = static_cast<i32>(std::floor(playerData.z));

            // 获取亮度等级
            u8 blockLight = getBlockLight(blockX, blockY, blockZ);
            u8 skyLight = getSkyLight(blockX, blockY, blockZ);
            u8 combinedLight = std::max(blockLight, skyLight);

            spdlog::info("[Light] 玩家 {} 位置 ({}, {}, {}) - 方块亮度: {}, 天空亮度: {}, 综合亮度: {}",
                playerData.username, blockX, blockY, blockZ, blockLight, skyLight, combinedLight);
        }
    }

    // 更新天气
    if (m_weatherManager) {
        m_weatherManager->tick();
    }

    // 执行计划刻（方块tick和流体tick）
    if (m_tickManager) {
        m_tickManager->tick(m_currentTick);
    }

    // 更新光照（处理光照传播队列）
    if (m_lightManager && m_lightManager->hasLightWork()) {
        // 每tick处理最多一定数量的光照更新
        m_lightManager->tick(32768, true, true);
    }

    // 更新所有实体
    size_t entityCount = m_entityManager.entityCount();
    if (m_currentTick % 600 == 0 && entityCount > 0) {
        spdlog::debug("ServerWorld::tick() tick={}, entities={}", m_currentTick, entityCount);
    }
    m_entityManager.tick();

    // 处理物品拾取
    m_itemPickupManager.tick(*this);

    // 更新实体追踪（发送位置更新给已追踪的玩家）
    m_entityTracker.tick(*this);

    // 更新玩家对实体的追踪状态
    {
        std::lock_guard<std::mutex> lock(m_playerMutex);
        for (const auto& [playerId, player] : m_players) {
            Vector3 playerPos(player.x, player.y, player.z);
            m_entityTracker.updatePlayerTracking(*this, playerId, playerPos);
        }
    }

    // 更新区块管理器
    if (m_chunkManager) {
        m_chunkManager->tick();
    }

    // 每 20 ticks 同步一次时间到客户端
    if (m_currentTick - m_lastTimeSyncTick >= time::TimeConstants::TIME_SYNC_INTERVAL) {
        broadcastTimeUpdate();
        m_lastTimeSyncTick = m_currentTick;
    }

    // 区块卸载现在由 ServerChunkManager 处理
}

void ServerWorld::initializeChunkLighting(ChunkCoord chunkX, ChunkCoord chunkZ) {
    if (!m_lightManager) {
        return;
    }

    // 获取区块数据
    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return;
    }

    ChunkPos chunkPos(chunkX, chunkZ);

    // 更新 BlockLightEngine 的空区块段映射（用于优化传播）
    auto* blockLightEngine = m_lightManager->getBlockLightEngine();
    if (blockLightEngine != nullptr) {
        blockLightEngine->updateEmptinessMap(chunkX, chunkZ, chunk);
    }

    // 为每个区块段初始化光照数据
    for (i32 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk->getSection(sectionY);
        SectionPos sectionPos(chunkX, sectionY, chunkZ);

        // 告诉光照引擎该区块段的状态（是否为空）
        bool isEmpty = (section == nullptr || section->isEmpty());
        m_lightManager->updateSectionStatus(sectionPos, isEmpty);

        // 如果区块段有数据，将光照数据同步到光照引擎
        if (section != nullptr) {
            // 同步天空光照
            if (m_lightManager->getSkyLightEngine()) {
                NibbleArray skyLightCopy = section->skyLightNibble().copy();
                m_lightManager->setData(LightType::SKY, sectionPos, skyLightCopy, false);
            }

            // 同步方块光照
            if (m_lightManager->getBlockLightEngine()) {
                NibbleArray blockLightCopy = section->blockLightNibble().copy();
                m_lightManager->setData(LightType::BLOCK, sectionPos, blockLightCopy, false);
            }
        }
    }

    // 启用该区块列的光源
    m_lightManager->enableLightSources(chunkPos, true);
}

size_t ServerWorld::chunkCount() const {
    if (m_chunkManager) {
        return m_chunkManager->singleChunkLifecycleManagerCount();
    }
    return 0;
}

size_t ServerWorld::loadedChunkCount() const {
    if (m_chunkManager) {
        return m_chunkManager->loadedChunkCount();
    }
    return 0;
}

// ============================================================================
// 时间管理
// ============================================================================

void ServerWorld::setDayTime(i64 time) {
    m_gameTime.setDayTime(time);
    // 立即广播时间变更
    broadcastTimeUpdate();
}

void ServerWorld::addDayTime(i64 ticks) {
    m_gameTime.addDayTime(ticks);
    // 立即广播时间变更
    broadcastTimeUpdate();
}

void ServerWorld::setDaylightCycleEnabled(bool enabled) {
    m_gameTime.setDaylightCycleEnabled(enabled);
    // 立即广播
    broadcastTimeUpdate();
}

void ServerWorld::broadcastTimeUpdate() {
    network::TimeUpdatePacket timePacket(
        m_gameTime.gameTime(),
        m_gameTime.dayTime(),
        m_gameTime.daylightCycleEnabled()
    );

    network::PacketSerializer ser;
    timePacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::TimeUpdate));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    broadcastPacket(fullPacket.buffer());
}

// ============================================================================
// IWorld 接口实现
// ============================================================================

const BlockState* ServerWorld::getBlockState(i32 x, i32 y, i32 z) const {
    // 调用已有的 const 版本方法
    ChunkCoord chunkX = blockToChunk(static_cast<f32>(x));
    ChunkCoord chunkZ = blockToChunk(static_cast<f32>(z));

    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) return nullptr;

    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    return chunk->getBlock(localX, y, localZ);
}

const fluid::FluidState* ServerWorld::getFluidState(i32 x, i32 y, i32 z) const {
    // 获取方块状态，然后从中获取流体状态
    const BlockState* blockState = getBlockState(x, y, z);
    if (blockState == nullptr) {
        // 返回空流体
        return fluid::Fluid::getFluidState(0);
    }
    return blockState->getFluidState();
}

bool ServerWorld::isWithinWorldBounds(i32 /*x*/, i32 y, i32 /*z*/) const {
    // MC 世界高度限制：-64 到 320 (1.18+), 或 0 到 256 (旧版本)
    // 暂时使用 0-256 范围
    return y >= 0 && y < 256;
}

bool ServerWorld::setBlock(i32 x, i32 y, i32 z, const BlockState* state) {
    ChunkCoord chunkX = blockToChunk(static_cast<f32>(x));
    ChunkCoord chunkZ = blockToChunk(static_cast<f32>(z));

    ChunkData* chunk = getChunkSync(chunkX, chunkZ);
    if (!chunk) return false;

    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    // 获取旧方块状态（用于光照更新）
    const BlockState* oldState = chunk->getBlock(localX, y, localZ);
    i32 oldLightLevel = oldState ? oldState->lightLevel() : 0;
    i32 newLightLevel = state ? state->lightLevel() : 0;

    // 设置新方块
    chunk->setBlock(localX, y, localZ, state);
    chunk->setDirty(true);

    // 更新光照
    if (m_lightManager) {
        BlockPos pos(x, y, z);

        // 检查位置的光照变化
        m_lightManager->checkBlock(pos);

        // 如果光源等级增加，通知光照引擎
        if (newLightLevel > oldLightLevel) {
            m_lightManager->onBlockEmissionIncrease(pos, newLightLevel);
        }
    }

    // 广播方块更新
    broadcastBlockUpdate(x, y, z, state ? state->stateId() : 0);
    return true;
}

i32 ServerWorld::getHeight(i32 /*x*/, i32 /*z*/) const {
    // TODO: 实现高度图查询
    return 64;
}

u8 ServerWorld::getBlockLight(i32 x, i32 y, i32 z) const {
    if (m_lightManager) {
        return m_lightManager->getBlockLight(BlockPos(x, y, z));
    }
    return 0;  // 无光
}

u8 ServerWorld::getSkyLight(i32 x, i32 y, i32 z) const {
    if (m_lightManager) {
        return m_lightManager->getSkyLight(BlockPos(x, y, z));
    }
    return 15;  // 默认全亮
}

// ============================================================================
// 碰撞检测
// ============================================================================

bool ServerWorld::hasBlockCollision(const AxisAlignedBB& box) const {
    // 计算 AABB 覆盖的区块范围
    ChunkCoord minChunkX = blockToChunk(box.minX);
    ChunkCoord maxChunkX = blockToChunk(box.maxX);
    ChunkCoord minChunkZ = blockToChunk(box.minZ);
    ChunkCoord maxChunkZ = blockToChunk(box.maxZ);

    // 遍历所有覆盖的区块
    for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
            const ChunkData* chunk = getChunk(cx, cz);
            if (!chunk) continue;

            // 计算区块内的 Y 范围
            i32 minY = std::max(0, static_cast<i32>(std::floor(box.minY)));
            i32 maxY = std::min(255, static_cast<i32>(std::ceil(box.maxY)));

            // 遍历区块内的方块
            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = 0; z < 16; ++z) {
                    for (i32 x = 0; x < 16; ++x) {
                        // 计算世界坐标
                        i32 wx = cx * 16 + x;
                        i32 wz = cz * 16 + z;

                        // 检查是否在 AABB 范围内
                        if (wx + 1 < box.minX || wx > box.maxX ||
                            wz + 1 < box.minZ || wz > box.maxZ) {
                            continue;
                        }

                        const BlockState* state = chunk->getBlock(x, y, z);
                        if (!state || state->isAir()) continue;

                        const CollisionShape& shape = state->getCollisionShape();
                        if (shape.isEmpty()) continue;

                        // 检测碰撞
                        if (shape.intersects(box, wx, y, wz)) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

std::vector<AxisAlignedBB> ServerWorld::getBlockCollisions(const AxisAlignedBB& box) const {
    std::vector<AxisAlignedBB> collisions;

    // 计算 AABB 覆盖的区块范围
    ChunkCoord minChunkX = blockToChunk(box.minX);
    ChunkCoord maxChunkX = blockToChunk(box.maxX);
    ChunkCoord minChunkZ = blockToChunk(box.minZ);
    ChunkCoord maxChunkZ = blockToChunk(box.maxZ);

    // 遍历所有覆盖的区块
    for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
            const ChunkData* chunk = getChunk(cx, cz);
            if (!chunk) continue;

            // 计算区块内的 Y 范围
            i32 minY = std::max(0, static_cast<i32>(std::floor(box.minY)));
            i32 maxY = std::min(255, static_cast<i32>(std::ceil(box.maxY)));

            // 遍历区块内的方块
            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = 0; z < 16; ++z) {
                    for (i32 x = 0; x < 16; ++x) {
                        // 计算世界坐标
                        i32 wx = cx * 16 + x;
                        i32 wz = cz * 16 + z;

                        // 检查是否在 AABB 范围内
                        if (wx + 1 < box.minX || wx > box.maxX ||
                            wz + 1 < box.minZ || wz > box.maxZ) {
                            continue;
                        }

                        const BlockState* state = chunk->getBlock(x, y, z);
                        if (!state || state->isAir()) continue;

                        const CollisionShape& shape = state->getCollisionShape();
                        if (shape.isEmpty()) continue;

                        // 获取碰撞箱
                        auto worldBoxes = shape.getWorldBoxes(wx, y, wz);
                        for (const auto& worldBox : worldBoxes) {
                            if (box.intersects(worldBox)) {
                                collisions.push_back(worldBox);
                            }
                        }
                    }
                }
            }
        }
    }

    return collisions;
}

bool ServerWorld::hasEntityCollision(const AxisAlignedBB& box, const Entity* except) const {
    auto entities = m_entityManager.getEntitiesInAABB(box, except);
    return !entities.empty();
}

std::vector<AxisAlignedBB> ServerWorld::getEntityCollisions(
    const AxisAlignedBB& box, const Entity* except) const {

    std::vector<AxisAlignedBB> collisions;

    auto entities = m_entityManager.getEntitiesInAABB(box, except);
    collisions.reserve(entities.size());

    for (const Entity* entity : entities) {
        collisions.push_back(entity->boundingBox());
    }

    return collisions;
}

std::vector<Entity*> ServerWorld::getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const {
    return m_entityManager.getEntitiesInAABB(box, except);
}

std::vector<Entity*> ServerWorld::getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const {
    return m_entityManager.getEntitiesInRange(pos, range, except);
}

// ============================================================================
// 碰撞缓存
// ============================================================================

void ServerWorld::invalidateCollisionCache(ChunkCoord chunkX, ChunkCoord chunkZ) {
    if (m_collisionCache) {
        m_collisionCache->invalidateChunkAndNeighbors(chunkX, chunkZ);
    }
}

void ServerWorld::clearCollisionCache() {
    if (m_collisionCache) {
        m_collisionCache->clear();
    }
}

// ============================================================================
// 实体管理
// ============================================================================

EntityId ServerWorld::spawnEntity(std::unique_ptr<Entity> entity) {
    if (!entity) {
        return 0;
    }

    // 设置实体的世界引用
    entity->setWorld(this);

    // 添加到实体管理器
    EntityId id = m_entityManager.addEntity(std::move(entity));

    // 注册到实体追踪器，以便同步给客户端
    Entity* addedEntity = m_entityManager.getEntity(id);
    if (addedEntity) {
        m_entityTracker.trackEntity(addedEntity);
    }

    spdlog::debug("Spawned entity with ID {}", id);
    return id;
}

std::unique_ptr<Entity> ServerWorld::removeEntity(EntityId id) {
    auto entity = m_entityManager.removeEntity(id);
    if (entity) {
        spdlog::debug("Removed entity with ID {}", id);
    }
    return entity;
}

Entity* ServerWorld::getEntity(EntityId id) {
    return m_entityManager.getEntity(id);
}

const Entity* ServerWorld::getEntity(EntityId id) const {
    return m_entityManager.getEntity(id);
}

bool ServerWorld::hasEntity(EntityId id) const {
    return m_entityManager.hasEntity(id);
}

size_t ServerWorld::entityCount() const {
    return m_entityManager.entityCount();
}

// ============================================================================
// 区块生成实体
// ============================================================================

i32 ServerWorld::spawnEntitiesFromChunkGeneration(const std::vector<SpawnedEntityData>& entities) {
    if (entities.empty()) {
        return 0;
    }

    i32 spawnedCount = 0;
    auto& registry = entity::EntityRegistry::instance();

    for (const auto& entityData : entities) {
        // 获取实体类型
        const entity::EntityType* entityType = registry.getType(entityData.entityTypeId);
        if (!entityType) {
            spdlog::debug("ServerWorld: Unknown entity type '{}' during chunk generation spawn",
                          entityData.entityTypeId);
            continue;
        }

        // 检查实体类型是否可以生成
        if (!entityType->canSummon()) {
            spdlog::trace("ServerWorld: Entity type '{}' cannot be summoned",
                          entityData.entityTypeId);
            continue;
        }

        // 创建实体实例
        std::unique_ptr<Entity> entity = entityType->create(this);
        if (!entity) {
            spdlog::debug("ServerWorld: Failed to create entity of type '{}'",
                          entityData.entityTypeId);
            continue;
        }

        // 设置实体的世界引用（重要：用于物理引擎和区块访问）
        entity->setWorld(this);

        // 设置实体位置
        entity->setPosition(Vector3(entityData.x, entityData.y, entityData.z));

        // 添加到实体管理器
        EntityId entityId = m_entityManager.addEntity(std::move(entity));
        if (entityId != 0) {
            // 注册到实体追踪器，以便同步给客户端
            Entity* addedEntity = m_entityManager.getEntity(entityId);
            if (addedEntity) {
                m_entityTracker.trackEntity(addedEntity);
            }

            ++spawnedCount;
        }
    }

    if (spawnedCount > 0) {
        spdlog::debug("ServerWorld: Spawned {} entities from chunk generation", spawnedCount);
    }

    return spawnedCount;
}

// ============================================================================
// Tick调度便捷方法
// ============================================================================

void ServerWorld::scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay,
                                     world::tick::TickPriority priority) {
    if (m_tickManager) {
        m_tickManager->scheduleBlockTick(pos, block, delay, priority);
    }
}

void ServerWorld::scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay,
                                     world::tick::TickPriority priority) {
    if (m_tickManager) {
        m_tickManager->scheduleFluidTick(pos, fluid, delay, priority);
    }
}

// ============================================================================
// IChunkLightProvider 接口实现
// ============================================================================

IChunk* ServerWorld::getChunkForLight(ChunkCoord x, ChunkCoord z) {
    return getChunk(x, z);
}

const IChunk* ServerWorld::getChunkForLight(ChunkCoord x, ChunkCoord z) const {
    return getChunk(x, z);
}

const BlockState* ServerWorld::getBlockStateForLight(const BlockPos& pos) const {
    return getBlockState(pos.x, pos.y, pos.z);
}

IWorld* ServerWorld::getWorld() {
    return this;
}

const IWorld* ServerWorld::getWorld() const {
    return this;
}

void ServerWorld::markLightChanged(LightType type, const SectionPos& pos) {
    // 标记区块为脏，以便保存和同步
    ChunkCoord chunkX = pos.x;
    ChunkCoord chunkZ = pos.z;
    ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (chunk) {
        chunk->setDirty(true);
    }

    // 从光照引擎同步光照数据到 ChunkSection
    // 这是关键步骤：光照引擎计算的数据存储在其内部 storage 中，
    // 需要将数据同步到 ChunkSection 才能正确广播给客户端
    syncLightDataToChunk(type, pos);

    // 发送光照更新包给客户端
    broadcastLightUpdate(type, pos);
}

void ServerWorld::broadcastLightUpdate(LightType type, const SectionPos& pos) {
    // 获取区块数据
    ChunkCoord chunkX = pos.x;
    ChunkCoord chunkZ = pos.z;
    ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return;
    }

    // 获取该区块段的索引
    i32 sectionY = pos.y;
    const ChunkSection* section = chunk->getSection(sectionY);
    if (!section) {
        return;
    }

    // 获取订阅该区块的玩家
    std::vector<PlayerId> subscribers;
    m_chunkSyncManager.getChunkSubscribers(chunkX, chunkZ, subscribers);
    if (subscribers.empty()) {
        return;
    }

    // 准备光照数据
    std::vector<u8> skyLightData;
    std::vector<u8> blockLightData;

    if (type == LightType::SKY) {
        const auto& skyLight = section->skyLightNibble();
        if (!skyLight.data().empty()) {
            skyLightData = skyLight.data();
        } else {
            // 如果为空，填充默认值15（全亮）
            skyLightData.resize(NibbleArray::BYTE_SIZE, 0xFF);
        }
    }

    if (type == LightType::BLOCK) {
        const auto& blockLight = section->blockLightNibble();
        if (!blockLight.data().empty()) {
            blockLightData = blockLight.data();
        } else {
            // 如果为空，填充默认值0（无光照）
            blockLightData.resize(NibbleArray::BYTE_SIZE, 0x00);
        }
    }

    // 创建光照更新包
    network::LightUpdatePacket lightPacket(
        chunkX, chunkZ, sectionY,
        std::move(skyLightData),
        std::move(blockLightData),
        false  // trustEdges
    );

    // 序列化包
    network::PacketSerializer ser;
    lightPacket.serialize(ser);

    network::PacketSerializer fullPacket;
    fullPacket.writeU32(static_cast<u32>(network::PACKET_HEADER_SIZE + ser.size()));
    fullPacket.writeU16(static_cast<u16>(network::PacketType::LightUpdate));
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeU16(0);
    fullPacket.writeBytes(ser.buffer());

    // 发送给所有订阅该区块的玩家
    std::lock_guard<std::mutex> lock(m_playerMutex);
    for (PlayerId playerId : subscribers) {
        auto it = m_players.find(playerId);
        if (it != m_players.end()) {
            it->second.send(fullPacket.data(), fullPacket.size());
        }
    }

    spdlog::debug("Broadcast light update: chunk({}, {}), section {}, type {}",
                  chunkX, chunkZ, sectionY,
                  type == LightType::SKY ? "sky" : "block");
}

void ServerWorld::syncLightDataToChunk(LightType type, const SectionPos& pos) {
    if (!m_lightManager) {
        return;
    }

    // 获取区块数据
    ChunkData* chunk = getChunk(pos.x, pos.z);
    if (!chunk) {
        return;
    }

    // 获取区块段索引
    i32 sectionIndex = pos.y;
    if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
        return;
    }

    // 获取或创建区块段
    ChunkSection* section = chunk->getSection(sectionIndex);
    if (!section) {
        // 如果没有区块段，需要创建一个
        section = chunk->createSection(sectionIndex);
        if (!section) {
            return;
        }
    }

    // 从光照引擎获取光照数据
    SWMRNibbleArray* lightArray = m_lightManager->getData(type, pos);
    if (!lightArray || lightArray->isNullUpdating()) {
        // 光照引擎没有该段的数据，使用默认值
        if (type == LightType::SKY) {
            // 天空光照默认为 15（全亮）
            section->fillSkyLight(15);
        } else {
            // 方块光照默认为 0（无光）
            section->fillBlockLight(0);
        }
        return;
    }

    // 转换 SWMRNibbleArray 到 NibbleArray 并复制到 ChunkSection
    std::vector<u8> data = lightArray->toByteArray();
    if (type == LightType::SKY) {
        NibbleArray& skyLight = section->skyLightNibble();
        if (data.empty()) {
            section->fillSkyLight(15);
        } else if (skyLight.data().size() == data.size()) {
            std::memcpy(skyLight.data().data(), data.data(), data.size());
        } else {
            // 尺寸不匹配，重新设置
            section->skyLightNibble() = NibbleArray(data);
        }
    } else {
        NibbleArray& blockLight = section->blockLightNibble();
        if (data.empty()) {
            section->fillBlockLight(0);
        } else if (blockLight.data().size() == data.size()) {
            std::memcpy(blockLight.data().data(), data.data(), data.size());
        } else {
            // 尺寸不匹配，重新设置
            section->blockLightNibble() = NibbleArray(data);
        }
    }
}

bool ServerWorld::hasSkyLight() const {
    // 主世界有天空光照，下界和末地没有
    return m_config.dimension == 0;
}

i32 ServerWorld::getMinBuildHeight() const {
    return 0;
}

i32 ServerWorld::getMaxBuildHeight() const {
    return 256;
}

i32 ServerWorld::getSectionCount() const {
    return 16;  // 256 / 16
}

// ============================================================================
// 天气接口 (IWorld)
// ============================================================================

bool ServerWorld::isRaining() const {
    return m_weatherManager ? m_weatherManager->isRaining() : false;
}

bool ServerWorld::isThundering() const {
    return m_weatherManager ? m_weatherManager->isThundering() : false;
}

f32 ServerWorld::rainStrength(f32 partialTick) const {
    return m_weatherManager ? m_weatherManager->rainStrength(partialTick) : 0.0f;
}

f32 ServerWorld::thunderStrength(f32 partialTick) const {
    return m_weatherManager ? m_weatherManager->thunderStrength(partialTick) : 0.0f;
}

bool ServerWorld::canRainAt(const BlockPos& pos) const {
    if (!m_weatherManager || !m_weatherManager->isRaining()) {
        return false;
    }
    return weather::WeatherUtils::canRainAt(*this, pos);
}

} // namespace mc::server
