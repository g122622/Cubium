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
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/crypto/Sha256.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/biome/source/EndBiomeSource.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorPresetRegistry.hpp"
#include "common/world/gen/settings/NoiseSettingsRegistry.hpp"
#include "common/world/gen/settings/WorldPreset.hpp"
#include "common/world/gen/settings/WorldPresetRegistry.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <cmath>
#include <fmt/format.h>
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

Result<void> ServerDimensionManager::initialize(
    u64 seed, i32 viewDistance, WorldType overworldType, resource::ResourceLocation worldPresetId)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerDimensionManager::initialize");

    if (m_initialized) {
        return {};
    }

    m_seed = seed;
    m_viewDistance = viewDistance;
    m_overworldType = overworldType;
    m_worldPresetId = std::move(worldPresetId);

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
// 内部方法
// ============================================================================

std::unique_ptr<ServerDimension> ServerDimensionManager::_createServerDimension(DimensionId id, u64 seed)
{
    // 数据驱动唯一路径：查 WorldPresetRegistry 取 WorldPreset，按 id 映射维度键装配三维度。
    // WorldPresetRegistry 未加载（数据包缺失）时回退旧 WorldType switch（仅验证期保留，5f 删旧工厂后清）。
    const auto* worldPreset = world::gen::settings::WorldPresetRegistry::instance().get(m_worldPresetId);

    // id → 维度键（world_preset JSON dimensions 的 key）
    resource::ResourceLocation dimKey;
    switch (id) {
        case OVERWORLD:
            dimKey = resource::ResourceLocation::parse("minecraft:overworld");
            break;
        case NETHER:
            dimKey = resource::ResourceLocation::parse("minecraft:the_nether");
            break;
        case THE_END:
            dimKey = resource::ResourceLocation::parse("minecraft:the_end");
            break;
        default:
            MC_ASSERT_RELEASE_MSG(false, fmt::format("Unknown dimension id {}", id).c_str());
            return nullptr;
    }

    // 维度类型（DimensionType 当前非数据驱动，按 dimensionType RL 映射 3 静态工厂）
    DimensionType type = DimensionType::overworld();
    if (id == NETHER) {
        type = DimensionType::nether();
    } else if (id == THE_END) {
        type = DimensionType::theEnd();
    }

    std::unique_ptr<IChunkGenerator> generator;

    if (worldPreset != nullptr) {
        const auto dimIt = worldPreset->dimensions.find(dimKey);
        MC_ASSERT_RELEASE_MSG(dimIt != worldPreset->dimensions.end(),
            fmt::format("WorldPreset '{}' has no dimension '{}'", m_worldPresetId.toString(), dimKey.toString())
                .c_str());
        const auto& dim = dimIt->second;
        const auto& gen = dim.generator;

        switch (gen.type) {
            case world::gen::settings::WorldPresetGenerator::Type::Noise: {
                // 查 NoiseSettingsRegistry 取 DimensionSettings（含 m_routerDfs 模板 + m_surfaceRule）
                const auto* settings = world::gen::settings::NoiseSettingsRegistry::instance().get(gen.noiseSettings);
                MC_ASSERT_RELEASE_MSG(settings != nullptr,
                    fmt::format("noise_settings '{}' not in NoiseSettingsRegistry (world_preset '{}')",
                        gen.noiseSettings.toString(),
                        m_worldPresetId.toString())
                        .c_str());
                DimensionSettings dimSettings = *settings;

                // 先构造 RandomState，再由生物群系源与生成器共享同一噪声缓存。
                auto randomState = world::gen::RandomState::create(dimSettings, seed);
                auto biomeSource = _createBiomeSource(gen, *randomState, seed);
                generator = std::make_unique<NoiseChunkGenerator>(
                    std::move(dimSettings), std::move(biomeSource), std::move(randomState));
                break;
            }
            case world::gen::settings::WorldPresetGenerator::Type::Flat:
                // flat 维度内联 settings 已在解析期产 FlatLevelGeneratorSettings（WorldPresetGenerator.flatSettings）
                generator = std::make_unique<FlatChunkGenerator>(seed, gen.flatSettings);
                break;
            case world::gen::settings::WorldPresetGenerator::Type::Debug:
                generator = std::make_unique<DebugChunkGenerator>();
                break;
        }
    } else {
        // 兜底：WorldPresetRegistry 未加载（数据包缺失/测试未加载）。保留旧 WorldType 装配。
        spdlog::warn(
            "WorldPreset '{}' not loaded, falling back to legacy WorldType assembly", m_worldPresetId.toString());
        generator = _createLegacyGenerator(id, seed);
    }

    MC_ASSERT_RELEASE_MSG(
        generator != nullptr, fmt::format("Failed to create chunk generator for dimension {}", id).c_str());

    auto world = _createServerWorld(id, seed, std::move(generator));
    auto dimension = std::make_unique<ServerDimension>(id, std::move(type), nullptr, seed, m_viewDistance);
    dimension->setWorld(std::move(world));
    return dimension;
}

