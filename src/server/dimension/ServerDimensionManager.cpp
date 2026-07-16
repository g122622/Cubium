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

#include "ServerDimensionManager.hpp"
#include "../application/MinecraftServer.hpp"
#include "../sync/ChunkSendManager.hpp"
#include "../world/ServerChunkManager.hpp"
#include "../world/ServerWorld.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Result.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/packet/DimensionPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/crypto/Sha256.hpp"
#include "common/world/biome/source/EndBiomeSource.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ServerDimensionManager::ServerDimensionManager(server::MinecraftServer* server)
    : m_server(server)
{
    MC_ASSERT(server != nullptr);
}

ServerDimensionManager::~ServerDimensionManager() = default;

// ============================================================================
// 初始化
// ============================================================================

Result<void> ServerDimensionManager::initialize(u64 seed, i32 viewDistance, WorldType overworldType)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerDimensionManager::initialize");

    if (m_initialized) {
        return {};
    }

    m_seed = seed;
    m_viewDistance = viewDistance;
    m_overworldType = overworldType;

    m_dimensions.clear();

    // 创建主世界
    auto overworld = _createServerDimension(OVERWORLD, seed);
    MC_ASSERT_RELEASE(overworld != nullptr);
    MC_ASSERT_RELEASE(overworld->id() == OVERWORLD);
    const bool overworldRegistered = registerDimension(std::move(overworld));
    MC_ASSERT_RELEASE_MSG(overworldRegistered, "Failed to register overworld");
    auto* registeredOverworld = static_cast<ServerDimension*>(DimensionManager::getDimension(OVERWORLD));
    MC_ASSERT_RELEASE(registeredOverworld != nullptr);
    auto overworldInitResult = registeredOverworld->initialize();
    MC_ASSERT_RELEASE_MSG(overworldInitResult.success(), "Failed to initialize overworld");

    // 创建下界
    auto nether = _createServerDimension(NETHER, seed);
    MC_ASSERT_RELEASE(nether != nullptr);
    MC_ASSERT_RELEASE(nether->id() == NETHER);
    const bool netherRegistered = registerDimension(std::move(nether));
    MC_ASSERT_RELEASE_MSG(netherRegistered, "Failed to register nether");
    auto* registeredNether = static_cast<ServerDimension*>(DimensionManager::getDimension(NETHER));
    MC_ASSERT_RELEASE(registeredNether != nullptr);
    auto netherInitResult = registeredNether->initialize();
    MC_ASSERT_RELEASE_MSG(netherInitResult.success(), "Failed to initialize nether");

    // 创建末地
    auto theEnd = _createServerDimension(THE_END, seed);
    MC_ASSERT_RELEASE(theEnd != nullptr);
    MC_ASSERT_RELEASE(theEnd->id() == THE_END);
    const bool endRegistered = registerDimension(std::move(theEnd));
    MC_ASSERT_RELEASE_MSG(endRegistered, "Failed to register the end");
    auto* registeredEnd = static_cast<ServerDimension*>(DimensionManager::getDimension(THE_END));
    MC_ASSERT_RELEASE(registeredEnd != nullptr);
    auto endInitResult = registeredEnd->initialize();
    MC_ASSERT_RELEASE_MSG(endInitResult.success(), "Failed to initialize the end");

    m_initialized = true;
    return {};
}

void ServerDimensionManager::shutdown()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerDimensionManager::shutdown");

    if (!m_initialized) {
        return;
    }

    // 清理玩家映射
    m_playerDimensions.clear();
    m_dimensionPlayers.clear();

    // 调用基类关闭：基类通过 m_dimensions.clear() 析构每个 ServerDimension，
    // 间接触发各维度的 ServerDimension::shutdown()（带 per-dimension trace）。
    DimensionManager::shutdown();

    m_initialized = false;
}

// ============================================================================
// 维度访问
// ============================================================================

ServerDimension* ServerDimensionManager::getDimension(DimensionId id)
{
    return static_cast<ServerDimension*>(DimensionManager::getDimension(id));
}

const ServerDimension* ServerDimensionManager::getDimension(DimensionId id) const
{
    return static_cast<const ServerDimension*>(DimensionManager::getDimension(id));
}

ServerDimension* ServerDimensionManager::getOverworld()
{
    return getDimension(OVERWORLD);
}

const ServerDimension* ServerDimensionManager::getOverworld() const
{
    return getDimension(OVERWORLD);
}

ServerDimension* ServerDimensionManager::getNether()
{
    return getDimension(NETHER);
}

const ServerDimension* ServerDimensionManager::getNether() const
{
    return getDimension(NETHER);
}

ServerDimension* ServerDimensionManager::getTheEnd()
{
    return getDimension(THE_END);
}

