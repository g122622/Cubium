#include "ServerDimensionManager.hpp"
#include "../application/MinecraftServer.hpp"
#include "../sync/ChunkSendManager.hpp"
#include "../world/ServerChunkManager.hpp"
#include "common/core/Result.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/chunk/NetherChunkGenerator.hpp"
#include "common/world/gen/chunk/EndChunkGenerator.hpp"
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/biome/provider/nether/NetherBiomeProvider.hpp"
#include "common/world/biome/provider/end/EndBiomeProvider.hpp"
#include "common/network/packet/DimensionPackets.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/perfetto/TraceEvents.hpp"

#include <algorithm>
#include <cmath>

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

Result<void> ServerDimensionManager::initialize(u64 seed, i32 viewDistance) {
    MC_TRACE_EVENT("server.initialization", "ServerDimensionManager::initialize");

    if (m_initialized) {
        return {};
    }

    m_seed = seed;
    m_viewDistance = viewDistance;

    // 调用基类初始化，创建 Dimension 实例
    DimensionManager::initialize(seed);

    // 为每个维度创建 ServerDimension 包装
    // 注意：这里需要重新创建 ServerDimension 实例
    m_dimensions.clear();  // 清除基类创建的 Dimension 实例

    // 创建主世界
    auto overworld = createServerDimension(OVERWORLD, seed);
    MC_ASSERT_MSG(registerDimension(std::move(overworld)), "Failed to register overworld");

    // 创建下界
    auto nether = createServerDimension(NETHER, seed);
    MC_ASSERT_MSG(registerDimension(std::move(nether)), "Failed to register nether");

    // 创建末地
    auto theEnd = createServerDimension(THE_END, seed);
    MC_ASSERT_MSG(registerDimension(std::move(theEnd)), "Failed to register the end");

    m_initialized = true;
    return {};
}

void ServerDimensionManager::shutdown() {
    // 清理玩家映射
    m_playerDimensions.clear();
    m_dimensionPlayers.clear();

    // 调用基类关闭
    DimensionManager::shutdown();

    m_initialized = false;
}

// ============================================================================
// 维度访问
// ============================================================================

ServerDimension* ServerDimensionManager::getDimension(DimensionId id) {
    return static_cast<ServerDimension*>(DimensionManager::getDimension(id));
}

const ServerDimension* ServerDimensionManager::getDimension(DimensionId id) const {
    return static_cast<const ServerDimension*>(DimensionManager::getDimension(id));
}

ServerDimension* ServerDimensionManager::getOverworld() {
    return getDimension(OVERWORLD);
}

const ServerDimension* ServerDimensionManager::getOverworld() const {
    return getDimension(OVERWORLD);
}

ServerDimension* ServerDimensionManager::getNether() {
    return getDimension(NETHER);
}

const ServerDimension* ServerDimensionManager::getNether() const {
    return getDimension(NETHER);
}

ServerDimension* ServerDimensionManager::getTheEnd() {
    return getDimension(THE_END);
}

const ServerDimension* ServerDimensionManager::getTheEnd() const {
    return getDimension(THE_END);
}

// ============================================================================
// 玩家维度管理
// ============================================================================

