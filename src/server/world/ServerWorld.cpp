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

// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "ServerWorld.hpp"
#include "ChunkLoadLightTask.hpp"
#include "ServerChunkManager.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/player/SpawnLocationHelper.hpp"
#include "common/entity/inventory/INamedContainerProvider.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/core/CoordConverter.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gameevent/GameEventDispatcher.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/gen/FeaturePlacer.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include "common/world/gen/structure/StructureTags.hpp"
#include "common/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"
#include "common/world/gen/structure/placement/RandomSpreadStructurePlacement.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/storage/entity/EntityStorageManager.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include "server/application/IServer.hpp"
#include "server/core/TimeManager.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include "server/world/blockentity/sculk/SculkVibrationSystem.hpp"
#include "weather/WeatherManager.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <spdlog/spdlog.h>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

using namespace mc::trace;

namespace mc::server {

using mc::LightType;
using mc::NibbleArray;
using mc::StarLightLightingProvider;
using mc::WorldLightManager;
using mc::util::core::CoordConverter;
using mc::world::chunk::ChunkPos;
using mc::world::chunk::ChunkSection;
using mc::world::chunk::IChunk;
using mc::world::chunk::SectionPos;

// ============================================================================
// ServerWorld 实现
// ============================================================================

ServerWorld::ServerWorld(const ServerWorldConfig& config)
    : m_config(config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerWorld::Constructor", "dimension", config.dimension);
}

ServerWorld::ServerWorld(const ServerWorldConfig& config, std::unique_ptr<ServerChunkManager> chunkManager)
    : m_config(config)
    , m_chunkManager(std::move(chunkManager))
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerWorld::Constructor", "dimension", config.dimension);
    MC_ASSERT_RELEASE(m_chunkManager != nullptr);
    m_chunkManager->setViewDistance(m_config.viewDistance);
}

ServerWorld::~ServerWorld()
{
    shutdown();
}

Result<void> ServerWorld::initialize()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerWorld::initialize");
    spdlog::info("Initializing server world with seed {}...", m_config.seed);

    if (m_initialized) {
        return Result<void>::ok();
    }

    MC_ASSERT_RELEASE(m_storage != nullptr);
    MC_ASSERT_RELEASE(m_storage->isOpen());

    MC_ASSERT_RELEASE(m_chunkManager != nullptr);

    // 确保区块管理器始终使用世界配置中的视距。
    m_chunkManager->setViewDistance(m_config.viewDistance);

    auto result = m_chunkManager->initialize();
    if (result.failed()) {
        return result;
    }

    // 注意：区块加载/卸载回调由 MinecraftServer::setupWorldCallbacks() 设置，
    // 其中会调用 onChunkLoaded/onChunkUnloading 来处理实体持久化。

    m_collisionCache = std::make_unique<physics::CollisionCache>();
    m_physicsEngine = std::make_unique<PhysicsEngine>(*this);
    m_tickManager = std::make_unique<world::tick::TickManager>(*this);

    // 初始化随机数生成器（用于随机刻）
    m_random.setSeed(m_config.seed);

    // 根据维度类型动态设置光照参数
    DimensionType dimensionType = getDimensionType();
    bool hasSkyLight = dimensionType.hasSkyLight();
    m_lightManager = std::make_unique<WorldLightManager>(this, true, hasSkyLight);

    m_weatherManager = std::make_unique<WeatherManager>();
    m_weatherManager->initialize(m_config.seed);
    m_weatherManager->setWorld(this);

    // 初始化游戏事件分发器
    m_gameEventDispatcher = std::make_unique<gameevent::GameEventDispatcher>(*this);

    // 初始化幽匿振动系统管理器
    m_sculkVibrationManager.setWorld(*this);

    // 初始化村庄和袭击管理器
    m_villageManager = std::make_unique<world::village::VillageManager>(*this);
    m_raidManager = std::make_unique<world::village::raid::RaidManager>(*this, *m_villageManager);

    // 初始化末影龙战斗管理器（仅末地维度）
    if (m_config.dimension == DimensionManager::THE_END) {
        // 尝试从存档加载末影龙战斗数据
        std::optional<EndDragonFight::Data> dragonFightData;
        if (m_storage && m_storage->isOpen()) {
            auto result = m_storage->loadDragonFightData();
            if (result.success() && result.value().has_value()) {
                dragonFightData = EndDragonFight::Data::fromJson(*result.value());
            }
        }
        m_dragonFight = std::make_unique<EndDragonFight>(m_config.seed, dragonFightData);
    }

    // 初始化地图数据管理器
    m_mapDataManager = std::make_unique<world::map::MapDataManager>();

    m_initialized = true;
    spdlog::info("Server world initialized");
    return Result<void>::ok();
}

void ServerWorld::shutdown()
{
    if (!m_initialized && m_chunkManager == nullptr && m_weatherManager == nullptr && m_lightManager == nullptr &&
        m_tickManager == nullptr && m_physicsEngine == nullptr && m_collisionCache == nullptr &&
        m_villageManager == nullptr && m_raidManager == nullptr) {
        return;
    }

    // per-dimension 世界卸载 trace，带维度 ID 便于区分主世界/下界/末地。
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Initialization, "ServerWorld::shutdown", "dim", static_cast<i32>(m_config.dimension));

    spdlog::info("Shutting down server world...");
    m_initialized = false;

    // 先保存所有已加载区块内的实体
    if (m_storage && m_storage->isOpen() && m_chunkManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerWorld::shutdown::SaveEntitiesInChunks");
        m_chunkManager->forEachLoadedChunk([this](ChunkData& chunk) {
            auto entityIds = m_entityChunkTracker.getEntitiesInChunk(chunk.x(), chunk.z());
            if (!entityIds.empty()) {
                auto* entityStorage = m_storage->entityStorage();
                if (entityStorage) {
                    std::vector<std::reference_wrapper<Entity>> entitiesToSave;
                    entitiesToSave.reserve(entityIds.size());
                    for (EntityInstanceId id : entityIds) {
                        Entity* entity = m_entityManager.getEntity(id);
                        if (entity) {
                            entitiesToSave.emplace_back(*entity);
                        }
                    }
                    if (!entitiesToSave.empty()) {
                        auto saveResult = entityStorage->saveEntitiesInChunk(
                            entitiesToSave, chunk.x(), chunk.z(), m_config.dimension);
                        if (saveResult.failed()) {
                            spdlog::error("Failed to save entities for chunk ({}, {}) during shutdown: {}",
                                chunk.x(),
                                chunk.z(),
                                saveResult.error().message());
                        }
                    }
                }
            }
            return true;
        });
        m_entityChunkTracker.clear();
    }

    // 先清理袭击管理器（可能引用村庄）
    m_raidManager.reset();
    // 再清理村庄管理器
    m_villageManager.reset();

    // 先停止区块管理器，避免后台生成/加载回调在世界子系统拆除后继续触发方块更新。
    if (m_chunkManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerWorld::shutdown::ShutdownChunkManager");
        m_chunkManager->shutdown();
    }

    m_weatherManager.reset();
    m_lightManager.reset();
    m_tickManager.reset();
    m_physicsEngine.reset();
    m_collisionCache.reset();

    m_chunkManager.reset();

    spdlog::info("Server world shut down");
}

Result<size_t> ServerWorld::saveAll()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "ServerWorld::saveAll");

    if (m_storage == nullptr || !m_storage->isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
    }

    // 先保存所有已加载运行时实体，避免只在区块卸载时才落盘。
    auto* entityStorage = m_storage->entityStorage();
    if (entityStorage) {
        auto entitiesToSave = _collectLoadedEntitiesForSave();
        auto saveResult = entityStorage->saveAllEntities(entitiesToSave, m_config.dimension);
        if (saveResult.failed()) {
            return saveResult.error();
        }
    }

    // 再保存所有已加载方块实体，保证 /save-all 与关服路径覆盖运行时修改。
    auto* blockEntityStorage = m_storage->blockEntityStorage();
    if (blockEntityStorage) {
        auto blockEntitiesToSave = _collectLoadedBlockEntitiesForSave();
        auto saveResult = blockEntityStorage->saveAllBlockEntities(blockEntitiesToSave, m_config.dimension);
        if (saveResult.failed()) {
            return saveResult.error();
        }
    }

    // 保存末影龙战斗数据（仅末地维度）
    if (m_dragonFight && m_storage && m_storage->isOpen()) {
        auto dragonFightResult = m_storage->saveDragonFightData(m_dragonFight->saveData().toJson());
        if (dragonFightResult.failed()) {
            spdlog::warn("Failed to save dragon fight data: {}", dragonFightResult.error().message());
        }
    }

    auto result = m_storage->saveAll();
    if (result.failed()) {
        return result.error();
    }

    spdlog::info("Saved {} cached sections", result.value());
    return result.value();
}

void ServerWorld::setSharedStorage(world::storage::SingleLevelStorageManager* storage)
{
    MC_ASSERT_RELEASE(!m_initialized);
    m_storage = storage;
}

void ServerWorld::setConfig(const ServerWorldConfig& config)
{
    m_config = config;
    if (m_chunkManager) {
        m_chunkManager->setViewDistance(config.viewDistance);
    }
}

bool ServerWorld::isDebugWorld() const
{
    // 通过检查区块生成器类型来判断是否为调试世界
    if (!m_chunkManager) {
        return false;
    }
    const IChunkGenerator* generator = m_chunkManager->generator();
    return generator && generator->isDebugGenerator();
}

void ServerWorld::playSound(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    if (m_onPlaySound) {
        m_onPlaySound(soundEventId, category, position, volume, pitch);
    }
}

void ServerWorld::playEvent(i32 eventId, const BlockPos& pos, i32 data)
{
    if (m_onBroadcastWorldEvent) {
        m_onBroadcastWorldEvent(eventId, pos.x, pos.y, pos.z, data);
    }
}

void ServerWorld::destroyBlockProgress(EntityInstanceId breakerId, const BlockPos& pos, i32 progress)
{
    if (m_onDestroyBlockProgress) {
        m_onDestroyBlockProgress(breakerId, pos.x, pos.y, pos.z, progress);
    }
}