const ServerDimension* ServerDimensionManager::getTheEnd() const
{
    return getDimension(THE_END);
}

// ============================================================================
// 玩家维度管理
// ============================================================================

void ServerDimensionManager::playerJoinDimension(PlayerId playerId, DimensionId dimId)
{
    auto* dim = getDimension(dimId);
    if (!dim) {
        return;
    }

    // 从旧维度移除
    playerLeaveDimension(playerId);

    // 添加到新维度
    m_playerDimensions[playerId] = dimId;
    m_dimensionPlayers[dimId].insert(playerId);
    dim->addPlayer(playerId);
}

void ServerDimensionManager::playerLeaveDimension(PlayerId playerId)
{
    auto it = m_playerDimensions.find(playerId);
    if (it == m_playerDimensions.end()) {
        return;
    }

    DimensionId oldDimId = it->second;
    auto* dim = getDimension(oldDimId);
    if (dim) {
        dim->removePlayer(playerId);
    }

    m_dimensionPlayers[oldDimId].erase(playerId);
    m_playerDimensions.erase(it);
}

DimensionId ServerDimensionManager::getPlayerDimension(PlayerId playerId) const
{
    auto it = m_playerDimensions.find(playerId);
    return it != m_playerDimensions.end() ? it->second : static_cast<DimensionId>(-1);
}

ServerDimension* ServerDimensionManager::getPlayerDimensionWorld(PlayerId playerId)
{
    DimensionId dimId = getPlayerDimension(playerId);
    if (dimId < 0) {
        return nullptr;
    }
    return getDimension(dimId);
}

std::vector<PlayerId> ServerDimensionManager::getPlayersInDimension(DimensionId dimId) const
{
    std::vector<PlayerId> players;
    auto it = m_dimensionPlayers.find(dimId);
    if (it != m_dimensionPlayers.end()) {
        players.reserve(it->second.size());
        for (PlayerId playerId : it->second) {
            players.push_back(playerId);
        }
    }
    return players;
}

bool ServerDimensionManager::isPlayerInDimension(PlayerId playerId, DimensionId dimId) const
{
    auto it = m_dimensionPlayers.find(dimId);
    if (it == m_dimensionPlayers.end()) {
        return false;
    }
    return it->second.find(playerId) != it->second.end();
}

// ============================================================================
// 维度切换
// ============================================================================

bool ServerDimensionManager::transferPlayerToDimension(
    PlayerId playerId, DimensionId targetDim, const std::optional<Vector3d>& position)
{
    // 获取目标维度
    auto* targetDimension = getDimension(targetDim);
    if (!targetDimension) {
        return false;
    }

    // 获取玩家当前维度
    DimensionId fromDim = getPlayerDimension(playerId);
    if (fromDim == targetDim) {
        // 已经在目标维度，无需切换
        return true;
    }

    // 确定目标位置
    Vector3d targetPos = position.value_or(targetDimension->spawnPoint());

    // 卸载当前维度的区块
    _unloadPlayerChunks(playerId);

    // 从旧维度移除
    playerLeaveDimension(playerId);

    // 添加到新维度
    playerJoinDimension(playerId, targetDim);

    // 发送维度切换包
    _sendDimensionChangePacket(playerId, targetDim, targetPos);

    // 加载新维度的区块
    _loadPlayerChunks(playerId, targetDimension);

    // 触发回调
    if (m_dimensionChangeCallback) {
        m_dimensionChangeCallback(playerId, fromDim, targetDim, targetPos);
    }

    return true;
}

// ============================================================================
// 更新
// ============================================================================

void ServerDimensionManager::tick()
{
    forEachDimension([](Dimension& dim) { dim.tick(); });
}

// ============================================================================
// 加载/卸载
// ============================================================================

ServerDimension* ServerDimensionManager::loadDimension(DimensionId id)
{
    if (hasDimension(id)) {
        return getDimension(id);
    }

    auto dim = _createServerDimension(id, m_seed);
    if (!dim) {
        return nullptr;
    }

    auto result = dim->initialize();
    if (result.failed()) {
        return nullptr;
    }

    if (!registerDimension(std::move(dim))) {
        return nullptr;
    }

    return getDimension(id);
}

bool ServerDimensionManager::unloadDimension(DimensionId id)
{
    // 不能卸载主维度
    if (id == OVERWORLD) {
        return false;
    }

    // 检查是否有玩家在维度中
    auto it = m_dimensionPlayers.find(id);
    if (it != m_dimensionPlayers.end() && !it->second.empty()) {
        return false;
    }

    // 卸载维度
    return unregisterDimension(id);
}

bool ServerDimensionManager::isDimensionLoaded(DimensionId id) const
{
    return hasDimension(id);
}

// ============================================================================
// 内部方法
// ============================================================================