void ServerDimensionManager::playerJoinDimension(PlayerId playerId, DimensionId dimId) {
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

void ServerDimensionManager::playerLeaveDimension(PlayerId playerId) {
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

DimensionId ServerDimensionManager::getPlayerDimension(PlayerId playerId) const {
    auto it = m_playerDimensions.find(playerId);
    return it != m_playerDimensions.end() ? it->second : static_cast<DimensionId>(-1);
}

ServerDimension* ServerDimensionManager::getPlayerDimensionWorld(PlayerId playerId) {
    DimensionId dimId = getPlayerDimension(playerId);
    if (dimId < 0) {
        return nullptr;
    }
    return getDimension(dimId);
}

std::vector<PlayerId> ServerDimensionManager::getPlayersInDimension(DimensionId dimId) const {
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

bool ServerDimensionManager::isPlayerInDimension(PlayerId playerId, DimensionId dimId) const {
    auto it = m_dimensionPlayers.find(dimId);
    if (it == m_dimensionPlayers.end()) {
        return false;
    }
    return it->second.find(playerId) != it->second.end();
}

// ============================================================================
// 维度切换
// ============================================================================

bool ServerDimensionManager::transferPlayerToDimension(PlayerId playerId,
                                                        DimensionId targetDim,
                                                        const Optional<Vector3d>& position) {
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
    unloadPlayerChunks(playerId);

    // 从旧维度移除
    playerLeaveDimension(playerId);

    // 添加到新维度
    playerJoinDimension(playerId, targetDim);

    // 发送维度切换包
    sendDimensionChangePacket(playerId, targetDim, targetPos);

    // 加载新维度的区块
    loadPlayerChunks(playerId, targetDimension);

    // 触发回调
    if (m_dimensionChangeCallback) {
        m_dimensionChangeCallback(playerId, fromDim, targetDim, targetPos);
    }

    return true;
}

// ============================================================================
// 更新
// ============================================================================

void ServerDimensionManager::tick() {
    forEachDimension([](Dimension& dim) {
        dim.tick();
    });
}

// ============================================================================
// 加载/卸载
// ============================================================================

ServerDimension* ServerDimensionManager::loadDimension(DimensionId id) {
    if (hasDimension(id)) {
        return getDimension(id);
    }

    auto dim = createServerDimension(id, m_seed);
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

bool ServerDimensionManager::unloadDimension(DimensionId id) {
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

bool ServerDimensionManager::isDimensionLoaded(DimensionId id) const {
    return hasDimension(id);
}

// ============================================================================
// 内部方法
// ============================================================================

std::unique_ptr<ServerDimension> ServerDimensionManager::createServerDimension(DimensionId id, u64 seed) {
    // 创建维度类型
    DimensionType type = DimensionType::overworld();  // 默认值
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
            MC_ASSERT_MSG(false, "Unknown dimension id");
            return nullptr;
    }

    // 根据维度类型创建生成器和生物群系提供者
    std::unique_ptr<IChunkGenerator> generator;
    std::unique_ptr<BiomeProvider> biomeProvider;

    switch (id) {
        case OVERWORLD: {
            auto settings = DimensionSettings::overworld();
            generator = std::make_unique<NoiseChunkGenerator>(seed, std::move(settings));
            biomeProvider = std::make_unique<LayerBiomeProvider>(seed, false);
            break;
        }
        case NETHER: {
            auto settings = DimensionSettings::nether();
            generator = std::make_unique<NetherChunkGenerator>(seed, std::move(settings));
            biomeProvider = std::make_unique<biome::nether::NetherBiomeProvider>(seed);
            break;
        }
        case THE_END: {
            auto settings = DimensionSettings::end();
            generator = std::make_unique<EndChunkGenerator>(seed, std::move(settings));
            biomeProvider = std::make_unique<biome::end::EndBiomeProvider>(seed);
            break;
        }
        default:
            return nullptr;
    }

    return std::make_unique<ServerDimension>(
        id,
        std::move(type),
        std::move(generator),
        std::move(biomeProvider),
        seed,
        m_viewDistance
    );
}

void ServerDimensionManager::sendDimensionChangePacket(PlayerId playerId, DimensionId newDim, const Vector3d& pos) {
    // 创建维度切换包
    network::ChangeDimensionPacket packet;
    packet.setDimension(newDim);
    packet.setPosition(pos);
    packet.setRespawn(false);

    // 序列化包
    auto result = packet.serialize();
    if (result.failed()) {
        return;
    }

    // 通过服务器发送给玩家
    m_server->sendPacketToPlayer(playerId, result.value().data(), result.value().size());
}

void ServerDimensionManager::unloadPlayerChunks(PlayerId playerId) {
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
    ChunkCoord playerChunkX = static_cast<ChunkCoord>(std::floor(playerData->x / 16.0f));
    ChunkCoord playerChunkZ = static_cast<ChunkCoord>(std::floor(playerData->z / 16.0f));

    auto& chunkSendManager = m_server->chunkSendManager();
    i32 viewDistance = m_viewDistance;

    // 卸载视野范围内的所有区块
    for (ChunkCoord dx = -viewDistance; dx <= viewDistance; ++dx) {
        for (ChunkCoord dz = -viewDistance; dz <= viewDistance; ++dz) {
            if (dx * dx + dz * dz <= viewDistance * viewDistance) {
                ChunkCoord cx = playerChunkX + dx;
                ChunkCoord cz = playerChunkZ + dz;
                chunkSendManager.unloadChunkFromPlayers(cx, cz, {playerId});
            }
        }
    }
}

void ServerDimensionManager::loadPlayerChunks(PlayerId playerId, ServerDimension* dim) {
    if (!dim || !dim->world()) {
        return;
    }

    // 获取玩家当前位置
    auto* playerData = m_server->playerManager().getPlayer(playerId);
    if (!playerData) {
        return;
    }

    // 计算玩家区块位置
    ChunkCoord playerChunkX = static_cast<ChunkCoord>(std::floor(playerData->x / 16.0f));
    ChunkCoord playerChunkZ = static_cast<ChunkCoord>(std::floor(playerData->z / 16.0f));

    auto& chunkSendManager = m_server->chunkSendManager();
    i32 viewDistance = m_viewDistance;

    // 加载视野范围内的所有区块
    for (ChunkCoord dx = -viewDistance; dx <= viewDistance; ++dx) {
        for (ChunkCoord dz = -viewDistance; dz <= viewDistance; ++dz) {
            if (dx * dx + dz * dz <= viewDistance * viewDistance) {
                ChunkCoord cx = playerChunkX + dx;
                ChunkCoord cz = playerChunkZ + dz;
                chunkSendManager.sendChunkToPlayers(cx, cz, {playerId});
            }
        }
    }
}

} // namespace mc