void ServerWorld::gameEvent(
    const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context)
{
    // 通过 GameEventDispatcher 将事件分发给附近的 GameEventListener
    // （如幽匿感测体 SculkSensor、幽匿尖啸体 SculkShrieker 等）。
    // 参考 MC: Level.gameEvent() -> GameEventDispatcher.post()
    if (m_gameEventDispatcher) {
        Vector3d eventPos(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
        m_gameEventDispatcher->post(event, eventPos, context);
    }
}

void ServerWorld::notifyBlockUpdate(const BlockPos& pos)
{
    // 即使方块状态未改变，也触发客户端同步通知
    // 参考 MC: Level.sendBlockUpdated(pos, oldState, newState, flags)
    const BlockState* state = getBlockState(pos);
    if (state != nullptr && m_onBlockChanged) {
        m_onBlockChanged(pos, state->stateId());
    }
}

// ============================================================================
// 方块事件
// ============================================================================

void ServerWorld::blockEvent(const BlockPos& pos, const Block& block, i32 paramA, i32 paramB)
{
    // 将方块事件加入队列，每tick处理时验证方块是否仍匹配并执行
    // 参考 MC Java: ServerLevel.blockEvent(BlockPos, Block, int, int)
    m_blockEvents.push_back(BlockEventData{pos, &block, paramA, paramB});
}

void ServerWorld::broadcastBlockEntity(const BlockPos& pos)
{
    // 方块实体数据变化后，触发回调将最新 NBT 快照发送给追踪该区块的客户端
    // 参考 MC Java: ServerLevel.sendBlockUpdated(BlockPos, BlockState, BlockState, int)
    if (m_onBroadcastBlockEntity) {
        m_onBroadcastBlockEntity(pos);
    }
}

void ServerWorld::runBlockEvents()
{
    // 参考 MC Java: ServerLevel.runBlockEvents()
    m_blockEventsToReschedule.clear();

    for (auto& event : m_blockEvents) {
        // 检查区块是否已加载并处于tick状态
        const ChunkCoord chunkX = world::toChunkCoord(event.pos.x);
        const ChunkCoord chunkZ = world::toChunkCoord(event.pos.z);
        if (hasChunk(chunkX, chunkZ)) {
            if (doBlockEvent(event)) {
                // 事件成功执行，广播给客户端
                if (m_onBroadcastBlockEvent) {
                    const BlockState* state = getBlockState(event.pos);
                    u32 blockStateId = state != nullptr ? state->stateId() : 0;
                    m_onBroadcastBlockEvent(event.pos.x,
                        event.pos.y,
                        event.pos.z,
                        static_cast<u8>(event.paramA),
                        static_cast<u8>(event.paramB),
                        blockStateId);
                }
            }
        } else {
            // 区块未加载，延迟到下tick处理
            m_blockEventsToReschedule.push_back(event);
        }
    }

    // 清空已处理的事件，将需要重新调度的事件加回队列
    m_blockEvents.clear();
    m_blockEvents.insert(m_blockEvents.end(), m_blockEventsToReschedule.begin(), m_blockEventsToReschedule.end());
    m_blockEventsToReschedule.clear();
}

bool ServerWorld::doBlockEvent(const BlockEventData& event)
{
    // 参考 MC Java: ServerLevel.doBlockEvent(BlockEventData)
    // 验证当前位置的方块是否仍然是触发事件时的方块类型
    const BlockState* state = getBlockState(event.pos);
    if (state == nullptr) {
        return false;
    }

    // 方块类型不匹配（被替换了），事件被丢弃
    if (&state->getBlock() != event.block) {
        return false;
    }

    // 调用 Block::triggerEvent()，默认实现委托给 BlockEntity::triggerEvent()
    return state->getBlock().triggerEvent(*state, *this, event.pos, event.paramA, event.paramB);
}

bool ServerWorld::openContainer(ContainerType type, const BlockPos& pos, Player& player)
{
    if (!m_onOpenContainer) {
        return false;
    }

    return m_onOpenContainer(type, pos, player);
}

bool ServerWorld::openEntityContainer(INamedContainerProvider& provider, Player& player)
{
    if (!m_onOpenEntityContainer) {
        return false;
    }

    return m_onOpenEntityContainer(provider, player);
}

void ServerWorld::setChunkManager(std::unique_ptr<ServerChunkManager> manager)
{
    m_chunkManager = std::move(manager);
    if (m_chunkManager) {
        // 替换区块管理器时同步当前世界视距，避免回落到默认值。
        m_chunkManager->setViewDistance(m_config.viewDistance);
    }
}

// ============================================================================
// 出生点管理
// ============================================================================

void ServerWorld::initializeWorldSpawn()
{
    spdlog::info("ServerWorld: Initializing world spawn point...");

    // 通过 Sampler 在气候空间径向搜索最佳出生区块，
    // 然后在出生区块中用 SpawnLocationHelper 查找有效出生位置。
    //
    // 仅 NoiseChunkGenerator 持有 RandomState（其 Sampler 内含 spawnTarget）。
    // 其他生成器（FlatChunkGenerator/DebugChunkGenerator）退回到 (0,0) 区块。
    ChunkPos spawnChunk(0, 0);

    auto* generator = m_chunkManager->generator();
    auto* noiseGenerator = dynamic_cast<NoiseChunkGenerator*>(generator);
    if (noiseGenerator != nullptr) {
        const auto& randomState = noiseGenerator->randomState();
        if (randomState != nullptr) {
            const auto& sampler = randomState->sampler();
            const auto& spawnTarget = sampler.spawnTarget();
            if (!spawnTarget.empty()) {
                // Climate-based 径向搜索：返回 (x, 0, z) 块坐标，转区块坐标
                const BlockPos climateSpawn = sampler.findSpawnPosition();
                spawnChunk = ChunkPos(climateSpawn);
                spdlog::info("ServerWorld: Climate-based spawn search returned block ({}, {}, {}), chunk ({}, {})",
                    climateSpawn.x,
                    climateSpawn.y,
                    climateSpawn.z,
                    spawnChunk.x,
                    spawnChunk.z);
            } else {
                spdlog::debug("ServerWorld: spawnTarget empty, falling back to (0,0) chunk");
            }
        }
    }

    // 直接使用 m_chunkManager->getChunkSync()，确保出生点区块已加载
    ChunkData* chunk = m_chunkManager->requestFullChunkSync(spawnChunk.x, spawnChunk.z);

    if (chunk == nullptr) {
        spdlog::error(
            "ServerWorld: Failed to load spawn chunk, using default spawn point (0, {}, 0)", world::SEA_LEVEL + 1);
        m_worldSpawnPoint = Vector3d(0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0);
        return;
    }

    // 使用 SpawnLocationHelper 在出生区块查找有效位置
    auto spawnPos = SpawnLocationHelper::findSpawnLocationInChunk(*this, spawnChunk, true);

    if (spawnPos.has_value()) {
        // 找到有效位置，设置到世界出生点
        m_worldSpawnPoint = Vector3d(spawnPos->x + 0.5,
            spawnPos->y + 1.0, // 站在方块上面
            spawnPos->z + 0.5);
        spdlog::info("ServerWorld: World spawn initialized at ({}, {}, {})", spawnPos->x, spawnPos->y + 1, spawnPos->z);
    } else {
        // 使用默认位置
        m_worldSpawnPoint = Vector3d(0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0);
        spdlog::error(
            "ServerWorld: No valid spawn found in spawn chunk, using default (0, {}, 0)", world::SEA_LEVEL + 1);
    }
}

void ServerWorld::applyLevelRuntimeData(const world::storage::LevelRuntimeData& runtimeData)
{
    if (m_timeManager != nullptr) {
        m_timeManager->setGameTime(runtimeData.gameTime);
        m_timeManager->setDayTime(runtimeData.dayTime);
    }

    // level.dat 中 SpawnY 语义为"脚下方块 Y"，玩家脚位置需 +1 站在方块上方，
    // 与 initializeWorldSpawn 的 (spawnPos.y + 1.0) 语义保持一致。
    m_worldSpawnPoint = Vector3d(static_cast<f64>(runtimeData.spawnX) + 0.5,
        static_cast<f64>(runtimeData.spawnY) + 1.0,
        static_cast<f64>(runtimeData.spawnZ) + 0.5);
    m_spawnAngle = runtimeData.spawnAngle;

    if (m_weatherManager == nullptr) {
        return;
    }

    m_weatherManager->setWeatherCycleEnabled(true);
    if (runtimeData.thundering) {
        m_weatherManager->setThunder(std::max(runtimeData.thunderTime, 1));
    } else if (runtimeData.raining) {
        m_weatherManager->setRain(std::max(runtimeData.rainTime, 1));
    } else {
        m_weatherManager->setClear(std::max(runtimeData.clearWeatherTime, 1));
    }
}

// ============================================================================
// 区块管理
// ============================================================================

// 注意：非const版本的 getChunk 提供可变访问，供需要修改区块的场景使用。
// const版本是 IWorld 接口实现。
ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z)
{
    return m_chunkManager->tryToGetChunkInMem(x, z);
}

const ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z) const
{
    return m_chunkManager->tryToGetChunkInMem(x, z);
}

bool ServerWorld::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    return m_chunkManager->hasChunkInMem(x, z);
}

const ChunkData* ServerWorld::getOrLoadChunk(ChunkCoord x, ChunkCoord z)
{
    // 同步加载区块：如果已加载则直接返回，否则在主线程上同步触发加载/生成
    return m_chunkManager->requestFullChunkSync(x, z);
}

// ============================================================================
// 方块操作
// ============================================================================

const BlockState* ServerWorld::getBlockState(i32 x, i32 y, i32 z) const
{
    ChunkCoord chunkX = CoordConverter::blockToChunk(x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(z);

    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) return nullptr;

    i32 localX = x - chunkX * world::CHUNK_WIDTH;
    i32 localZ = z - chunkZ * world::CHUNK_WIDTH;

    return chunk->getBlockState(localX, y, localZ);
}

// ============================================================================
// 修改世界中的方块
// 注意：会自动给客户端发包，不要在外部调用后再发一次！
// ============================================================================
bool ServerWorld::setBlockState(i32 x, i32 y, i32 z, const BlockState* state)
{
    const BlockPos changedPos(x, y, z);

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "ServerWorld::setBlockState",
        "x",
        x,
        "y",
        y,
        "z",
        z,
        "state",
        state != nullptr ? state->toString() : std::string("null"),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(x, y, z).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::DebugWorldCheck", "x", x, "y", y, "z", z);

        // 调试世界禁止方块修改
        if (isDebugWorld()) {
            return false;
        }
    }

    ChunkCoord chunkX = CoordConverter::blockToChunk(x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(z);
    ChunkData* chunk = nullptr;

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::ChunkLookup", "chunkX", chunkX, "chunkZ", chunkZ);

        chunk = m_chunkManager->getChunkSync(chunkX, chunkZ);
        if (!chunk) {
            return false;
        }
    }

    const BlockRegistry& blockRegistry = BlockRegistry::instance();
    const BlockState* airState = blockRegistry.airState();

    const auto canonicalizeState = [&](const BlockState* inputState) -> const BlockState* {
        if (inputState == nullptr) {
            return airState;
        }

        const BlockState* canonical = blockRegistry.getBlockState(inputState->stateId());
        if (canonical != nullptr) {
            return canonical;
        }

        if (inputState->isAir()) {
            return airState;
        }

        return inputState;
    };

    i32 localX = x - chunkX * world::CHUNK_WIDTH;
    i32 localZ = z - chunkZ * world::CHUNK_WIDTH;

    const BlockState* oldState = nullptr;
    const BlockState* newState = nullptr;

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::CanonicalizeState", "x", x, "y", y, "z", z);

        oldState = canonicalizeState(chunk->getBlockState(localX, y, localZ));
        newState = canonicalizeState(state);
        if (newState != nullptr && newState->isAir()) {
            newState = airState;
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::StateComparison", "x", x, "y", y, "z", z);

        if (oldState == newState) {
            return false;
        }
    }

    const bool oldIsAir = (oldState == nullptr || oldState->isAir());
    const bool newIsAir = (newState == nullptr || newState->isAir());
    const bool blockTypeChanged =
        (oldState == nullptr || newState == nullptr || oldState->blockId() != newState->blockId());

    i32 oldLightLevel = oldState ? oldState->lightLevel() : 0;
    i32 newLightLevel = newState ? newState->lightLevel() : 0;

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
            "ServerWorld::setBlockState::WriteChunk",
            "x",
            x,
            "y",
            y,
            "z",
            z,
            "oldBlockId",
            oldState ? oldState->blockId() : 0,
            "newBlockId",
            newState ? newState->blockId() : 0);

        const BlockState* storedState = newIsAir ? nullptr : newState;
        chunk->setBlockState(localX, y, localZ, storedState);
        chunk->setDirty(true);
        // 先写入区块，后续旧方块回调可能会在同一坐标上做二次替换。
    }

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::OldBlockCallbacks", "x", x, "y", y, "z", z);

        // 通知村庄管理器方块移除（如果旧方块存在且不是空气）
        if (m_villageManager && !oldIsAir) {
            m_villageManager->onBlockRemoved(changedPos);
        }

        if (!oldIsAir && blockTypeChanged) {
            Block& oldBlock = oldState->getBlockMutable();
            oldBlock.onBlockRemoved(*this, changedPos, *oldState);
        }
    }

    const BlockState* currentState = canonicalizeState(chunk->getBlockState(localX, y, localZ));
    if (currentState != newState) {
        return true;
    }

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::NewBlockCallbacks", "x", x, "y", y, "z", z);

        if (m_onBlockChanged) {
            m_onBlockChanged(changedPos, currentState ? currentState->stateId() : 0u);
        }

        if (!newIsAir && blockTypeChanged) {
            Block& newBlock = newState->getBlockMutable();
            newBlock.onBlockAdded(*this, changedPos, *newState);
        }

        // 新方块有方块实体时创建
        if (!newIsAir && newState->getBlock().hasBlockEntity()) {
            Block& newBlock = newState->getBlockMutable();

            // 检查旧方块是否要求保留方块实体（如铜箱子在氧化/涂蜡/除蜡/刮削时）
            // 参考: net.minecraft.world.level.block.state.BlockBehaviour#shouldChangedStateKeepBlockEntity
            std::unique_ptr<BlockEntity> migratedEntity;
            if (!oldIsAir && blockTypeChanged && oldState->getBlock().shouldChangedStateKeepBlockEntity(*oldState)) {
                // 取出旧方块实体（不销毁），用于迁移到新方块
                // 旧方块的 shouldChangedStateKeepBlockEntity 返回 true 表示新方块语义上兼容旧实体
                migratedEntity = chunk->removeBlockEntity(changedPos);
            }

            std::unique_ptr<BlockEntity> blockEntity;
            if (migratedEntity != nullptr) {
                // 直接复用旧方块实体（shouldChangedStateKeepBlockEntity 已保证语义兼容）
                blockEntity = std::move(migratedEntity);
            } else {
                blockEntity = newBlock.createBlockEntity(changedPos);
            }

            if (blockEntity != nullptr) {
                setBlockEntity(changedPos, blockEntity.release());
            }
        }

        // 通知村庄管理器方块放置（如果新方块存在且不是空气）
        if (m_villageManager && !newIsAir) {
            m_villageManager->onBlockPlaced(changedPos, newState->blockId());
        }
    }

    const BlockState* sourceState = (!newIsAir) ? newState : oldState;
    Block* sourceBlock = nullptr;
    if (sourceState != nullptr && !sourceState->isAir()) {
        sourceBlock = &sourceState->getBlockMutable();
    }

    struct NeighborDelta {
        i32 dx;
        i32 dy;
        i32 dz;
        Direction direction;
    };

    constexpr std::array<NeighborDelta, 6> NEIGHBOR_DELTAS = {{{-1, 0, 0, Direction::West},
        {1, 0, 0, Direction::East},
        {0, -1, 0, Direction::Down},
        {0, 1, 0, Direction::Up},
        {0, 0, -1, Direction::North},
        {0, 0, 1, Direction::South}}};

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::NeighborUpdates", "x", x, "y", y, "z", z);

        for (const auto& neighbor : NEIGHBOR_DELTAS) {
            const BlockPos neighborPos(x + neighbor.dx, y + neighbor.dy, z + neighbor.dz);
            const BlockState* neighborState =
                canonicalizeState(getBlockState(neighborPos.x, neighborPos.y, neighborPos.z));

            const BlockState* updatedState = nullptr;

            {
                MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
                    "ServerWorld::setBlockState::NeighborUpdatePostPlacement",
                    "x",
                    neighborPos.x,
                    "y",
                    neighborPos.y,
                    "z",
                    neighborPos.z);

                if (neighborState != nullptr && !neighborState->isAir() && newState != nullptr) {
                    Block& neighborBlock = neighborState->getBlockMutable();
                    const BlockState* stateBeforeUpdate = neighborState;
                    BlockState updatedStateValue = neighborBlock.updatePostPlacement(*neighborState,
                        Directions::opposite(neighbor.direction),
                        *newState,
                        *this,
                        neighborPos,
                        changedPos);

                    const BlockState* stateAfterUpdate = canonicalizeState(getBlockState(neighborPos));
                    if (stateAfterUpdate != stateBeforeUpdate) {
                        neighborState = stateAfterUpdate;
                    } else {
                        updatedState = blockRegistry.getBlockState(updatedStateValue.stateId());
                        if (updatedState == nullptr && updatedStateValue.isAir()) {
                            updatedState = airState;
                        }
                    }
                }
            }

            if (updatedState != nullptr && updatedState != neighborState) {
                setBlockState(neighborPos, updatedState);
                neighborState = canonicalizeState(getBlockState(neighborPos));
            }

            {
                MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
                    "ServerWorld::setBlockState::NeighborChanged",
                    "x",
                    neighborPos.x,
                    "y",
                    neighborPos.y,
                    "z",
                    neighborPos.z);

                if (sourceBlock != nullptr && neighborState != nullptr && !neighborState->isAir()) {
                    Block& neighborBlock = neighborState->getBlockMutable();
                    neighborBlock.neighborChanged(*this, neighborPos, *sourceBlock, changedPos, false);
                }
            }
        }
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
            "ServerWorld::setBlockState::LightUpdates",
            "x",
            x,
            "y",
            y,
            "z",
            z,
            "oldLightLevel",
            oldLightLevel,
            "newLightLevel",
            newLightLevel);

        if (m_lightManager) {
            // 运行时方块变更改为入队延迟处理：仅标记坐标脏，不立即传播。
            // 光照在 tick 时批量传播（同区块多变更合并去重，一次 setupCaches）。
            // 原同步路径 checkBlock + onBlockEmissionIncrease 已合并：
            // 批量 blocksChangedInChunk 内部会重新读取当前方块状态并自动
            // 处理发光等级，onBlockEmissionIncrease 仅是无用的二次 checkBlock。
            m_lightQueue.queueBlockChange(changedPos.x, changedPos.y, changedPos.z);
        } else {
            MC_ASSERT_RELEASE(false);
        }
    }

    // setBlockState 路径不会自动触发 LiquidBlock 回调，这里主动补一次流体初始调度。
    // 同时调度周围六邻域，确保水/岩浆在方块变化后能及时重算流动。
    const auto scheduleFluidAt = [&](const BlockPos& pos, const BlockState* blockState) {
        MC_ASSERT_RELEASE(blockState && m_tickManager);

        const fluid::FluidState* fluidState = blockState->getFluidState();
        if (fluidState == nullptr || fluidState->isEmpty()) {
            return;
        }

        const fluid::Fluid& fluid = fluidState->getFluid();
        m_tickManager->scheduleFluidTick(pos, fluid, fluid.getTickDelay(*this), world::tick::TickPriority::Normal);
    };

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.World, "ServerWorld::setBlockState::FluidScheduling", "x", x, "y", y, "z", z);

        scheduleFluidAt(changedPos, newState);

        constexpr std::array<std::array<i32, 3>, 6> NEIGHBOR_OFFSETS = {
            {{{-1, 0, 0}}, {{1, 0, 0}}, {{0, -1, 0}}, {{0, 1, 0}}, {{0, 0, -1}}, {{0, 0, 1}}}};

        for (const auto& offset : NEIGHBOR_OFFSETS) {
            const BlockPos neighborPos(x + offset[0], y + offset[1], z + offset[2]);
            const BlockState* neighborState =
                canonicalizeState(getBlockState(neighborPos.x, neighborPos.y, neighborPos.z));
            scheduleFluidAt(neighborPos, neighborState);
        }
    }

    return true;
}