std::unique_ptr<ServerDimension> ServerDimensionManager::_createServerDimension(DimensionId id, u64 seed)
{
    // 创建维度类型
    DimensionType type = DimensionType::overworld(); // 默认值
    switch (id) {
        case OVERWORLD:
            type = DimensionType::overworld();
            break;
        case NETHER:
            type = DimensionType::nether();
            break;
        case THE_END:
            type = DimensionType::theEnd();
            break;
        default:
            MC_ASSERT_RELEASE_MSG(false, "Unknown dimension id");
            return nullptr;
    }

    // 根据维度类型创建生成器和生物群系提供者
    std::unique_ptr<IChunkGenerator> generator;

    switch (id) {
        case OVERWORLD: {
            if (m_overworldType == WorldType::Debug) {
                generator = std::make_unique<DebugChunkGenerator>();
                break;
            }

            // 超平坦世界走 FlatChunkGenerator（不走 NoiseChunkGenerator / RandomState::create）。
            // 阶段4 改造点 D：flat 无对应 noise_settings JSON，无法走数据驱动噪声路径。
            if (m_overworldType == WorldType::Flat) {
                generator = std::make_unique<FlatChunkGenerator>(seed, FlatLevelGeneratorSettings::createDefault());
                break;
            }

            DimensionSettings settings;
            switch (m_overworldType) {
                case WorldType::LargeBiomes:
                    settings = DimensionSettings::largeBiomesPreset();
                    break;
                case WorldType::Amplified:
                    settings = DimensionSettings::amplified();
                    break;
                case WorldType::Flat:
                case WorldType::Default:
                case WorldType::Debug:
                default:
                    settings = DimensionSettings::overworld();
                    break;
            }

            const bool isLargeBiomes = (m_overworldType == WorldType::LargeBiomes);
            const bool isAmplified = (m_overworldType == WorldType::Amplified);

            // 先构造 RandomState，再由生物群系源与生成器共享同一噪声缓存。
            auto randomState = mc::world::gen::RandomState::create(settings, seed);
            auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(
                *randomState, isLargeBiomes, isAmplified);
            generator = std::make_unique<NoiseChunkGenerator>(
                std::move(settings), std::move(biomeSource), std::move(randomState));
            break;
        }
        case NETHER: {
            auto settings = DimensionSettings::nether();
            auto randomState = mc::world::gen::RandomState::create(settings, seed);
            auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createNether(*randomState);
            generator = std::make_unique<NoiseChunkGenerator>(
                std::move(settings), std::move(biomeSource), std::move(randomState));
            break;
        }
        case THE_END: {
            auto settings = DimensionSettings::end();
            auto randomState = mc::world::gen::RandomState::create(settings, seed);
            auto biomeSource = std::make_unique<mc::world::biome::source::EndBiomeSource>(*randomState);
            generator = std::make_unique<NoiseChunkGenerator>(
                std::move(settings), std::move(biomeSource), std::move(randomState));
            break;
        }
        default:
            return nullptr;
    }

    auto world = _createServerWorld(id, seed, std::move(generator));
    auto dimension = std::make_unique<ServerDimension>(id, std::move(type), nullptr, seed, m_viewDistance);
    dimension->setWorld(std::move(world));
    return dimension;
}

std::unique_ptr<server::ServerWorld> ServerDimensionManager::_createServerWorld(
    DimensionId id, u64 seed, std::unique_ptr<IChunkGenerator> generator) const
{
    MC_ASSERT_RELEASE(m_server != nullptr);
    MC_ASSERT_RELEASE(generator != nullptr);

    server::ServerWorldConfig worldConfig;
    worldConfig.viewDistance = m_viewDistance;
    worldConfig.dimension = id;
    worldConfig.seed = seed;

    auto world = std::make_unique<server::ServerWorld>(worldConfig);
    auto chunkManager = std::make_unique<server::ServerChunkManager>(*world, std::move(generator));
    chunkManager->setWorkerPool(&m_server->m_computationWorkerPool);
    chunkManager->setViewDistance(m_viewDistance);
    world->setChunkManager(std::move(chunkManager));
    world->setSharedStorage(m_server->m_storage.get());
    world->setTimeManager(&m_server->timeManager());
    world->setDifficultyCallback([server = m_server]() { return server->difficulty(); });
    world->setLootTableManager(&m_server->m_lootTableManager);
    return world;
}