std::unique_ptr<world::biome::IBiomeSource> ServerDimensionManager::_createBiomeSource(
    const world::gen::settings::WorldPresetGenerator& gen, const world::gen::RandomState& rs, u64 seed)
{
    using BS = world::gen::settings::WorldPresetGenerator::BiomeSourceType;
    switch (gen.biomeSourceType) {
        case BS::MultiNoise: {
            // preset 名映射：minecraft:overworld→createOverworld，minecraft:nether→createNether
            // largeBiomes/amplified 由 world_preset 名推导（createOverworld 当前 (void) 丢弃，参数仅保留接口）
            const bool isLargeBiomes = (m_worldPresetId == resource::ResourceLocation("minecraft", "large_biomes"));
            const bool isAmplified = (m_worldPresetId == resource::ResourceLocation("minecraft", "amplified"));
            if (gen.multiNoisePreset == resource::ResourceLocation("minecraft", "nether")) {
                return world::biome::source::MultiNoiseBiomeSource::createNether(rs);
            }
            return world::biome::source::MultiNoiseBiomeSource::createOverworld(rs, isLargeBiomes, isAmplified);
        }
        case BS::TheEnd:
            return std::make_unique<world::biome::source::EndBiomeSource>(rs);
        case BS::Fixed: {
            auto biomeId = world::biome::BiomeLoader::biomeIdByName(gen.fixedBiome);
            MC_ASSERT_RELEASE_MSG(biomeId.has_value(),
                fmt::format("world_preset fixed biome '{}' has no BiomeId mapping", gen.fixedBiome.toString()).c_str());
            return std::make_unique<world::biome::source::FixedBiomeSource>(seed, biomeId.value());
        }
    }
    MC_ASSERT_RELEASE_MSG(false, "Unsupported biome_source type");
    return nullptr;
}

std::unique_ptr<IChunkGenerator> ServerDimensionManager::_createLegacyGenerator(DimensionId id, u64 seed)
{
    // 旧 WorldType 装配兜底（WorldPresetRegistry 未加载时）。
    switch (id) {
        case OVERWORLD: {
            if (m_overworldType == WorldType::Debug) {
                return std::make_unique<DebugChunkGenerator>();
            }
            if (m_overworldType == WorldType::Flat) {
                // flat 兜底：查 FlatLevelGeneratorPresetRegistry 取 classic_flat，未加载回退 createDefault()
                FlatLevelGeneratorSettings flatSettings = FlatLevelGeneratorSettings::createDefault();
                const auto presetId = resource::ResourceLocation("minecraft", "classic_flat");
                const auto* preset = world::gen::settings::FlatLevelGeneratorPresetRegistry::instance().get(presetId);
                if (preset != nullptr) {
                    flatSettings = *preset;
                }
                return std::make_unique<FlatChunkGenerator>(seed, std::move(flatSettings));
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
            auto randomState = world::gen::RandomState::create(settings, seed);
            auto biomeSource =
                world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, isLargeBiomes, isAmplified);
            return std::make_unique<NoiseChunkGenerator>(
                std::move(settings), std::move(biomeSource), std::move(randomState));
        }
        case NETHER: {
            auto settings = DimensionSettings::nether();
            auto randomState = world::gen::RandomState::create(settings, seed);
            auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(*randomState);
            return std::make_unique<NoiseChunkGenerator>(
                std::move(settings), std::move(biomeSource), std::move(randomState));
        }
        case THE_END: {
            auto settings = DimensionSettings::end();
            auto randomState = world::gen::RandomState::create(settings, seed);
            auto biomeSource = std::make_unique<world::biome::source::EndBiomeSource>(*randomState);
            return std::make_unique<NoiseChunkGenerator>(
                std::move(settings), std::move(biomeSource), std::move(randomState));
        }
        default:
            return nullptr;
    }
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
    chunkManager->setWorkerPool(&m_server->computationWorkerPool());
    chunkManager->setViewDistance(m_viewDistance);
    world->setChunkManager(std::move(chunkManager));
    world->setSharedStorage(m_server->sharedStorage());
    world->setTimeManager(&m_server->timeManager());
    world->setDifficultyCallback([server = m_server]() { return server->difficulty(); });
    world->setLootTableManager(&m_server->lootTableManager());
    return world;
}