// ============================================================================
// 方块实体管理
// ============================================================================

BlockEntity* ServerWorld::getBlockEntity(const BlockPos& pos)
{
    // 超出世界高度范围返回 nullptr
    if (!isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        return nullptr;
    }

    // 获取区块
    ChunkCoord chunkX = CoordConverter::blockToChunk(pos.x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(pos.z);
    ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return nullptr;
    }

    // ChunkData::getBlockEntity 接受世界坐标
    return chunk->getBlockEntity(pos);
}

const BlockEntity* ServerWorld::getBlockEntity(const BlockPos& pos) const
{
    // 超出世界高度范围返回 nullptr
    if (!isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        return nullptr;
    }

    // 获取区块
    ChunkCoord chunkX = CoordConverter::blockToChunk(pos.x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(pos.z);
    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return nullptr;
    }

    // ChunkData::getBlockEntity 接受世界坐标
    return chunk->getBlockEntity(pos);
}

void ServerWorld::setBlockEntity(const BlockPos& pos, BlockEntity* entity)
{
    if (entity == nullptr) {
        return;
    }

    // 超出世界高度范围不设置
    if (!isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        // 释放实体以避免内存泄漏
        delete entity;
        return;
    }

    // 获取区块
    ChunkCoord chunkX = CoordConverter::blockToChunk(pos.x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(pos.z);
    ChunkData* chunk = m_chunkManager->getChunkSync(chunkX, chunkZ);
    if (!chunk) {
        // 区块未加载，无法设置方块实体
        // 注意：如果区块未加载，方块实体会丢失
        delete entity;
        return;
    }

    // 设置方块实体的世界引用
    entity->setWorld(this);

    // 将原始指针包装为 unique_ptr 并调用 ChunkData::setBlockEntity
    // 注意：ChunkData::setBlockEntity 返回旧实体（如果有）
    // ChunkData::setBlockEntity 接受世界坐标
    std::unique_ptr<BlockEntity> entityPtr(entity);
    std::unique_ptr<BlockEntity> oldEntity = chunk->setBlockEntity(pos, std::move(entityPtr));

    // 如果有旧实体，它会被自动销毁
    // 标记区块为已修改
    chunk->setDirty(true);

    // 检测幽匿方块实体并注册振动监听器到 GameEventListenerRegistry
    if (entity->getType() == BlockEntityType::SculkSensor) {
        auto* sensor = dynamic_cast<blockentity::SculkSensorBlockEntity*>(entity);
        if (sensor != nullptr) {
            m_sculkVibrationManager.registerSculkSensor(*sensor);
        }
    } else if (entity->getType() == BlockEntityType::SculkShrieker) {
        auto* shrieker = dynamic_cast<blockentity::SculkShriekerBlockEntity*>(entity);
        if (shrieker != nullptr) {
            m_sculkVibrationManager.registerSculkShrieker(*shrieker);
        }
    }
}

void ServerWorld::removeBlockEntity(const BlockPos& pos)
{
    // 超出世界高度范围不处理
    if (!isWithinWorldBounds(pos.x, pos.y, pos.z)) {
        return;
    }

    // 获取区块
    ChunkCoord chunkX = CoordConverter::blockToChunk(pos.x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(pos.z);
    ChunkData* chunk = m_chunkManager->getChunkSync(chunkX, chunkZ);
    if (!chunk) {
        return;
    }

    // 移除方块实体
    // ChunkData::removeBlockEntity 接受世界坐标
    std::unique_ptr<BlockEntity> oldEntity = chunk->removeBlockEntity(pos);

    // 如果有旧实体，标记区块为已修改
    if (oldEntity) {
        chunk->setDirty(true);

        // 注销幽匿方块实体的振动监听器
        m_sculkVibrationManager.unregisterSculkBlockEntity(pos);
    }
}

// ============================================================================
// 更新循环
// 服务端世界
//
// 纯粹的世界数据容器，职责：
// - 区块管理
// - 光照计算
// - 物理模拟？
// - Tick 调度
// - 天气状态
// - 村庄和袭击管理
//
// 不负责：
// - 实体管理 (这个容易踩坑，我们认为实体不属于世界范畴，不交由世界管理)
// - 玩家管理（由 PlayerManager 管理）
// - 网络通信（由 ConnectionManager 管理）
// - 时间管理（由 TimeManager 管理）
// - 传送（由 TeleportManager 管理）
// - 游戏模式（由 GameModeManager 管理）
// ============================================================================
void ServerWorld::tick()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick");

    // 时间由外部 TimeManager 管理，不再自增 tick 计数

    // 区块 tick - 包括区块内实体、方块随机刻、区块状态更新等
    if (m_chunkManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::ChunkTick");
        m_chunkManager->tick();
    }

    // 光照更新 - 统一异步调度（③-2b）：
    //   1. flush 上一 tick worker 完成的 dirty section（主线程独占 markLightChanged，
    //      把 visible nibble 同步到 ChunkSection 供 send serialize 读）
    //   2. send 上一 tick worker 完成光照的区块（serialize 读已 flush nibble + removeLightTicket）
    //   3. 排空运行时方块变更延迟队列（提交新 worker 任务，writeRadius=2 区域互斥）
    // 顺序关键：flush→send→drain，保证 serialize 读到已 flush nibble。
    // 引擎已无 m_mutex——worker 写 updating 经区域锁串行（同 LIGHT 生成阶段、ChunkLoadLightTask）。
    if (m_lightManager) {
        _drainPendingLightFlushes();
        _drainPendingChunkSends();

        if (!m_lightQueue.empty()) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::LightQueueDrain");
            m_lightQueue.drainAndProcess(*this);
        }
    }

    // 获取当前 tick
    u64 currentTick = m_timeManager ? m_timeManager->currentTick() : 0;
    i64 gameTime = m_timeManager ? m_timeManager->dayTime() : 0;

    // 调试世界不执行计划刻
    if (!isDebugWorld() && m_tickManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::TickManager");
        m_tickManager->tick(currentTick);
    }

    // 方块实体tick - 熔炉、漏斗、刷怪笼等需要每tick更新的方块实体
    if (!isDebugWorld()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::BlockEntityTick");
        tickBlockEntities();
    }

    // 方块事件处理 - 箱子开合、活塞伸缩、音符盒播放等延迟事件
    if (!isDebugWorld()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::BlockEvents");
        runBlockEvents();
    }

    // 调试世界不执行随机刻
    if (!isDebugWorld()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::EnvironmentTick");
        // 从游戏规则获取随机刻速度
        i32 randomTickSpeed = m_gameRules.getInt(world::gamerule::GameRuleKeys::RANDOM_TICK_SPEED);
        tickEnvironment(randomTickSpeed);

        // 降水对方块的影响（结冰和降雪）
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::PrecipitationTick");
        tickPrecipitation(randomTickSpeed);
    }

    // 调试世界不执行红石清理
    if (!isDebugWorld()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::RedstoneTick");
        // 定期清理红石火把烧毁记录（每 200 tick）
        if (currentTick % 200 == 0) {
            world::redstone::RedstoneSystem::instance().cleanupBurnoutRecords(currentTick);
        }
    }

    // 调试世界不执行天气 tick
    if (!isDebugWorld() && m_weatherManager) {
        m_weatherManager->tick();

        // 雷暴时尝试生成闪电实体（trySpawnLightning 内部已含 isThundering/isRaining、概率、
        // canRainAt、canSeeSky 判定）
        auto [ok, pos] = m_weatherManager->trySpawnLightning();
        if (ok) {
            auto bolt = entity::LightningBoltEntity::create(this);
            bolt->setPosition(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
            EntityInstanceId boltId = spawnEntity(std::move(bolt));
            if (boltId == 0) {
                spdlog::warn("Failed to spawn lightning bolt at ({}, {}, {})", pos.x, pos.y, pos.z);
            }
        }
    }

    // 检查全员睡眠状态
    if (m_allPlayersSleeping) {
        checkSleepStatus();
    }

    // 更新村庄系统（流言衰减、边界重算等）
    if (m_villageManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::VillageTick");
        m_villageManager->tick(gameTime);
    }

    // 更新袭击系统（波次推进、掠夺者生成等）
    if (m_raidManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::RaidTick");
        m_raidManager->tick();
    }

    // 更新村庄围攻系统（僵尸围村）
    // 调试世界不执行村庄围攻
    if (!isDebugWorld()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::VillageSiege");
        m_villageSiege.tick(*this, true); // spawnHostiles = true
    }

    // 更新世界边界（渐变动画）
    m_worldBorder.tick();

    // 更新地图数据（持有地图的玩家位置追踪等）
    if (m_mapDataManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::MapDataTick");
        m_mapDataManager->tick(*this);
    }

    // 更新末影龙战斗状态（仅末地维度）
    if (m_dragonFight) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::DragonFightTick");
        m_dragonFight->tick(*this);
    }

    // EntityManager 由 MinecraftServer 驱动
    // EntityTracker 和 ItemPickupManager 由 MinecraftServer::tickEntities() 驱动
    m_entityManager.forEachEntity([this](Entity* entity) {
        if (entity == nullptr || entity->isRemoved()) {
            return true;
        }

        const ChunkCoord currentCx = CoordConverter::blockToChunk(entity->x());
        const ChunkCoord currentCz = CoordConverter::blockToChunk(entity->z());
        const auto trackedChunk = m_entityChunkTracker.getEntityChunk(entity->id());
        if (!trackedChunk.has_value()) {
            m_entityChunkTracker.onEntityAdded(entity->id(), currentCx, currentCz);
            return true;
        }

        const auto [oldCx, oldCz] = *trackedChunk;
        m_entityChunkTracker.onEntityMoved(entity->id(), oldCx, oldCz, currentCx, currentCz);
        return true;
    });
}

// ============================================================================
// 方块实体tick
// ============================================================================

void ServerWorld::tickBlockEntities()
{
    if (!m_chunkManager) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tickBlockEntities");

    // 遍历所有已加载区块，对需要tick的方块实体调用tick()
    // 快照方块实体列表以避免迭代期间修改导致的问题
    m_chunkManager->forEachLoadedChunk([this](ChunkData& chunk) {
        auto blockEntities = chunk.getAllBlockEntities();
        for (auto* blockEntity : blockEntities) {
            if (blockEntity != nullptr && !blockEntity->isRemoved() && blockEntity->needsTick()) {
                blockEntity->tick(*this);
            }
        }
        return true;
    });

    // 驱动所有已注册幽匿方块实体的振动系统 tick
    m_sculkVibrationManager.tickAll();
}

// ============================================================================
// 随机刻系统
// ============================================================================