void ServerDimensionManager::_sendDimensionChangePacket(PlayerId playerId, DimensionId newDim, const Vector3d& pos)
{
    // 获取维度类型
    auto* targetDim = getDimension(newDim);
    if (!targetDim) {
        return;
    }

    // 使用 RespawnPacket 进行维度切换
    network::RespawnPacket packet;

    // 设置维度类型
    // 0 = minecraft:overworld
    // 1 = minecraft:the_nether
    // 2 = minecraft:the_end
    i32 dimensionTypeId = 0;
    switch (newDim) {
        case 0: // Overworld
            dimensionTypeId = 0;
            break;
        case -1: // Nether
            dimensionTypeId = 1;
            break;
        case 1: // The End
            dimensionTypeId = 2;
            break;
        default:
            dimensionTypeId = 0;
            break;
    }
    packet.setDimensionType(dimensionTypeId);
    packet.setDimension(newDim);

    // 计算世界种子的哈希值（SHA-256 前 8 字节）
    packet.setHashedSeed(util::crypto::Sha256::hashWorldSeed(m_seed));

    // 设置游戏模式（从玩家数据获取）
    auto* playerData = m_server->playerManager().getPlayer(playerId);
    if (playerData) {
        packet.setGameMode(playerData->gameMode);
        packet.setPreviousGameMode(GameMode::NotSet);
    }

    // 维度切换时保留数据
    packet.setKeepData(true);

    // 设置上次死亡位置（从玩家实体获取）
    auto& playerEntityManager = m_server->playerEntityManager();
    if (auto* dimension = getPlayerDimensionWorld(playerId)) {
        if (auto* world = dimension->world()) {
            if (Player* player = playerEntityManager.getPlayerEntity(playerId, *world)) {
                packet.setLastDeathLocation(player->getLastDeathLocation());
            }
        }
    }

    // 序列化包
    auto result = packet.serialize();
    if (result.failed()) {
        return;
    }

    // 通过服务器发送给玩家
    m_server->sendPacketToPlayer(playerId, result.value().data(), result.value().size());
}

void ServerDimensionManager::_unloadPlayerChunks(PlayerId playerId)
{
    // 获取玩家当前维度
    DimensionId dimId = getPlayerDimension(playerId);
    if (dimId < 0) {
        return;
    }

    auto* dim = getDimension(dimId);
    if (!dim || !dim->world()) {
        return;
    }

    // 获取玩家当前区块位置
    auto* playerData = m_server->playerManager().getPlayer(playerId);
    if (!playerData) {
        return;
    }

    // 计算玩家视野范围内的区块并发送卸载通知
    ChunkCoord playerChunkX = static_cast<ChunkCoord>(std::floor(playerData->x / static_cast<f32>(world::CHUNK_WIDTH)));
    ChunkCoord playerChunkZ = static_cast<ChunkCoord>(std::floor(playerData->z / static_cast<f32>(world::CHUNK_WIDTH)));

    // 通过维度的 ChunkSendManager 卸载区块
    auto* chunkSendMgr = dim->chunkSendManager();
    if (chunkSendMgr) {
        i32 viewDistance = m_viewDistance;

        // 卸载视野范围内的所有区块
        for (ChunkCoord dx = -viewDistance; dx <= viewDistance; ++dx) {
            for (ChunkCoord dz = -viewDistance; dz <= viewDistance; ++dz) {
                if (dx * dx + dz * dz <= viewDistance * viewDistance) {
                    ChunkCoord cx = playerChunkX + dx;
                    ChunkCoord cz = playerChunkZ + dz;
                    chunkSendMgr->unloadChunkFromPlayers(cx, cz, {playerId});
                }
            }
        }
    }
}

void ServerDimensionManager::_loadPlayerChunks(PlayerId playerId, ServerDimension* dim)
{
    if (!dim || !dim->world()) {
        return;
    }

    // 获取玩家当前位置
    auto* playerData = m_server->playerManager().getPlayer(playerId);
    if (!playerData) {
        return;
    }

    // 计算玩家区块位置
    ChunkCoord playerChunkX = static_cast<ChunkCoord>(std::floor(playerData->x / static_cast<f32>(world::CHUNK_WIDTH)));
    ChunkCoord playerChunkZ = static_cast<ChunkCoord>(std::floor(playerData->z / static_cast<f32>(world::CHUNK_WIDTH)));

    // 通过维度的 ChunkSendManager 加载区块
    auto* chunkSendMgr = dim->chunkSendManager();
    if (chunkSendMgr) {
        i32 viewDistance = m_viewDistance;

        // 加载视野范围内的所有区块
        for (ChunkCoord dx = -viewDistance; dx <= viewDistance; ++dx) {
            for (ChunkCoord dz = -viewDistance; dz <= viewDistance; ++dz) {
                if (dx * dx + dz * dz <= viewDistance * viewDistance) {
                    ChunkCoord cx = playerChunkX + dx;
                    ChunkCoord cz = playerChunkZ + dz;
                    chunkSendMgr->sendChunkToPlayers(cx, cz, {playerId});
                }
            }
        }
    }
}

} // namespace mc