void ServerDimensionManager::_sendDimensionChangePacket(PlayerId playerId, DimensionId newDim, const Vector3d& pos)
{
    // 获取维度类型
    auto* targetDim = getDimension(newDim);
    if (!targetDim) {
        return;
    }

    // 1.21.11 Respawn：复用 CommonPlayerSpawnInfo + u8 dataToKeep。
    // dimensionType 为 dimension_type 注册表 holder id（纯 VarInt）。客户端在 Configuration
    // 阶段按 RegistryDataBuilder 下发顺序 [overworld, overworld_caves, the_nether, the_end]
    // register() 自增分配 id，故 overworld=0/overworld_caves=1/the_nether=2/the_end=3。
    // dimension 为维度 ResourceKey 字符串。
    i32 dimensionTypeId = 0;
    std::string dimensionKey = "minecraft:overworld";
    switch (newDim) {
        case 0: // Overworld
            dimensionTypeId = 0;
            dimensionKey = "minecraft:overworld";
            break;
        case -1: // Nether
            dimensionTypeId = 2;
            dimensionKey = "minecraft:the_nether";
            break;
        case 1: // The End
            dimensionTypeId = 3;
            dimensionKey = "minecraft:the_end";
            break;
        default:
            dimensionTypeId = 0;
            dimensionKey = "minecraft:overworld";
            break;
    }

    mc::network::ir::play::Respawn pkt;
    pkt.spawnInfo.dimensionType = dimensionTypeId;
    pkt.spawnInfo.dimension = dimensionKey;
    pkt.spawnInfo.seed = static_cast<i64>(util::crypto::Sha256::hashWorldSeed(m_seed));

    // 设置游戏模式（从玩家数据获取）
    auto* playerData = m_server->playerManager().getPlayer(playerId);
    if (playerData) {
        pkt.spawnInfo.gameType = playerData->gameMode;
        pkt.spawnInfo.previousGameType = -1; // NotSet → null（1.21.11 用 -1 表 null）
    }

    // 维度切换时保留数据（KEEP_ALL_DATA = 3）
    pkt.dataToKeep = 3;

    // isDebug/isFlat 从目标维度世界获取
    if (auto* world = targetDim->world()) {
        pkt.spawnInfo.isDebug = world->isDebugWorld();
    }
    // isFlat 仅主世界可能为超平坦（下界/末地生成器非 flat）。由装配期记录的
    // m_overworldType 推导，对齐 vanilla WorldData.isFlatWorld()。
    pkt.spawnInfo.isFlat = (newDim == 0 && m_overworldType == WorldType::Flat);

    // 设置上次死亡位置（从玩家实体获取）
    auto& playerEntityManager = m_server->playerEntityManager();
    if (auto* dimension = getPlayerDimensionWorld(playerId)) {
        if (auto* world = dimension->world()) {
            if (Player* player = playerEntityManager.getPlayerEntity(playerId, *world)) {
                auto lastDeath = player->getLastDeathLocation();
                if (lastDeath.has_value()) {
                    // GlobalPos → (dimension ResourceKey, BlockPos.asLong)
                    std::string deathDimKey;
                    switch (lastDeath->getDimensionId()) {
                        case 0:
                            deathDimKey = "minecraft:overworld";
                            break;
                        case -1:
                            deathDimKey = "minecraft:the_nether";
                            break;
                        case 1:
                            deathDimKey = "minecraft:the_end";
                            break;
                        default:
                            deathDimKey = "minecraft:overworld";
                            break;
                    }
                    pkt.spawnInfo.lastDeathLocation = std::make_pair(deathDimKey, lastDeath->getPos().asLong());
                }
            }
        }
    }

    (void)pos; // 旧 RespawnPacket 不携带坐标；1.21.11 Respawn 后由 PlayerPosition 单独传送
    m_server->sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{std::move(pkt)},
        });
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