void ServerWorld::tickEnvironment(i32 randomTickSpeed)
{
    if (randomTickSpeed <= 0 || !m_chunkManager) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tickEnvironment", "randomTickSpeed", randomTickSpeed);

    // 遍历所有已加载区块
    m_chunkManager->forEachLoadedChunk([this, randomTickSpeed](ChunkData& chunk) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "tickChunk", "x", chunk.x(), "z", chunk.z());

        // 获取区块起始坐标（方块坐标）
        i32 chunkX = chunk.x() * world::CHUNK_WIDTH;
        i32 chunkZ = chunk.z() * world::CHUNK_WIDTH;

        // 遍历区块中的每个段
        for (i32 sectionIndex = 0; sectionIndex < world::CHUNK_SECTIONS; ++sectionIndex) {
            const ChunkSection* section = chunk.getSection(sectionIndex);
            if (!section || !section->needsRandomTickAny()) {
                continue;
            }

            // 区块段的 Y 起始坐标
            i32 sectionY = sectionIndex * world::CHUNK_SECTION_HEIGHT + world::MIN_BUILD_HEIGHT;

            // 对每个 randomTickSpeed，选择一个随机位置执行 tick
            for (i32 i = 0; i < randomTickSpeed; ++i) {
                BlockPos pos = getBlockRandomPos(chunkX, sectionY, chunkZ);

                // 获取方块状态
                const BlockState* blockState = chunk.getBlockState(pos.x - chunkX, pos.y, pos.z - chunkZ);

                if (blockState) {
                    // 执行方块随机刻
                    if (blockState->getBlock().ticksRandomly()) {
                        Block& block = blockState->getBlockMutable();
                        block.randomTick(*this, pos, const_cast<BlockState&>(*blockState), m_random);

                        // 派发自定义方块组件回调 - onRandomTick
                        auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
                        std::string typeId = blockState->getBlock().blockLocation().toString();
                        if (blockCompReg.hasRandomTickCallback(typeId)) {
                            mc::mod::bedrock::addon::BlockComponentRandomTickEvent event;
                            event.blockTypeId = typeId;
                            event.blockX = pos.x;
                            event.blockY = pos.y;
                            event.blockZ = pos.z;
                            event.dimensionId = dimension();
                            blockCompReg.dispatchRandomTick(typeId, event);
                        }
                    }

                    // 执行流体随机刻
                    const fluid::FluidState* fluidState = blockState->getFluidState();
                    if (fluidState && !fluidState->isEmpty() && fluidState->getFluid().ticksRandomly()) {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast) — randomTick 是非 const 方法，需要
                        // const_cast
                        fluid::Fluid& fluid = const_cast<fluid::Fluid&>(fluidState->getFluid());
                        fluid.randomTick(*this, pos, *fluidState, m_random);
                    }
                }
            }
        }

        return true; // 继续遍历
    });
}

BlockPos ServerWorld::getBlockRandomPos(i32 chunkX, i32 sectionY, i32 chunkZ)
{
    // 使用 LCG (Linear Congruential Generator) 确保分布均匀
    m_updateLCG = m_updateLCG * 3 + 1013904223;
    i32 i = static_cast<i32>(m_updateLCG >> 2);

    // 计算 x, y, z 偏移
    // x = chunkX + (i & 15)          -> 范围 [0, 15]
    // y = sectionY + ((i >> 16) & 15) -> 范围 [0, 15]
    // z = chunkZ + ((i >> 8) & 15)   -> 范围 [0, 15]
    constexpr i32 SECTION_MASK = world::CHUNK_SECTION_HEIGHT - 1;

    return BlockPos(
        chunkX + (i & SECTION_MASK), sectionY + ((i >> 16) & SECTION_MASK), chunkZ + ((i >> 8) & SECTION_MASK));
}

void ServerWorld::tickPrecipitation(i32 randomTickSpeed)
{
    if (randomTickSpeed <= 0 || !m_chunkManager) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tickPrecipitation");

    const i32 maxSnowAccumulation = m_gameRules.getInt(world::gamerule::GameRuleKeys::MAX_SNOW_ACCUMULATION_HEIGHT);
    const bool isRaining = m_weatherManager && m_weatherManager->isRaining();

    m_chunkManager->forEachLoadedChunk([this, randomTickSpeed, maxSnowAccumulation, isRaining](ChunkData& chunk) {
        i32 chunkX = chunk.x() * world::CHUNK_WIDTH;
        i32 chunkZ = chunk.z() * world::CHUNK_WIDTH;

        for (i32 i = 0; i < randomTickSpeed; ++i) {
            // 每次迭代以 1/48 的概率触发降水 tick
            if (m_random.nextInt(48) != 0) {
                continue;
            }

            // 生成随机 XZ 位置，Y 坐标使用 MOTION_BLOCKING 高度图确定
            BlockPos randomPos = getBlockRandomPos(chunkX, 0, chunkZ);
            BlockCoord localX = randomPos.x - chunkX;
            BlockCoord localZ = randomPos.z - chunkZ;

            i32 topY = chunk.getTopBlockY(HeightmapType::MotionBlocking, localX, localZ);
            if (topY < world::MIN_BUILD_HEIGHT) {
                continue;
            }

            // topY 是最高运动阻挡方块的 Y 坐标（getTopBlockY 已从 getHeight 减 1）
            // 冰检查位置是 topY 本身（水面/地面），雪检查位置是 topY + 1（上方的空气）
            BlockPos surfacePos(randomPos.x, topY, randomPos.z);
            BlockPos aboveSurfacePos(randomPos.x, topY + 1, randomPos.z);

            // 获取生物群系
            BiomeId biomeId = chunk.getBiomeAtBlock(localX, topY, localZ);
            const world::biome::Biome& biome = world::biome::BiomeRegistry::instance().get(biomeId);

            // === 冰形成 ===
            // 冰形成不受天气状态影响，低温即可结冰
            if (biome.shouldFreeze(*this, surfacePos.x, surfacePos.y, surfacePos.z, world::SEA_LEVEL, true)) {
                const BlockState* iceState = VanillaBlocks::getState(VanillaBlocks::ICE);
                if (iceState) {
                    setBlockState(surfacePos.x, surfacePos.y, surfacePos.z, iceState, 3);
                }
            }

            // === 降雪 ===
            // 降雪仅在下雪时执行
            if (isRaining && maxSnowAccumulation > 0) {
                if (biome.shouldSnow(
                        *this, aboveSurfacePos.x, aboveSurfacePos.y, aboveSurfacePos.z, world::SEA_LEVEL)) {
                    const BlockState* currentBlock =
                        getBlockState(aboveSurfacePos.x, aboveSurfacePos.y, aboveSurfacePos.z);
                    if (currentBlock == nullptr) {
                        continue;
                    }

                    if (currentBlock->is(VanillaBlocks::SNOW)) {
                        // 已有雪层：尝试增加层数
                        i32 layers = currentBlock->get(blocks::SnowBlock::LAYERS());
                        i32 maxLayers = std::min(maxSnowAccumulation, 8);
                        if (layers < maxLayers) {
                            const BlockState* newState = &currentBlock->with(blocks::SnowBlock::LAYERS(), layers + 1);
                            if (newState) {
                                // 使用 pushEntitiesUp 将嵌入方块的实体向上推出
                                Block::pushEntitiesUp(*currentBlock, *newState, *this, aboveSurfacePos);
                                setBlockState(aboveSurfacePos.x, aboveSurfacePos.y, aboveSurfacePos.z, newState, 3);
                            }
                        }
                    } else if (currentBlock->isAir()) {
                        // 空气：放置新的雪层
                        const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
                        if (snowState) {
                            setBlockState(aboveSurfacePos.x, aboveSurfacePos.y, aboveSurfacePos.z, snowState, 3);

                            // 更新下方方块的 SNOWY 属性（如草方块、菌丝等）
                            const BlockState* belowBlock = getBlockState(surfacePos.x, surfacePos.y, surfacePos.z);
                            if (belowBlock && belowBlock->hasProperty(BlockStateProperties::SNOWY())) {
                                const BlockState* snowyState = &belowBlock->with(BlockStateProperties::SNOWY(), true);
                                if (snowyState) {
                                    setBlockState(surfacePos.x, surfacePos.y, surfacePos.z, snowyState, 3);
                                }
                            }
                        }
                    }
                }
            }

            // === 降水方块处理（炼药锅填充、避雷针激活等）===
            // MC Java 在 tickIceAndSnow 中对每个降水位置调用 block.handlePrecipitation(state, level, pos,
            // precipitation)。仅在世界正在下雨时执行，且降水类型不为 None 时才调用。
            if (isRaining) {
                auto precipitation =
                    biome.getPrecipitationAt(surfacePos.x, surfacePos.y, surfacePos.z, world::SEA_LEVEL);
                if (precipitation != BiomeClimate::Precipitation::None) {
                    const BlockState* surfaceState = getBlockState(surfacePos.x, surfacePos.y, surfacePos.z);
                    if (surfaceState != nullptr) {
                        Block& block = surfaceState->getBlockMutable();
                        block.handlePrecipitation(*this, surfacePos, precipitation);
                    }
                }
            }
        }

        return true; // 继续遍历
    });
}

size_t ServerWorld::chunkCount() const
{
    if (m_chunkManager) {
        return m_chunkManager->singleChunkLifecycleManagerCount();
    }
    return 0;
}

size_t ServerWorld::loadedChunkCount() const
{
    if (m_chunkManager) {
        return m_chunkManager->loadedChunkCount();
    }
    return 0;
}

// ============================================================================
// 时间管理（委托给外部 TimeManager）
// ============================================================================

u64 ServerWorld::currentTick() const
{
    return m_timeManager ? m_timeManager->currentTick() : 0;
}

i64 ServerWorld::dayTime() const
{
    // 返回累积的日光时间（无边界）
    // 主世界使用 TimeManager 的时间
    return m_timeManager ? m_timeManager->dayTime() : 0;
}

i64 ServerWorld::dayTimeOfDay() const
{
    // 检查维度是否有固定时间（下界=18000午夜，末地=6000正午）
    DimensionType dimType = getDimensionType();
    if (dimType.hasFixedTime()) {
        auto fixedTime = dimType.fixedTimeValue();
        if (fixedTime.has_value()) {
            return fixedTime.value();
        }
    }
    // 主世界使用 TimeManager 的 dayTimeOfDay
    return m_timeManager ? m_timeManager->dayTimeOfDay() : 0;
}

// ============================================================================
// IWorld 接口实现
// ============================================================================

const fluid::FluidState* ServerWorld::getFluidState(i32 x, i32 y, i32 z) const
{
    const BlockState* blockState = getBlockState(x, y, z);
    if (blockState == nullptr) {
        return fluid::Fluid::getFluidState(0);
    }
    return blockState->getFluidState();
}

bool ServerWorld::isWithinWorldBounds(i32, i32 y, i32) const
{
    return y >= getMinBuildHeight() && y < getMaxBuildHeight();
}

bool ServerWorld::isBlockInLine(
    const Vector3d& from, const Vector3d& to, std::function<bool(const BlockState&)> predicate) const
{
    // 使用 DDA 算法沿 from -> to 逐格遍历，检查每个方块是否匹配谓词。
    // 与 raycastBlocks 不同，此方法不做碰撞箱精确检测，仅检查方块状态的谓词。

    if (!predicate) {
        return false;
    }

    // 计算起点和终点的方块坐标
    i32 currentX = static_cast<i32>(std::floor(from.x));
    i32 currentY = static_cast<i32>(std::floor(from.y));
    i32 currentZ = static_cast<i32>(std::floor(from.z));

    const i32 endX = static_cast<i32>(std::floor(to.x));
    const i32 endY = static_cast<i32>(std::floor(to.y));
    const i32 endZ = static_cast<i32>(std::floor(to.z));

    // 起点等于终点：仅检查一个方块
    if (currentX == endX && currentY == endY && currentZ == endZ) {
        if (isWithinWorldBounds(currentX, currentY, currentZ)) {
            const BlockState* state = getBlockState(currentX, currentY, currentZ);
            if (state != nullptr && predicate(*state)) {
                return true;
            }
        }
        return false;
    }

    // DDA 步进方向
    const i32 stepX = (to.x > from.x) ? 1 : (to.x < from.x) ? -1 : 0;
    const i32 stepY = (to.y > from.y) ? 1 : (to.y < from.y) ? -1 : 0;
    const i32 stepZ = (to.z > from.z) ? 1 : (to.z < from.z) ? -1 : 0;

    // 计算到下一个边界的 t 值
    const f64 dx = to.x - from.x;
    const f64 dy = to.y - from.y;
    const f64 dz = to.z - from.z;

    const f64 tDeltaX = (dx != 0.0) ? std::abs(1.0 / dx) : 1e30;
    const f64 tDeltaY = (dy != 0.0) ? std::abs(1.0 / dy) : 1e30;
    const f64 tDeltaZ = (dz != 0.0) ? std::abs(1.0 / dz) : 1e30;

    f64 tMaxX = (stepX > 0) ? ((static_cast<f64>(currentX + 1) - from.x) / dx)
        : (stepX < 0)       ? ((static_cast<f64>(currentX) - from.x) / dx)
                            : 1e30;
    f64 tMaxY = (stepY > 0) ? ((static_cast<f64>(currentY + 1) - from.y) / dy)
        : (stepY < 0)       ? ((static_cast<f64>(currentY) - from.y) / dy)
                            : 1e30;
    f64 tMaxZ = (stepZ > 0) ? ((static_cast<f64>(currentZ + 1) - from.z) / dz)
        : (stepZ < 0)       ? ((static_cast<f64>(currentZ) - from.z) / dz)
                            : 1e30;

    // 最大遍历步数，避免无限循环
    constexpr i32 MAX_STEPS = 1024;

    for (i32 step = 0; step < MAX_STEPS; ++step) {
        // 检查当前方块
        if (isWithinWorldBounds(currentX, currentY, currentZ)) {
            const BlockState* state = getBlockState(currentX, currentY, currentZ);
            if (state != nullptr && predicate(*state)) {
                return true;
            }
        }

        // 到达终点方块后停止
        if (currentX == endX && currentY == endY && currentZ == endZ) {
            break;
        }

        // DDA 步进：选择 t 值最小的轴
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                if (tMaxX > 1.0) break;
                currentX += stepX;
                tMaxX += tDeltaX;
            } else {
                if (tMaxZ > 1.0) break;
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                if (tMaxY > 1.0) break;
                currentY += stepY;
                tMaxY += tDeltaY;
            } else {
                if (tMaxZ > 1.0) break;
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
    }

    return false;
}

i32 ServerWorld::getHeight(i32 x, i32 z) const
{
    const ChunkCoord chunkX = CoordConverter::blockToChunk(x);
    const ChunkCoord chunkZ = CoordConverter::blockToChunk(z);

    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        // 区块未加载时返回海平面附近，避免调用方得到无意义常量值。
        return world::SEA_LEVEL + 1;
    }

    const i32 localX = x - chunkX * 16;
    const i32 localZ = z - chunkZ * 16;

    // 优先使用世界生成高度图；Chunk 侧公开的是最高方块 Y，这里要转换成空气层 Y。
    i32 height = chunk->getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;
    if (height <= world::MIN_BUILD_HEIGHT) {
        // 回退到基础高度图（m_heightMap 存储的是最高实心方块 Y，需要 +1 对齐语义）。
        const i32 highestSolidY = chunk->getHighestBlock(localX, localZ);
        height = highestSolidY + 1;
    }

    return std::clamp(height, world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT);
}

u8 ServerWorld::getBlockLight(i32 x, i32 y, i32 z) const
{
    if (m_lightManager) {
        return m_lightManager->getBlockLight(x, y, z);
    }
    return 0;
}

u8 ServerWorld::getSkyLight(i32 x, i32 y, i32 z) const
{
    if (m_lightManager) {
        return m_lightManager->getSkyLight(x, y, z);
    }
    return 15;
}

bool ServerWorld::isRaining() const
{
    return m_weatherManager ? m_weatherManager->isRaining() : false;
}

bool ServerWorld::isThundering() const
{
    return m_weatherManager ? m_weatherManager->isThundering() : false;
}

f32 ServerWorld::rainStrength(f32 partialTick) const
{
    return m_weatherManager ? m_weatherManager->rainStrength(partialTick) : 0.0f;
}

f32 ServerWorld::thunderStrength(f32 partialTick) const
{
    return m_weatherManager ? m_weatherManager->thunderStrength(partialTick) : 0.0f;
}

bool ServerWorld::canRainAt(const BlockPos& pos) const
{
    return m_weatherManager ? mc::weather::WeatherUtils::canRainAt(*this, pos) : false;
}

// ============================================================================
// 碰撞检测
// ============================================================================

bool ServerWorld::hasBlockCollision(const AxisAlignedBB& box) const
{
    ChunkCoord minChunkX = CoordConverter::blockToChunk(box.minX);
    ChunkCoord maxChunkX = CoordConverter::blockToChunk(box.maxX);
    ChunkCoord minChunkZ = CoordConverter::blockToChunk(box.minZ);
    ChunkCoord maxChunkZ = CoordConverter::blockToChunk(box.maxZ);

    for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
            const ChunkData* chunk = getChunk(cx, cz);
            if (!chunk) continue;

            i32 minY = std::max(world::MIN_BUILD_HEIGHT, static_cast<i32>(std::floor(box.minY)));
            i32 maxY = std::min(world::MAX_BUILD_HEIGHT - 1, static_cast<i32>(std::ceil(box.maxY)));

            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                        i32 wx = cx * world::CHUNK_WIDTH + x;
                        i32 wz = cz * world::CHUNK_WIDTH + z;

                        if (wx + 1 < box.minX || wx > box.maxX || wz + 1 < box.minZ || wz > box.maxZ) {
                            continue;
                        }

                        const BlockState* state = chunk->getBlockState(x, y, z);
                        if (!state || state->isAir()) continue;

                        const CollisionShape& shape = state->getCollisionShape();
                        if (shape.isEmpty()) continue;

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

std::vector<AxisAlignedBB> ServerWorld::getBlockCollisions(const AxisAlignedBB& box) const
{
    std::vector<AxisAlignedBB> collisions;

    ChunkCoord minChunkX = CoordConverter::blockToChunk(box.minX);
    ChunkCoord maxChunkX = CoordConverter::blockToChunk(box.maxX);
    ChunkCoord minChunkZ = CoordConverter::blockToChunk(box.minZ);
    ChunkCoord maxChunkZ = CoordConverter::blockToChunk(box.maxZ);

    for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
            const ChunkData* chunk = getChunk(cx, cz);
            if (!chunk) continue;

            i32 minY = std::max(world::MIN_BUILD_HEIGHT, static_cast<i32>(std::floor(box.minY)));
            i32 maxY = std::min(world::MAX_BUILD_HEIGHT - 1, static_cast<i32>(std::ceil(box.maxY)));

            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                        i32 wx = cx * world::CHUNK_WIDTH + x;
                        i32 wz = cz * world::CHUNK_WIDTH + z;

                        if (wx + 1 < box.minX || wx > box.maxX || wz + 1 < box.minZ || wz > box.maxZ) {
                            continue;
                        }

                        const BlockState* state = chunk->getBlockState(x, y, z);
                        if (!state || state->isAir()) continue;

                        const CollisionShape& shape = state->getCollisionShape();
                        if (shape.isEmpty()) continue;

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

bool ServerWorld::hasEntityCollision(const AxisAlignedBB& box, const Entity* except) const
{
    auto entities = m_entityManager.getEntitiesInAABB(box, except);
    return !entities.empty();
}

std::vector<AxisAlignedBB> ServerWorld::getEntityCollisions(const AxisAlignedBB& box, const Entity* except) const
{
    std::vector<AxisAlignedBB> collisions;
    auto entities = m_entityManager.getEntitiesInAABB(box, except);
    collisions.reserve(entities.size());

    for (const Entity* entity : entities) {
        collisions.push_back(entity->boundingBox());
    }

    return collisions;
}

// IWorld 接口实现：委托给 EntityManager
std::vector<Entity*> ServerWorld::getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const
{
    return m_entityManager.getEntitiesInAABB(box, except);
}

std::vector<Entity*> ServerWorld::getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const
{
    return m_entityManager.getEntitiesInRange(pos, range, except);
}

std::vector<Entity*> ServerWorld::getPlayers() const
{
    return m_entityManager.getPlayers();
}

std::vector<Entity*> ServerWorld::getEntitiesByType(const std::string& typeId) const
{
    return m_entityManager.getEntitiesByType(typeId);
}

// ============================================================================
// 最近玩家查询
// ============================================================================

Player* ServerWorld::getClosestPlayer(const Vector3& pos, f32 maxDistance)
{
    return const_cast<Player*>(static_cast<const ServerWorld*>(this)->getClosestPlayer(pos, maxDistance));
}

const Player* ServerWorld::getClosestPlayer(const Vector3& pos, f32 maxDistance) const
{
    return getClosestPlayer(pos, maxDistance, nullptr);
}

Player* ServerWorld::getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude)
{
    return const_cast<Player*>(static_cast<const ServerWorld*>(this)->getClosestPlayer(pos, maxDistance, exclude));
}

const Player* ServerWorld::getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude) const
{
    const Player* closestPlayer = nullptr;
    f64 closestDistSq = std::numeric_limits<f64>::max();
    f64 maxDistSq =
        (maxDistance < 0.0f) ? std::numeric_limits<f64>::max() : static_cast<f64>(maxDistance) * maxDistance;

    auto players = m_entityManager.getPlayers();
    for (const Entity* entity : players) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 排除指定实体
        if (exclude && entity == exclude) {
            continue;
        }

        // 只处理玩家实体
        const Player* player = dynamic_cast<const Player*>(entity);
        if (!player) {
            continue;
        }

        // 观察者模式的玩家不计入
        if (player->isSpectator()) {
            continue;
        }

        Vector3 playerPos = player->position();
        f64 dx = playerPos.x - pos.x;
        f64 dy = playerPos.y - pos.y;
        f64 dz = playerPos.z - pos.z;
        f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDistSq && distSq <= maxDistSq) {
            closestDistSq = distSq;
            closestPlayer = player;
        }
    }

    return closestPlayer;
}

f64 ServerWorld::getClosestPlayerDistanceSq(const Vector3& pos) const
{
    f64 closestDistSq = std::numeric_limits<f64>::max();

    auto players = m_entityManager.getPlayers();
    for (const Entity* entity : players) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 观察者模式的玩家不计入距离检查
        if (const Player* player = dynamic_cast<const Player*>(entity)) {
            if (player->isSpectator()) {
                continue;
            }
        }

        Vector3 playerPos = entity->position();
        f64 dx = playerPos.x - pos.x;
        f64 dy = playerPos.y - pos.y;
        f64 dz = playerPos.z - pos.z;
        f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDistSq) {
            closestDistSq = distSq;
        }
    }

    return closestDistSq;
}

// ============================================================================
// 实体管理
// ============================================================================

EntityInstanceId ServerWorld::spawnEntity(std::unique_ptr<Entity> entity)
{
    if (!entity) {
        return 0;
    }

    entity->setWorld(this);
    EntityInstanceId id = m_entityManager.addEntity(std::move(entity));

    Entity* addedEntity = m_entityManager.getEntity(id);
    if (addedEntity) {
        m_entityTracker.trackEntity(addedEntity);
        // 注册到区块跟踪器
        ChunkCoord cx = CoordConverter::blockToChunk(addedEntity->x());
        ChunkCoord cz = CoordConverter::blockToChunk(addedEntity->z());
        m_entityChunkTracker.onEntityAdded(id, cx, cz);
    } else {
        // 理论上不应该发生，addEntity 成功后应该能通过 getEntity 获取到实体。这里做个断言以便排查潜在问题。
        MC_ASSERT_RELEASE(false);
    }

    return id;
}

// 注意：此方法不仅移除实体，还负责取消追踪和区块归属注销。
// 调用者应使用此方法而非直接调用 entityManager().removeEntity()，
// 以确保实体追踪器和区块跟踪器状态正确更新。
std::unique_ptr<Entity> ServerWorld::removeEntity(EntityInstanceId id)
{
    // 先向追踪玩家发送 destroy 包并取消追踪：必须在 EntityManager 移除实体之前完成，
    // 否则客户端缓存的旧 ClientEntity（typeId 不可变、网格按 ID 缓存）可能残留，
    // 在后续同 ID 实体生成时被错误复用渲染（虽然 ID 已不复用，主动发包消除时序窗口）。
    if (m_server) {
        m_entityTracker.untrackEntity(*m_server, id);
    } else {
        m_entityTracker.untrackEntity(id);
    }

    auto entity = m_entityManager.removeEntity(id);
    if (entity) {
        // 从区块跟踪器中移除
        m_entityChunkTracker.onEntityRemoved(id);
    } else {
        spdlog::error("Attempted to remove non-existent entity with ID {}", id);
    }
    return entity;
}

// IWorld 接口实现：委托给 EntityManager
Entity* ServerWorld::getEntity(EntityInstanceId id)
{
    return m_entityManager.getEntity(id);
}

const Entity* ServerWorld::getEntity(EntityInstanceId id) const
{
    return m_entityManager.getEntity(id);
}

Entity* ServerWorld::getEntityByUuid(const std::string& uuid)
{
    return m_entityManager.getEntityByUuid(uuid);
}

const Entity* ServerWorld::getEntityByUuid(const std::string& uuid) const
{
    return m_entityManager.getEntityByUuid(uuid);
}

i32 ServerWorld::spawnEntitiesFromChunkGeneration(const std::vector<SpawnedEntityData>& entities)
{
    if (entities.empty()) {
        return 0;
    }

    i32 spawnedCount = 0;
    auto& registry = entity::EntityRegistry::instance();

    for (const auto& entityData : entities) {
        const entity::EntityType* entityType = registry.getType(entityData.entityTypeId);
        if (!entityType) {
            continue;
        }

        if (!entityType->canSummon()) {
            continue;
        }

        std::unique_ptr<Entity> entity = entityType->create(this);
        if (!entity) {
            continue;
        }

        entity->setWorld(this);
        entity->setPosition(Vector3(entityData.x, entityData.y, entityData.z));

        // 实例级生成规则检查（对应 MC PathfinderMob.checkSpawnRules）
        // 在实体创建后、finalizeSpawn之前，检查该位置是否适合该实体的寻路偏好
        auto* creatureEntity = dynamic_cast<CreatureEntity*>(entity.get());
        if (creatureEntity != nullptr) {
            if (!creatureEntity->canSpawnAt(entityData.x, entityData.y, entityData.z)) {
                continue;
            }
        }

        // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
        auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
        if (mobEntity != nullptr) {
            entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*this,
                BlockPos(static_cast<i32>(std::floor(entityData.x)),
                    static_cast<i32>(entityData.y),
                    static_cast<i32>(std::floor(entityData.z))));
            mobEntity->finalizeSpawn(*this, difficultyInstance, world::spawn::SpawnReason::ChunkGeneration);
        }

        EntityInstanceId entityId = m_entityManager.addEntity(std::move(entity));
        if (entityId != 0) {
            Entity* addedEntity = m_entityManager.getEntity(entityId);
            if (addedEntity) {
                m_entityTracker.trackEntity(addedEntity);
                // 注册到区块跟踪器
                ChunkCoord cx = CoordConverter::blockToChunk(addedEntity->x());
                ChunkCoord cz = CoordConverter::blockToChunk(addedEntity->z());
                m_entityChunkTracker.onEntityAdded(entityId, cx, cz);
            }
            ++spawnedCount;
        }
    }

    return spawnedCount;
}

// ============================================================================
// 实体区块持久化
// ============================================================================

void ServerWorld::onChunkLoaded(ChunkCoord x, ChunkCoord z)
{
    ChunkData* chunk = m_chunkManager ? m_chunkManager->tryToGetChunkInMem(x, z) : nullptr;
    if (chunk != nullptr && chunk->hasLoadedEntities()) {
        auto loadedEntities = chunk->takeLoadedEntities();
        for (auto& entityPtr : loadedEntities) {
            if (!entityPtr) {
                continue;
            }

            EntityInstanceId id = spawnEntity(std::move(entityPtr));
            if (id == 0) {
                spdlog::warn("Failed to spawn chunk-loaded entity for chunk ({}, {})", x, z);
                continue;
            }

            // 主实体已 spawn 拿到真实 id，挂载反序列化阶段暂存的 Passengers。
            // 必须在 spawn 之后调用：乘客 startRiding 时会把 m_vehicle 记为 vehicle.id()，
            // 若在 spawn 之前调用，vehicle.id() 仍为 0，会导致乘客 m_vehicle 失效。
            Entity* spawnedEntity = m_entityManager.getEntity(id);
            if (spawnedEntity != nullptr) {
                auto attachResult = entity::serialization::EntityDeserializer::attachPassengers(*spawnedEntity, *this);
                if (attachResult.failed()) {
                    spdlog::warn("Failed to attach passengers for entity {} in chunk ({}, {}): {}",
                        spawnedEntity->getTypeId(),
                        x,
                        z,
                        attachResult.error().message());
                }
            }
        }
    }

    // Native 路径：从 EntityStorageManager 加载区块内所有实体并注入世界
    if (!m_storage || !m_storage->isOpen()) {
        return;
    }

    auto* entityStorage = m_storage->entityStorage();
    if (!entityStorage) {
        return;
    }

    auto result = entityStorage->loadEntitiesInChunk(x, z, m_config.dimension);
    if (result.failed()) {
        spdlog::error("Failed to load entities for chunk ({}, {}): {}", x, z, result.error().message());
        return;
    }

    auto& entities = result.value();
    for (auto& entityPtr : entities) {
        if (!entityPtr) {
            continue;
        }

        // 计算实体所在区块坐标，确保与加载的区块一致
        ChunkCoord entityCx = CoordConverter::blockToChunk(entityPtr->x());
        ChunkCoord entityCz = CoordConverter::blockToChunk(entityPtr->z());

        EntityInstanceId id = spawnEntity(std::move(entityPtr));
        if (id == 0) {
            spdlog::warn("Failed to spawn storage-loaded entity for chunk ({}, {})", x, z);
            continue;
        }

        // 主实体已 spawn 拿到真实 id，挂载反序列化阶段暂存的 Passengers。
        // 必须在 spawn 之后调用：乘客 startRiding 时会把 m_vehicle 记为 vehicle.id()，
        // 若在 spawn 之前调用，vehicle.id() 仍为 0，会导致乘客 m_vehicle 失效。
        Entity* spawnedEntity = m_entityManager.getEntity(id);
        if (spawnedEntity != nullptr) {
            auto attachResult = entity::serialization::EntityDeserializer::attachPassengers(*spawnedEntity, *this);
            if (attachResult.failed()) {
                spdlog::warn("Failed to attach passengers for entity {} in chunk ({}, {}): {}",
                    spawnedEntity->getTypeId(),
                    x,
                    z,
                    attachResult.error().message());
            }
        }

        if (entityCx != x || entityCz != z) {
            m_entityChunkTracker.onEntityRemoved(id);
            m_entityChunkTracker.onEntityAdded(id, entityCx, entityCz);
        }
    }
}

void ServerWorld::onChunkUnloading(ChunkCoord x, ChunkCoord z)
{
    // 保存区块内所有实体到 EntityStorageManager，然后从 EntityManager 移除
    if (!m_storage || !m_storage->isOpen()) {
        // 存储不可用时，仅移除实体（不保存）
        auto entityIds = m_entityChunkTracker.getEntitiesInChunk(x, z);
        for (EntityInstanceId id : entityIds) {
            removeEntity(id);
        }
        return;
    }

    auto* entityStorage = m_storage->entityStorage();
    if (!entityStorage) {
        auto entityIds = m_entityChunkTracker.getEntitiesInChunk(x, z);
        for (EntityInstanceId id : entityIds) {
            removeEntity(id);
        }
        return;
    }

    auto entityIds = m_entityChunkTracker.getEntitiesInChunk(x, z);
    if (entityIds.empty()) {
        return;
    }

    auto deleteResult = entityStorage->deleteEntitiesInChunk(x, z, m_config.dimension);
    if (deleteResult.failed()) {
        spdlog::error(
            "Failed to clear old entity records for chunk ({}, {}): {}", x, z, deleteResult.error().message());
    }

    // 收集实体引用用于批量保存
    std::vector<std::reference_wrapper<Entity>> entitiesToSave;
    entitiesToSave.reserve(entityIds.size());

    for (EntityInstanceId id : entityIds) {
        Entity* entity = m_entityManager.getEntity(id);
        if (entity) {
            entitiesToSave.emplace_back(*entity);
        }
    }

    // 批量保存
    if (!entitiesToSave.empty()) {
        auto saveResult = entityStorage->saveEntitiesInChunk(entitiesToSave, x, z, m_config.dimension);
        if (saveResult.failed()) {
            spdlog::error("Failed to save entities for chunk ({}, {}): {}", x, z, saveResult.error().message());
        }
    }

    // 从世界移除实体（先发 destroy 包并取消追踪，再从 EntityManager 移除并注销区块归属）
    for (EntityInstanceId id : entityIds) {
        // 向追踪玩家发送 destroy 包并取消追踪（必须在实体被移除前完成）
        if (m_server) {
            m_entityTracker.untrackEntity(*m_server, id);
        } else {
            m_entityTracker.untrackEntity(id);
        }

        m_entityChunkTracker.onEntityRemoved(id);
        m_entityManager.removeEntity(id);
    }
}

// ============================================================================
// StarLightLightingProvider 接口实现
// ============================================================================

IChunk* ServerWorld::getChunkForLight(ChunkCoord x, ChunkCoord z)
{
    return m_chunkManager ? m_chunkManager->tryToGetChunkInMem(x, z) : nullptr;
}

const IChunk* ServerWorld::getChunkForLight(ChunkCoord x, ChunkCoord z) const
{
    return m_chunkManager ? m_chunkManager->tryToGetChunkInMem(x, z) : nullptr;
}

const BlockState* ServerWorld::getBlockStateForLight(const BlockPos& pos) const
{
    return getBlockState(pos);
}

IWorld* ServerWorld::getWorld() noexcept
{
    return this;
}

const IWorld* ServerWorld::getWorld() const noexcept
{
    return this;
}

void ServerWorld::markLightChanged(LightType type, const SectionPos& pos)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "ServerWorld::markLightChanged",
        "Type",
        (type == LightType::SKY) ? "Sky" : "Block",
        "Section",
        fmt::format("({}, {}, {})", pos.x, pos.y, pos.z));

    if (m_chunkManager) {
        ChunkData* chunk = m_chunkManager->tryToGetChunkInMem(pos.x, pos.z);
        if (chunk) {
            chunk->setDirty(true);
        }
    }

    _syncLightDataToChunk(type, pos);

    if (m_onLightChanged) {
        m_onLightChanged(type, pos);
    }
}

void ServerWorld::_enqueueLightFlush(std::vector<std::pair<LightType, SectionPos>> dirtySections)
{
    // worker 线程调用：入队 dirty section，主线程 tick 统一 flush。
    // 不去重——主线程 markLightChanged 幂等（setDirty/_syncLightDataToChunk/网络包均重复安全）。
    std::lock_guard<std::mutex> lock(m_pendingLightFlushesMutex);
    m_pendingLightFlushes.insert(m_pendingLightFlushes.end(),
        std::make_move_iterator(dirtySections.begin()),
        std::make_move_iterator(dirtySections.end()));
}

void ServerWorld::_drainPendingLightFlushes()
{
    // 主线程调用：swap 出队列后逐项调真正的 markLightChanged（主线程独占，安全）。
    // swap 在锁内最小化临界区；markLightChanged 在锁外执行避免与 _enqueueLightFlush 死锁
    // （markLightChanged 不回调 _enqueueLightFlush，无递归锁需求）
    std::vector<std::pair<LightType, SectionPos>> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingLightFlushesMutex);
        pending.swap(m_pendingLightFlushes);
    }

    for (const auto& [type, pos] : pending) {
        markLightChanged(type, pos);
    }
}

void ServerWorld::enqueueChunkLoadLight(ChunkCoord x, ChunkCoord z)
{
    // 主线程调用（chunkLoadedCallback）。先 add LIGHT 票据保活区块（level=Full=33，
    // shouldLoad true→不卸载），覆盖 worker 在途 + processTicketUpdates 生效窗口。
    // shared_ptr 5×5 保活由 RuntimeLightingProvider 构造时建立，与票据互补。
    if (m_chunkManager) {
        m_chunkManager->addLightTicket(x, z);
    }

    util::ServerWorkerPool* executor = m_chunkManager ? m_chunkManager->radiusAwareExecutor() : nullptr;
    if (executor == nullptr) {
        // 启动早期/测试环境：主线程同步执行（无 worker 池）。
        RuntimeLightingProvider provider(*this, x, z);
        _executeChunkLoadLight(provider, x, z);
        return;
    }

    auto task = std::make_unique<ChunkLoadLightTask>(*this, x, z);
    executor->submit(std::move(task), /*callback=*/nullptr, x, z, /*writeRadius=*/2);
}

void ServerWorld::_enqueueChunkSend(ChunkCoord x, ChunkCoord z)
{
    // worker 线程调用（ChunkLoadLightTask::execute/onCancel）：入队 chunk 坐标，
    // 主线程 _drainPendingChunkSends 时 send + removeLightTicket。
    std::lock_guard<std::mutex> lock(m_pendingChunkSendsMutex);
    m_pendingChunkSends.emplace_back(x, z);
}

bool ServerWorld::hasPendingLightWork() const noexcept
{
    // 各队列 empty() 的瞬态无锁近似——仅供外部查询是否需继续 tick，非精确同步。
    return !m_lightQueue.empty() || !m_pendingLightFlushes.empty() || !m_pendingChunkSends.empty();
}

void ServerWorld::_drainPendingChunkSends()
{
    // 主线程调用（在 _drainPendingLightFlushes 之后）：swap 出队列后逐区块 send +
    // removeLightTicket。swap 在锁内最小化临界区；send/remove 在锁外执行。
    std::vector<std::pair<ChunkCoord, ChunkCoord>> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingChunkSendsMutex);
        pending.swap(m_pendingChunkSends);
    }

    if (pending.empty()) {
        return;
    }

    sync::ChunkSendManager* sendManager = m_chunkManager ? m_chunkManager->chunkSendManager() : nullptr;
    for (const auto& [x, z] : pending) {
        // send serialize 读已 flush 的 ChunkSection nibble（flush 先于 send）。
        // 区块已不在 m_chunks 时 sendChunkToTrackingPlayers 内部跳过（hasChunkInMem 校验）。
        if (sendManager != nullptr) {
            sendManager->sendChunkToTrackingPlayers(x, z);
        }
        // 释放 enqueueChunkLoadLight 时 add 的 LIGHT 票据。任务取消路径也经此释放。
        if (m_chunkManager) {
            m_chunkManager->removeLightTicket(x, z);
        }
    }
}

void ServerWorld::_executeChunkLoadLight(RuntimeLightingProvider& provider, ChunkCoord x, ChunkCoord z)
{
    // 主线程 fallback 路径（无 worker 池）：与 ChunkLoadLightTask::execute 同构。
    // 中心区块取自 provider 保活范围。
    IChunk* centerIChunk = provider.getChunkForLight(x, z);
    if (centerIChunk == nullptr) {
        return;
    }

    auto* chunkData = static_cast<ChunkData*>(centerIChunk);

    std::vector<bool> emptySections;
    const ChunkSection* const* sections = chunkData->getSections();
    constexpr i32 sectionCount = world::CHUNK_SECTIONS;
    emptySections.resize(static_cast<size_t>(sectionCount), false);
    for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
        const ChunkSection* section = (sections != nullptr) ? sections[static_cast<size_t>(sectionY)] : nullptr;
        emptySections[static_cast<size_t>(sectionY)] = (section == nullptr || section->isEmpty());
    }

    const bool isLightCorrect = chunkData->isLightCorrect();
    const ChunkLoadStatus status = chunkData->getStatus();
    const bool hasLightStatus = (status == ChunkLoadStatus::Generated || status == ChunkLoadStatus::Loaded);

    WorldLightManager* lightManager = m_lightManager.get();
    MC_ASSERT_RELEASE(lightManager != nullptr);

    if (isLightCorrect && hasLightStatus) {
        if (lightManager->hasSkyLight()) {
            auto* skyEngine = WorldLightManager::acquireSkyLightEngine();
            skyEngine->forceHandleEmptySectionChanges(&provider, chunkData, emptySections);
            skyEngine->StarLightEngine::checkChunkEdges(&provider, x, z);
            WorldLightManager::releaseSkyLightEngine(skyEngine);
        }
        if (lightManager->hasBlockLight()) {
            auto* blockEngine = WorldLightManager::acquireBlockLightEngine();
            blockEngine->forceHandleEmptySectionChanges(&provider, chunkData, emptySections);
            blockEngine->StarLightEngine::checkChunkEdges(&provider, x, z);
            WorldLightManager::releaseBlockLightEngine(blockEngine);
        }
    } else {
        chunkData->setLightCorrect(false);

        if (lightManager->hasBlockLight()) {
            auto* blockEngine = WorldLightManager::acquireBlockLightEngine();
            blockEngine->updateEmptinessMap(x, z, chunkData);
            for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
                const ChunkSection* section = (sections != nullptr) ? sections[static_cast<size_t>(sectionY)] : nullptr;
                const SectionPos sectionPos(x, world::sectionIndexToCoord(sectionY), z);
                blockEngine->updateSectionStatus(sectionPos, section == nullptr || section->isEmpty());
            }
            blockEngine->light(&provider, chunkData, /*needsEdgeChecks=*/true);
            WorldLightManager::releaseBlockLightEngine(blockEngine);
        }

        if (lightManager->hasSkyLight()) {
            auto* skyEngine = WorldLightManager::acquireSkyLightEngine();
            for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
                const ChunkSection* section = (sections != nullptr) ? sections[static_cast<size_t>(sectionY)] : nullptr;
                const SectionPos sectionPos(x, world::sectionIndexToCoord(sectionY), z);
                skyEngine->updateSectionStatus(sectionPos, section == nullptr || section->isEmpty());
            }
            skyEngine->light(&provider, chunkData, /*needsEdgeChecks=*/true);
            WorldLightManager::releaseSkyLightEngine(skyEngine);
        }

        chunkData->setLightCorrect(true);
    }

    // fallback 主线程路径：dirty section 直接调 markLightChanged（无需 flush 队列），
    // 区块发送直接调（无需续延队列），票据释放对称 add。
    auto dirtySections = provider.takeDirtySections();
    for (const auto& [type, pos] : dirtySections) {
        markLightChanged(type, pos);
    }

    sync::ChunkSendManager* sendManager = m_chunkManager ? m_chunkManager->chunkSendManager() : nullptr;
    if (sendManager != nullptr) {
        sendManager->sendChunkToTrackingPlayers(x, z);
    }
    if (m_chunkManager) {
        m_chunkManager->removeLightTicket(x, z);
    }
}

bool ServerWorld::hasSkyLight() const
{
    return getDimensionType().hasSkyLight();
}

DimensionType ServerWorld::getDimensionType() const
{
    // 标准维度：主世界=0，下界=-1，末地=1
    switch (m_config.dimension) {
        case 0:
            return DimensionType::overworld();
        case -1:
            return DimensionType::nether();
        case 1:
            return DimensionType::theEnd();
        default:
            return DimensionType::overworld();
    }
}

Difficulty ServerWorld::difficulty() const
{
    // 如果设置了难度回调，使用回调获取难度
    // 否则返回默认值 Normal
    if (m_difficultyCallback) {
        return m_difficultyCallback();
    }
    return Difficulty::Normal;
}

bool ServerWorld::isPvpAllowed() const
{
    return m_gameRules.getBoolean(world::gamerule::GameRuleKeys::PVP);
}

i32 ServerWorld::getMinBuildHeight() const noexcept
{
    return getDimensionType().minHeight();
}

i32 ServerWorld::getMaxBuildHeight() const noexcept
{
    return getDimensionType().maxHeight();
}

i32 ServerWorld::getSectionCount() const noexcept
{
    return world::CHUNK_SECTIONS;
}

void ServerWorld::_syncLightDataToChunk(LightType type, const SectionPos& pos)
{
    if (!m_lightManager || !m_chunkManager) {
        return;
    }

    ChunkData* chunk = m_chunkManager->tryToGetChunkInMem(pos.x, pos.z);
    if (!chunk) {
        return;
    }

    const i32 sectionIndex = world::sectionCoordToIndex(pos.y);
    if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
        return;
    }

    ChunkSection* section = chunk->getSection(sectionIndex);
    if (!section) {
        return;
    }

    SWMRNibbleArray* lightData = m_lightManager->getData(type, pos);
    if (!lightData) {
        return;
    }

    std::vector<u8> data = lightData->toByteArray();
    if (data.size() != NibbleArray::BYTE_SIZE) {
        return;
    }

    NibbleArray& targetArray = (type == LightType::SKY) ? section->skyLightNibble() : section->blockLightNibble();
    targetArray.data() = std::move(data);
}

std::vector<std::reference_wrapper<Entity>> ServerWorld::_collectLoadedEntitiesForSave()
{
    std::vector<std::reference_wrapper<Entity>> entities;
    m_entityManager.forEachEntity([&entities](Entity* entity) {
        MC_ASSERT_RELEASE(entity != nullptr);
        entities.emplace_back(*entity);
        return true;
    });

    return entities;
}

std::vector<std::reference_wrapper<const BlockEntity>> ServerWorld::_collectLoadedBlockEntitiesForSave() const
{
    std::vector<std::reference_wrapper<const BlockEntity>> blockEntities;
    if (!m_chunkManager) {
        return blockEntities;
    }

    m_chunkManager->forEachLoadedChunk([&blockEntities](ChunkData& chunk) {
        auto chunkBlockEntities = chunk.getAllBlockEntities();
        blockEntities.reserve(blockEntities.size() + chunkBlockEntities.size());
        for (const BlockEntity* blockEntity : chunkBlockEntities) {
            if (blockEntity != nullptr) {
                blockEntities.emplace_back(*blockEntity);
            }
        }
        return true;
    });

    return blockEntities;
}

// ============================================================================
// 粒子接口实现
// ============================================================================

void ServerWorld::addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity)
{
    // 服务端不生成粒子，而是广播给附近玩家
    if (m_onBroadcastParticle) {
        m_onBroadcastParticle(type, pos, velocity, Vector3(0.0f, 0.0f, 0.0f), 1);
    }
}

void ServerWorld::addParticle(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count)
{
    // 服务端不生成粒子，而是广播给附近玩家
    if (m_onBroadcastParticle) {
        m_onBroadcastParticle(type, pos, velocity, offset, count);
    }
}

void ServerWorld::addVibrationParticle(
    const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks)
{
    // 服务端不生成粒子，而是广播振动粒子给附近玩家
    if (m_onBroadcastVibrationParticle) {
        m_onBroadcastVibrationParticle(pos, targetSource, arrivalInTicks);
    }
}

void ServerWorld::addTrailParticle(const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks)
{
    // 服务端不生成粒子，而是广播轨迹粒子给附近玩家
    if (m_onBroadcastTrailParticle) {
        m_onBroadcastTrailParticle(pos, targetPosition, color, durationInTicks);
    }
}

void ServerWorld::addEntityEffectParticle(
    const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color)
{
    // 服务端不生成粒子，而是广播带颜色的 EntityEffect 粒子给附近玩家
    if (m_onBroadcastEntityEffectParticle) {
        m_onBroadcastEntityEffectParticle(pos, velocity, offset, count, color);
    }
}

void ServerWorld::addBlockParticle(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const BlockState& blockState)
{
    // 服务端不生成粒子，而是广播携带方块状态 ID 的方块粒子给附近玩家
    if (m_onBroadcastBlockParticle) {
        m_onBroadcastBlockParticle(type, pos, velocity, blockState.stateId());
    }
}

void ServerWorld::addItemParticle(
    particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack)
{
    // 服务端不生成粒子，而是广播携带物品堆的物品粒子给附近玩家
    if (m_onBroadcastItemParticle) {
        m_onBroadcastItemParticle(type, pos, velocity, itemStack);
    }
}

bool ServerWorld::shouldSpawnParticleAt(const Vector3& pos, f32 maxDistance) const
{
    // 服务端总是返回 true，广播系统会根据玩家距离决定是否发送
    return true;
}

void ServerWorld::broadcastEntityStatus(EntityInstanceId entityId, u8 status)
{
    if (m_onBroadcastEntityStatus) {
        m_onBroadcastEntityStatus(entityId, status);
    }
}

void ServerWorld::broadcastEntityAnimation(EntityInstanceId entityId, u8 animation)
{
    if (m_onBroadcastEntityAnimation) {
        m_onBroadcastEntityAnimation(entityId, animation);
    }
}

void ServerWorld::broadcastHurtAnimation(EntityInstanceId entityId, f32 hurtDir)
{
    if (m_onBroadcastHurtAnimation) {
        m_onBroadcastHurtAnimation(entityId, hurtDir);
    }
}

void ServerWorld::broadcastSetEntityLink(EntityInstanceId entityId, EntityInstanceId linkedEntityId)
{
    if (m_onBroadcastSetEntityLink) {
        m_onBroadcastSetEntityLink(entityId, linkedEntityId);
    }
}

void ServerWorld::broadcastExplosion(const Vector3& position,
    f32 strength,
    const std::vector<BlockPos>& affectedBlocks,
    const std::unordered_map<u64, Vector3>& playerKnockback)
{
    // 委托给 MinecraftServer 注册的回调，按 64 格范围筛选玩家并逐个发送 ExplosionPacket
    if (m_onBroadcastExplosion) {
        m_onBroadcastExplosion(position, strength, affectedBlocks, playerKnockback);
    }
}

// ============================================================================
// 爆炸
// ============================================================================

void ServerWorld::createExplosion(
    const Vector3& position, f32 radius, world::explosion::ExplosionMode mode, bool causesFire, Entity* source)
{
    // 创建爆炸对象
    auto explosion = std::make_unique<world::explosion::Explosion>(*this,
        position,
        radius,
        mode,
        causesFire,
        source,
        nullptr,           // 使用默认伤害来源
        m_lootTableManager // 传递掉落表管理器用于生成方块掉落
    );

    // 执行爆炸
    explosion->explode();

    // 广播爆炸包给客户端（发送给爆炸点 64 格范围内的玩家）
    if (m_onBroadcastExplosion) {
        m_onBroadcastExplosion(position, radius, explosion->affectedBlocks(), explosion->playerKnockback());
    }
}

void ServerWorld::createExplosionWithSource(const Vector3& position,
    f32 radius,
    world::explosion::ExplosionMode mode,
    bool causesFire,
    Entity* source,
    const DamageSource* damageSource)
{
    // 将 const DamageSource* 转换为 std::unique_ptr<DamageSource>（通过 clone）
    std::unique_ptr<DamageSource> damageSourcePtr = (damageSource != nullptr) ? damageSource->clone() : nullptr;

    // 创建爆炸对象（使用自定义伤害来源）
    auto explosion = std::make_unique<world::explosion::Explosion>(*this,
        position,
        radius,
        mode,
        causesFire,
        source,
        std::move(damageSourcePtr), // 传递自定义伤害来源
        m_lootTableManager          // 传递掉落表管理器用于生成方块掉落
    );

    // 执行爆炸
    explosion->explode();

    // 广播爆炸包给客户端（发送给爆炸点 64 格范围内的玩家）
    if (m_onBroadcastExplosion) {
        m_onBroadcastExplosion(position, radius, explosion->affectedBlocks(), explosion->playerKnockback());
    }
}

void ServerWorld::createExplosionWithContext(const Vector3& position,
    f32 radius,
    world::explosion::ExplosionMode mode,
    bool causesFire,
    Entity* source,
    std::unique_ptr<world::explosion::ExplosionContext> context)
{
    // 创建带自定义爆炸上下文的爆炸对象
    auto explosion = std::make_unique<world::explosion::Explosion>(*this,
        position,
        radius,
        mode,
        causesFire,
        source,
        nullptr,            // 使用默认伤害来源
        m_lootTableManager, // 传递掉落表管理器用于生成方块掉落
        std::move(context)  // 传递自定义爆炸上下文
    );

    // 执行爆炸
    explosion->explode();

    // 广播爆炸包给客户端（发送给爆炸点 64 格范围内的玩家）
    if (m_onBroadcastExplosion) {
        m_onBroadcastExplosion(position, radius, explosion->affectedBlocks(), explosion->playerKnockback());
    }
}

// ============================================================================
// 命令执行
// ============================================================================

i32 ServerWorld::executeCommand(const std::string& command, const Vector3d& position, i32 permissionLevel)
{
    // 通过回调执行命令
    // 回调由 IntegratedServer 设置，调用 CommandRegistry::execute()
    if (m_onExecuteCommand) {
        return m_onExecuteCommand(command, position, permissionLevel);
    }
    return 0;
}

// ============================================================================
// 睡眠管理
// ============================================================================

void ServerWorld::skipToMorning()
{
    if (m_timeManager == nullptr) {
        return;
    }

    // 计算下一个早晨的时间
    // dayTime 范围是 0-23999，0 表示早晨 6:00
    i64 currentTime = m_timeManager->dayTime();
    i64 newTime = ((currentTime / 24000) + 1) * 24000;

    m_timeManager->setDayTime(newTime);
}

bool ServerWorld::canSkipNight() const
{
    // 检查日光周期是否启用
    return m_timeManager && m_timeManager->daylightCycleEnabled();
}

bool ServerWorld::canClearWeather() const
{
    // 检查天气周期是否启用
    return m_weatherManager && m_weatherManager->weatherCycleEnabled();
}

void ServerWorld::updateAllPlayersSleepingFlag()
{
    // 检查是否有玩家在睡眠
    bool anySleeping = false;
    bool allSleeping = true;

    // 获取所有玩家实体
    auto players = m_entityManager.getEntitiesByType(entity::EntityTypeKeys::PLAYER);
    if (players.empty()) {
        m_allPlayersSleeping = false;
        return;
    }

    for (Entity* entity : players) {
        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 跳过观察者模式的玩家
        if (player->isSpectator()) {
            continue;
        }

        if (player->isSleeping()) {
            anySleeping = true;
            // 检查是否完全入睡
            if (!player->isPlayerFullyAsleep()) {
                allSleeping = false;
            }
        } else {
            allSleeping = false;
        }
    }

    m_allPlayersSleeping = anySleeping && allSleeping;
}

void ServerWorld::checkSleepStatus()
{
    if (!m_allPlayersSleeping) {
        return;
    }

    // 重新检查所有玩家是否完全入睡
    updateAllPlayersSleepingFlag();

    if (!m_allPlayersSleeping) {
        return;
    }

    // 所有玩家都完全入睡，跳过夜晚
    if (canSkipNight()) {
        skipToMorning();
    }

    // 唤醒所有玩家
    wakeUpAllPlayers();

    // 清除天气
    if (canClearWeather() && m_weatherManager) {
        m_weatherManager->resetWeather();
    }
}

void ServerWorld::wakeUpAllPlayers()
{
    // 获取所有玩家实体并唤醒
    auto players = m_entityManager.getEntitiesByType(entity::EntityTypeKeys::PLAYER);
    for (Entity* entity : players) {
        Player* player = dynamic_cast<Player*>(entity);
        if (player != nullptr && player->isSleeping()) {
            // 直接停止睡眠状态，不需要发送网络包（玩家客户端会被跳过夜晚的逻辑通知）
            player->stopSleeping();
            player->setSleepTimer(0);
        }
    }

    m_allPlayersSleeping = false;
}

void ServerWorld::onBlockPlaced(PlayerId playerId, const BlockPos& pos, const BlockState* state, const ItemStack* item)
{
    // 发布 BlockPlaceEvent 用于进度触发
    event::BlockPlaceEvent event{currentTick(), playerId, pos, state, item};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onZombieVillagerCured(const std::string& starterUuid, Entity* zombie, Entity* villager)
{
    // 发布 CuredZombieVillagerEvent 用于进度触发
    event::CuredZombieVillagerEvent event{currentTick(), starterUuid, zombie, villager};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onChanneledLightning(PlayerId casterId, const std::vector<Entity*>& victims)
{
    // 发布 ChanneledLightningEvent 用于进度触发
    event::ChanneledLightningEvent event{currentTick(), casterId, victims};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onBredAnimals(PlayerId playerId, Entity* child, Entity* parent1, Entity* parent2)
{
    // 发布 BredAnimalsEvent 用于进度触发
    event::BredAnimalsEvent event{currentTick(), playerId, child, parent1, parent2};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onVillagerTrade(
    PlayerId playerId, Entity* villager, const ItemStack& resultItem, const ItemStack& paymentItem)
{
    // 发布 VillagerTradeEvent 用于进度触发
    // bought = 玩家买到的物品（交易结果），sold = 玩家卖出的物品（支付物品）
    event::VillagerTradeEvent event{currentTick(), playerId, villager, resultItem, paymentItem};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onPlayerDestroyItem(PlayerId playerId, const ItemStack& item, i32 slot, Hand hand)
{
    // 发布 PlayerDestroyItemEvent 用于进度触发
    event::PlayerDestroyItemEvent event{currentTick(), playerId, item, slot, hand};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onConsumeItem(PlayerId playerId, const ItemStack& item)
{
    // 发布 ConsumeItemEvent 用于进度触发
    event::ConsumeItemEvent consumeEvent{currentTick(), playerId, item};
    event::ServerEventBus::instance().publish(consumeEvent);
}

void ServerWorld::onItemDurabilityChange(PlayerId playerId, const ItemStack& item, i32 oldDurability, i32 newDurability)
{
    // 发布 ItemDurabilityEvent 用于进度触发
    event::ItemDurabilityEvent durabilityEvent{currentTick(), playerId, item, oldDurability, newDurability};
    event::ServerEventBus::instance().publish(durabilityEvent);
}

void ServerWorld::onEnchantItem(PlayerId playerId, const ItemStack& item, i32 levels)
{
    // 发布 EnchantItemEvent 用于进度触发
    event::EnchantItemEvent enchantEvent{currentTick(), playerId, item, levels, levels};
    event::ServerEventBus::instance().publish(enchantEvent);
}

void ServerWorld::onFilledBucket(PlayerId playerId, const ItemStack& bucket)
{
    // 发布 FilledBucketEvent 用于进度触发
    event::FilledBucketEvent bucketEvent{currentTick(), playerId, bucket};
    event::ServerEventBus::instance().publish(bucketEvent);
}

void ServerWorld::onEnterBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state)
{
    // 发布 EnterBlockEvent 用于进度触发
    if (state == nullptr) {
        return;
    }
    event::EnterBlockEvent enterEvent{currentTick(), playerId, pos, state};
    event::ServerEventBus::instance().publish(enterEvent);
}

void ServerWorld::onSlideDownBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state)
{
    // 发布 SlideDownBlockEvent 用于进度触发
    if (state == nullptr) {
        return;
    }
    event::SlideDownBlockEvent slideEvent{currentTick(), playerId, pos, state};
    event::ServerEventBus::instance().publish(slideEvent);
}

void ServerWorld::onBeeNestDestroyed(
    PlayerId playerId, const BlockPos& pos, const BlockState* state, const ItemStack& tool, i32 numBeesInside)
{
    // 发布 BeeNestDestroyedEvent 用于进度触发
    if (state == nullptr) {
        return;
    }
    event::BeeNestDestroyedEvent beeEvent{currentTick(), playerId, pos, state, tool, numBeesInside};
    event::ServerEventBus::instance().publish(beeEvent);
}

void ServerWorld::onTameAnimal(PlayerId playerId, Entity* animal)
{
    // 发布 TameAnimalEvent 用于进度触发
    event::TameAnimalEvent tameEvent{currentTick(), playerId, animal};
    event::ServerEventBus::instance().publish(tameEvent);
}

void ServerWorld::onSummonedEntity(PlayerId playerId, Entity* entity)
{
    // 发布 SummonedEntityEvent 用于进度触发
    event::SummonedEntityEvent summonEvent{currentTick(), playerId, entity};
    event::ServerEventBus::instance().publish(summonEvent);
}

// ============================================================================
// 结构定位
// ============================================================================

std::optional<BlockPos> ServerWorld::findNearestStructure(
    const BlockPos& center, const ResourceLocation& structureId, i32 maxDistance, bool skipExisting)
{
    // 通过结构 ID 查找所属的 StructureSet，获取放置规则
    auto& structureSetRegistry = world::gen::structure::StructureSetRegistry::instance();
    const world::gen::structure::StructureSet* structureSet = structureSetRegistry.findByStructure(structureId);
    if (structureSet == nullptr) {
        return std::nullopt;
    }

    const auto& placement = structureSet->placement();
    i64 worldSeed = static_cast<i64>(m_config.seed);

    // 获取 StructureCheck 缓存（用于快速跳过不含结构的区块）
    world::gen::structure::StructureCheck* structureCheck = nullptr;
    if (auto* cm = chunkManager()) {
        if (auto* gen = cm->generator()) {
            structureCheck = gen->structureCheck();
        }
    }

    // 将方块坐标转换为区块坐标
    i32 centerChunkX = center.x >> 4;
    i32 centerChunkZ = center.z >> 4;

    // 将最大搜索距离转换为区块范围
    i32 chunkRadius = (maxDistance + 15) >> 4; // 向上取整到区块

    std::optional<BlockPos> nearestPos;
    f64 nearestDistSq = static_cast<f64>(maxDistance * maxDistance) + 1.0;

    // 根据放置策略类型使用不同的搜索算法
    auto* randomSpread =
        dynamic_cast<const world::gen::structure::placement::RandomSpreadStructurePlacement*>(&placement);
    auto* concentricRings =
        dynamic_cast<const world::gen::structure::placement::ConcentricRingsStructurePlacement*>(&placement);

    if (randomSpread != nullptr) {
        // RandomSpread：网格搜索，使用 getPotentialStructureChunk 计算候选区块
        i32 spacing = randomSpread->spacing();

        i32 minGridX = (centerChunkX - chunkRadius) / spacing - 1;
        i32 maxGridX = (centerChunkX + chunkRadius) / spacing + 1;
        i32 minGridZ = (centerChunkZ - chunkRadius) / spacing - 1;
        i32 maxGridZ = (centerChunkZ + chunkRadius) / spacing + 1;

        for (i32 gridX = minGridX; gridX <= maxGridX; ++gridX) {
            for (i32 gridZ = minGridZ; gridZ <= maxGridZ; ++gridZ) {
                i32 baseChunkX = gridX * spacing;
                i32 baseChunkZ = gridZ * spacing;

                // 使用放置规则计算此网格中的候选区块
                auto candidate = randomSpread->getPotentialStructureChunk(worldSeed, baseChunkX, baseChunkZ);

                // 检查候选区块距离是否在搜索范围内
                i32 dx = candidate.x - centerChunkX;
                i32 dz = candidate.z - centerChunkZ;
                if (dx * dx + dz * dz > chunkRadius * chunkRadius) {
                    continue;
                }

                // 验证此候选区块是否真正生成结构（频率缩减 + 排斥区检查）
                if (!placement.isStructureChunk(worldSeed, candidate.x, candidate.z)) {
                    continue;
                }

                // 对齐 MC 1.21.11 ChunkGenerator.getStructureGeneratingAt()：
                // 使用 StructureCheck 缓存快速判断区块是否包含目标结构
                if (structureCheck != nullptr) {
                    const u64 chunkPosId = (static_cast<u64>(static_cast<u32>(candidate.x)) << 32) |
                        static_cast<u64>(static_cast<u32>(candidate.z));
                    auto result = structureCheck->checkStart(chunkPosId, structureId, skipExisting);

                    if (result == world::gen::structure::StructureCheckResult::StartPresent) {
                        // 精确缓存命中：结构存在于该区块，直接返回位置
                        BlockPos locatePos = placement.getLocatePos(candidate);
                        i32 posDx = locatePos.x - center.x;
                        i32 posDz = locatePos.z - center.z;
                        f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

                        if (distSq < nearestDistSq) {
                            nearestDistSq = distSq;
                            nearestPos = locatePos;
                        }
                        continue;
                    }

                    if (result == world::gen::structure::StructureCheckResult::StartNotPresent) {
                        // 精确缓存或近似缓存确认该区块不含目标结构，跳过
                        continue;
                    }

                    // ChunkLoadNeeded：缓存未命中，继续执行当前逻辑（基于放置规则判断）
                    // 将放置规则检查结果写入近似缓存，供后续查询使用
                    structureCheck->setFeatureCheckResult(chunkPosId, true);
                }

                // 使用放置规则的定位偏移计算最终方块位置
                BlockPos locatePos = placement.getLocatePos(candidate);
                i32 posDx = locatePos.x - center.x;
                i32 posDz = locatePos.z - center.z;
                f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

                if (distSq < nearestDistSq) {
                    nearestDistSq = distSq;
                    nearestPos = locatePos;
                }
            }
        }
    } else if (concentricRings != nullptr) {
        // ConcentricRings（要塞）：直接获取所有预计算位置，找最近的
        const auto& ringPositions = concentricRings->getRingPositions(worldSeed);
        for (const auto& chunkPos : ringPositions) {
            i32 dx = chunkPos.x - centerChunkX;
            i32 dz = chunkPos.z - centerChunkZ;
            if (dx * dx + dz * dz > chunkRadius * chunkRadius) {
                continue;
            }

            // 对齐 MC 1.21.11：ConcentricRings 也使用 StructureCheck 缓存
            if (structureCheck != nullptr) {
                const u64 chunkPosId = (static_cast<u64>(static_cast<u32>(chunkPos.x)) << 32) |
                    static_cast<u64>(static_cast<u32>(chunkPos.z));
                auto result = structureCheck->checkStart(chunkPosId, structureId, skipExisting);

                if (result == world::gen::structure::StructureCheckResult::StartPresent) {
                    BlockPos locatePos = placement.getLocatePos(chunkPos);
                    i32 posDx = locatePos.x - center.x;
                    i32 posDz = locatePos.z - center.z;
                    f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

                    if (distSq < nearestDistSq) {
                        nearestDistSq = distSq;
                        nearestPos = locatePos;
                    }
                    continue;
                }

                if (result == world::gen::structure::StructureCheckResult::StartNotPresent) {
                    continue;
                }
            }

            BlockPos locatePos = placement.getLocatePos(chunkPos);
            i32 posDx = locatePos.x - center.x;
            i32 posDz = locatePos.z - center.z;
            f64 distSq = static_cast<f64>(posDx * posDx + posDz * posDz);

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearestPos = locatePos;
            }
        }
    }

    return nearestPos;
}

std::optional<BlockPos> ServerWorld::findNearestMapStructure(
    const BlockPos& center, const ResourceLocation& tagId, i32 maxDistance, bool skipExisting)
{
    // 通过结构标签 ID 查找标签，遍历标签中的所有结构 ID，对每个结构调用 findNearestStructure，
    // 返回所有候选中距离最近的位置。对应 MC 1.21.11 ServerLevel.findNearestMapStructure()。
    auto* tag = world::gen::structure::StructureTags::getTag(tagId);
    if (tag == nullptr) {
        spdlog::warn("ServerWorld::findNearestMapStructure: 未知结构标签 '{}', 返回空", tagId.toString());
        return std::nullopt;
    }

    if (tag->getStructureIds().empty()) {
        return std::nullopt;
    }

    std::optional<BlockPos> nearestPos;
    f64 nearestDistSq = std::numeric_limits<f64>::max();

    for (const auto& structureId : tag->getStructureIds()) {
        auto candidatePos = findNearestStructure(center, structureId, maxDistance, skipExisting);
        if (!candidatePos.has_value()) {
            continue;
        }

        i32 dx = candidatePos->x - center.x;
        i32 dz = candidatePos->z - center.z;
        f64 distSq = static_cast<f64>(dx * dx + dz * dz);

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestPos = candidatePos;
        }
    }

    return nearestPos;
}

// ========== 按需特征放置 ==========

std::unique_ptr<WorldGenRegion> ServerWorld::createFeatureRegion(const BlockPos& position)
{
    constexpr i32 chunkRadius = 1;
    const ChunkCoord centerChunkX = world::toChunkCoord(position.x);
    const ChunkCoord centerChunkZ = world::toChunkCoord(position.z);
    const i32 diameter = 2 * chunkRadius + 1;

    auto* chunkManager = this->chunkManager();
    if (chunkManager == nullptr) {
        return nullptr;
    }

    // 收集已加载的区块
    std::vector<IChunk*> chunks;
    chunks.reserve(static_cast<size_t>(diameter) * static_cast<size_t>(diameter));

    for (i32 dz = -chunkRadius; dz <= chunkRadius; ++dz) {
        for (i32 dx = -chunkRadius; dx <= chunkRadius; ++dx) {
            const ChunkCoord cx = centerChunkX + dx;
            const ChunkCoord cz = centerChunkZ + dz;

            ChunkData* chunkData = chunkManager->tryToGetChunkInMem(cx, cz);
            if (chunkData == nullptr) {
                return nullptr;
            }

            // ChunkData 继承自 IChunk，可以直接作为 IChunk* 使用
            chunks.push_back(static_cast<IChunk*>(chunkData));
        }
    }

    // 使用 FeaturePlacer 构建 WorldGenRegion
    auto region = world::gen::FeaturePlacer::createRegion(
        centerChunkX, centerChunkZ, std::move(chunks), chunkRadius, this->dimension());

    // 填充世界状态
    world::gen::FeaturePlacer::populateWorldState(
        *region, this->seed(), this->currentTick(), this->dayTime(), this->isHardcore(), this->difficulty());

    return region;
}

} // namespace mc::server

#pragma pop_macro("BYTE_SIZE")
