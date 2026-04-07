#include "ServerWorld.hpp"
#include "ServerChunkManager.hpp"
#include "weather/WeatherManager.hpp"
#include "server/core/TimeManager.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/Direction.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>
#include <cmath>

namespace mc::server {

using mc::WorldLightManager;
using mc::IChunk;
using mc::StarLightLightingProvider;
using mc::LightType;
using mc::SectionPos;
using mc::ChunkPos;
using mc::ChunkSection;
using mc::NibbleArray;

// ============================================================================
// ServerWorld 实现
// ============================================================================

ServerWorld::ServerWorld()
{
    auto generator = std::make_unique<NoiseChunkGenerator>(
        m_config.seed,
        DimensionSettings::overworld()
    );
    m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
    m_chunkManager->setViewDistance(m_config.viewDistance);
}

ServerWorld::ServerWorld(const ServerWorldConfig& config)
    : m_config(config)
{
    auto generator = std::make_unique<NoiseChunkGenerator>(
        m_config.seed,
        DimensionSettings::overworld()
    );
    m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
    m_chunkManager->setViewDistance(m_config.viewDistance);
}

ServerWorld::~ServerWorld()
{
    shutdown();
}

Result<void> ServerWorld::initialize()
{
    spdlog::info("Initializing server world with seed {}...", m_config.seed);

    if (m_initialized) {
        return Result<void>::ok();
    }

    if (!m_chunkManager) {
        auto generator = std::make_unique<NoiseChunkGenerator>(
            m_config.seed,
            DimensionSettings::overworld()
        );
        m_chunkManager = std::make_unique<ServerChunkManager>(*this, std::move(generator));
        m_chunkManager->setViewDistance(m_config.viewDistance);
    }

    // 确保区块管理器始终使用世界配置中的视距。
    m_chunkManager->setViewDistance(m_config.viewDistance);

    m_chunkManager->setChunkLoadedCallback([this](ChunkCoord x, ChunkCoord z) {
        initializeChunkLighting(x, z);
        // 通知村庄管理器区块加载
        if (m_villageManager) {
            m_villageManager->onChunkLoaded(x, z);
        }
    });

    m_chunkManager->setChunkUnloadedCallback([this](ChunkCoord x, ChunkCoord z) {
        // 通知村庄管理器区块卸载（用于清理 POI 等）
        if (m_villageManager) {
            m_villageManager->onChunkUnloaded(x, z);
        }
    });

    auto result = m_chunkManager->initialize();
    if (result.failed()) {
        return result;
    }

    m_collisionCache = std::make_unique<physics::CollisionCache>();
    m_physicsEngine = std::make_unique<PhysicsEngine>(*this);
    m_tickManager = std::make_unique<world::tick::TickManager>(*this);

    // 根据维度类型动态设置光照参数
    DimensionType dimensionType = getDimensionType();
    bool hasSkyLight = dimensionType.hasSkyLight();
    m_lightManager = std::make_unique<WorldLightManager>(this, true, hasSkyLight);

    m_weatherManager = std::make_unique<WeatherManager>();
    m_weatherManager->initialize(m_config.seed);
    m_weatherManager->setWorld(this);

    // 初始化村庄和袭击管理器
    m_villageManager = std::make_unique<world::village::VillageManager>(*this);
    m_raidManager = std::make_unique<world::village::raid::RaidManager>(*this, *m_villageManager);

    m_initialized = true;
    spdlog::info("Server world initialized");
    return Result<void>::ok();
}

void ServerWorld::shutdown()
{
    spdlog::info("Shutting down server world...");
    m_initialized = false;

    // 先清理袭击管理器（可能引用村庄）
    m_raidManager.reset();
    // 再清理村庄管理器
    m_villageManager.reset();

    m_weatherManager.reset();
    m_lightManager.reset();
    m_tickManager.reset();
    m_physicsEngine.reset();
    m_collisionCache.reset();

    if (m_chunkManager) {
        m_chunkManager->shutdown();
        m_chunkManager.reset();
    }

    spdlog::info("Server world shut down");
}

void ServerWorld::setConfig(const ServerWorldConfig& config)
{
    m_config = config;
    if (m_chunkManager) {
        m_chunkManager->setViewDistance(config.viewDistance);
    }
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
// 区块管理
// ============================================================================

ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z)
{
    if (m_chunkManager) {
        return m_chunkManager->getChunk(x, z);
    }
    return nullptr;
}

const ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z) const
{
    if (m_chunkManager) {
        return m_chunkManager->getChunk(x, z);
    }
    return nullptr;
}

bool ServerWorld::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    if (m_chunkManager) {
        return m_chunkManager->hasChunk(x, z);
    }
    return false;
}

ChunkData* ServerWorld::getChunkSync(ChunkCoord x, ChunkCoord z)
{
    if (m_chunkManager) {
        return m_chunkManager->getChunkSync(x, z);
    }
    return nullptr;
}

void ServerWorld::unloadChunk(ChunkCoord x, ChunkCoord z)
{
    if (m_chunkManager) {
        m_chunkManager->unloadChunk(x, z);
    }
}

// ============================================================================
// 方块操作
// ============================================================================

const BlockState* ServerWorld::getBlockState(i32 x, i32 y, i32 z) const
{
    ChunkCoord chunkX = blockToChunk(static_cast<f32>(x));
    ChunkCoord chunkZ = blockToChunk(static_cast<f32>(z));

    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) return nullptr;

    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    return chunk->getBlock(localX, y, localZ);
}

bool ServerWorld::setBlock(i32 x, i32 y, i32 z, const BlockState* state)
{
    // 调试世界禁止方块修改
    if (m_config.isDebugWorld) {
        return false;
    }

    ChunkCoord chunkX = blockToChunk(static_cast<f32>(x));
    ChunkCoord chunkZ = blockToChunk(static_cast<f32>(z));

    ChunkData* chunk = getChunkSync(chunkX, chunkZ);
    if (!chunk) return false;

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

    const BlockPos changedPos(x, y, z);
    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    const BlockState* oldState = canonicalizeState(chunk->getBlock(localX, y, localZ));
    const BlockState* newState = canonicalizeState(state);
    if (newState != nullptr && newState->isAir()) {
        newState = airState;
    }

    if (oldState == newState) {
        return false;
    }

    const bool oldIsAir = (oldState == nullptr || oldState->isAir());
    const bool newIsAir = (newState == nullptr || newState->isAir());
    const bool blockTypeChanged =
        (oldState == nullptr || newState == nullptr || oldState->blockId() != newState->blockId());

    i32 oldLightLevel = oldState ? oldState->lightLevel() : 0;
    i32 newLightLevel = newState ? newState->lightLevel() : 0;

    // 通知村庄管理器方块移除（如果旧方块存在且不是空气）
    if (m_villageManager && !oldIsAir) {
        m_villageManager->onBlockRemoved(changedPos);
    }

    if (!oldIsAir && blockTypeChanged) {
        Block& oldBlock = const_cast<Block&>(oldState->getBlock());
        oldBlock.onBlockRemoved(*this, changedPos, *oldState);
    }

    const BlockState* storedState = newIsAir ? nullptr : newState;
    chunk->setBlock(localX, y, localZ, storedState);
    chunk->setDirty(true);

    if (!newIsAir && blockTypeChanged) {
        Block& newBlock = const_cast<Block&>(newState->getBlock());
        newBlock.onBlockAdded(*this, changedPos, *newState);
    }

    // 通知村庄管理器方块放置（如果新方块存在且不是空气）
    if (m_villageManager && !newIsAir) {
        m_villageManager->onBlockPlaced(changedPos, newState->blockId());
    }

    const BlockState* sourceState = (!newIsAir) ? newState : oldState;
    Block* sourceBlock = nullptr;
    if (sourceState != nullptr && !sourceState->isAir()) {
        sourceBlock = &const_cast<Block&>(sourceState->getBlock());
    }

    struct NeighborDelta {
        i32 dx;
        i32 dy;
        i32 dz;
        Direction direction;
    };

    constexpr std::array<NeighborDelta, 6> NEIGHBOR_DELTAS = {{
        {-1, 0, 0, Direction::West},
        {1, 0, 0, Direction::East},
        {0, -1, 0, Direction::Down},
        {0, 1, 0, Direction::Up},
        {0, 0, -1, Direction::North},
        {0, 0, 1, Direction::South}
    }};

    for (const auto& neighbor : NEIGHBOR_DELTAS) {
        const BlockPos neighborPos(x + neighbor.dx, y + neighbor.dy, z + neighbor.dz);
        const BlockState* neighborState = canonicalizeState(getBlockState(
            neighborPos.x,
            neighborPos.y,
            neighborPos.z));

        if (neighborState != nullptr && !neighborState->isAir() && newState != nullptr) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            BlockState updatedStateValue = neighborBlock.updatePostPlacement(
                *neighborState,
                Directions::opposite(neighbor.direction),
                *newState,
                *this,
                neighborPos,
                changedPos);

            const BlockState* updatedState = blockRegistry.getBlockState(updatedStateValue.stateId());
            if (updatedState == nullptr && updatedStateValue.isAir()) {
                updatedState = airState;
            }

            if (updatedState != nullptr && updatedState != neighborState) {
                setBlock(neighborPos.x, neighborPos.y, neighborPos.z, updatedState);
                neighborState = canonicalizeState(getBlockState(neighborPos.x, neighborPos.y, neighborPos.z));
            }
        }

        if (sourceBlock != nullptr && neighborState != nullptr && !neighborState->isAir()) {
            Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
            neighborBlock.neighborChanged(*this, neighborPos, *sourceBlock, changedPos, false);
        }
    }

    if (m_lightManager) {
        MC_TRACE_INSTANT("server.lighting",
            "CheckBlock",
            "pos", fmt::format("({}, {}, {})", changedPos.x, changedPos.y, changedPos.z),
            "oldLight", oldLightLevel,
            "newLight", newLightLevel,
            [flow = ::perfetto::Flow::ProcessScoped(changedPos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });

        m_lightManager->checkBlock(changedPos.x, changedPos.y, changedPos.z);

        if (newLightLevel > oldLightLevel) {
            m_lightManager->onBlockEmissionIncrease(changedPos.x, changedPos.y, changedPos.z, newLightLevel);
        }
    }

    // setBlock 路径不会自动触发 LiquidBlock 回调，这里主动补一次流体初始调度。
    // 同时调度周围六邻域，确保水/岩浆在方块变化后能及时重算流动。
    const auto scheduleFluidAt = [&](const BlockPos& pos, const BlockState* blockState) {
        if (blockState == nullptr || m_tickManager == nullptr) {
            return;
        }

        const fluid::FluidState* fluidState = blockState->getFluidState();
        if (fluidState == nullptr || fluidState->isEmpty()) {
            return;
        }

        fluid::Fluid& fluid = const_cast<fluid::Fluid&>(fluidState->getFluid());
        const i32 delay = std::max(1, fluid.getTickDelay());
        scheduleFluidTick(pos, fluid, delay, world::tick::TickPriority::Normal);
    };

    scheduleFluidAt(changedPos, newState);

    constexpr std::array<std::array<i32, 3>, 6> NEIGHBOR_OFFSETS = {{
        {{-1, 0, 0}},
        {{1, 0, 0}},
        {{0, -1, 0}},
        {{0, 1, 0}},
        {{0, 0, -1}},
        {{0, 0, 1}}
    }};

    for (const auto& offset : NEIGHBOR_OFFSETS) {
        const BlockPos neighborPos(x + offset[0], y + offset[1], z + offset[2]);
        const BlockState* neighborState = canonicalizeState(getBlockState(
            neighborPos.x,
            neighborPos.y,
            neighborPos.z));
        scheduleFluidAt(neighborPos, neighborState);
    }

    return true;
}

// ============================================================================
// 更新循环
// ============================================================================

void ServerWorld::tick()
{
    // 时间由外部 TimeManager 管理，不再自增 tick 计数

    // 调试世界不执行天气 tick
    if (!m_config.isDebugWorld && m_weatherManager) {
        m_weatherManager->tick();
    }

    // 获取当前 tick
    u64 currentTick = m_timeManager ? m_timeManager->currentTick() : 0;
    i64 gameTime = m_timeManager ? m_timeManager->dayTime() : 0;

    // 调试世界不执行计划刻
    if (!m_config.isDebugWorld && m_tickManager) {
        m_tickManager->tick(currentTick);
    }

    if (m_lightManager && m_lightManager->hasLightWork()) {
        m_lightManager->tick(32768, true, true);
    }

    // 调试世界不执行红石清理
    if (!m_config.isDebugWorld) {
        // 定期清理红石火把烧毁记录（每 200 tick）
        if (currentTick % 200 == 0) {
            world::redstone::RedstoneSystem::instance().cleanupBurnoutRecords(currentTick);
        }
    }

    // 更新村庄系统（流言衰减、边界重算等）
    if (m_villageManager) {
        m_villageManager->tick(gameTime);
    }

    // 更新袭击系统（波次推进、掠夺者生成等）
    if (m_raidManager) {
        m_raidManager->tick();
    }

    // EntityManager 由 MinecraftServer 驱动
    // EntityTracker 和 ItemPickupManager 由 MinecraftServer::tickEntities() 驱动

    if (m_chunkManager) {
        m_chunkManager->tick();
    }
}

void ServerWorld::initializeChunkLighting(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    if (!m_lightManager) {
        return;
    }

    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        return;
    }

    ChunkPos chunkPos(chunkX, chunkZ);

    auto* blockLightEngine = m_lightManager->getBlockLightEngine();
    if (blockLightEngine != nullptr) {
        blockLightEngine->updateEmptinessMap(chunkX, chunkZ, chunk);
    }

    for (i32 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk->getSection(sectionY);
        SectionPos sectionPos(chunkX, sectionY, chunkZ);

        bool isEmpty = (section == nullptr || section->isEmpty());
        m_lightManager->updateSectionStatus(sectionPos, isEmpty);

        if (section != nullptr) {
            if (m_lightManager->getSkyLightEngine()) {
                NibbleArray skyLightCopy = section->skyLightNibble().copy();
                m_lightManager->setData(LightType::SKY, sectionPos, skyLightCopy, false);
            }

            if (m_lightManager->getBlockLightEngine()) {
                NibbleArray blockLightCopy = section->blockLightNibble().copy();
                m_lightManager->setData(LightType::BLOCK, sectionPos, blockLightCopy, false);
            }
        }
    }

    m_lightManager->enableLightSources(chunkPos, true);
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
    return m_timeManager ? m_timeManager->dayTime() : 0;
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
    return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
}

i32 ServerWorld::getHeight(i32 x, i32 z) const
{
    const ChunkCoord chunkX = blockToChunk(static_cast<f32>(x));
    const ChunkCoord chunkZ = blockToChunk(static_cast<f32>(z));

    const ChunkData* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) {
        // 区块未加载时返回海平面附近，避免调用方得到无意义常量值。
        return world::SEA_LEVEL + 1;
    }

    const i32 localX = x - chunkX * 16;
    const i32 localZ = z - chunkZ * 16;

    // 优先使用世界生成高度图（返回的是“顶部空气层 Y”）。
    i32 height = chunk->getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ);
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
    ChunkCoord minChunkX = blockToChunk(box.minX);
    ChunkCoord maxChunkX = blockToChunk(box.maxX);
    ChunkCoord minChunkZ = blockToChunk(box.minZ);
    ChunkCoord maxChunkZ = blockToChunk(box.maxZ);

    for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
            const ChunkData* chunk = getChunk(cx, cz);
            if (!chunk) continue;

            i32 minY = std::max(0, static_cast<i32>(std::floor(box.minY)));
            i32 maxY = std::min(255, static_cast<i32>(std::ceil(box.maxY)));

            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = 0; z < 16; ++z) {
                    for (i32 x = 0; x < 16; ++x) {
                        i32 wx = cx * 16 + x;
                        i32 wz = cz * 16 + z;

                        if (wx + 1 < box.minX || wx > box.maxX ||
                            wz + 1 < box.minZ || wz > box.maxZ) {
                            continue;
                        }

                        const BlockState* state = chunk->getBlock(x, y, z);
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

    ChunkCoord minChunkX = blockToChunk(box.minX);
    ChunkCoord maxChunkX = blockToChunk(box.maxX);
    ChunkCoord minChunkZ = blockToChunk(box.minZ);
    ChunkCoord maxChunkZ = blockToChunk(box.maxZ);

    for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
        for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
            const ChunkData* chunk = getChunk(cx, cz);
            if (!chunk) continue;

            i32 minY = std::max(0, static_cast<i32>(std::floor(box.minY)));
            i32 maxY = std::min(255, static_cast<i32>(std::ceil(box.maxY)));

            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = 0; z < 16; ++z) {
                    for (i32 x = 0; x < 16; ++x) {
                        i32 wx = cx * 16 + x;
                        i32 wz = cz * 16 + z;

                        if (wx + 1 < box.minX || wx > box.maxX ||
                            wz + 1 < box.minZ || wz > box.maxZ) {
                            continue;
                        }

                        const BlockState* state = chunk->getBlock(x, y, z);
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

std::vector<AxisAlignedBB> ServerWorld::getEntityCollisions(
    const AxisAlignedBB& box, const Entity* except) const
{
    std::vector<AxisAlignedBB> collisions;
    auto entities = m_entityManager.getEntitiesInAABB(box, except);
    collisions.reserve(entities.size());

    for (const Entity* entity : entities) {
        collisions.push_back(entity->boundingBox());
    }

    return collisions;
}

std::vector<Entity*> ServerWorld::getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const
{
    return m_entityManager.getEntitiesInAABB(box, except);
}

std::vector<Entity*> ServerWorld::getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const
{
    return m_entityManager.getEntitiesInRange(pos, range, except);
}

// ============================================================================
// 碰撞缓存
// ============================================================================

void ServerWorld::invalidateCollisionCache(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    if (m_collisionCache) {
        m_collisionCache->invalidateChunkAndNeighbors(chunkX, chunkZ);
    }
}

void ServerWorld::clearCollisionCache()
{
    if (m_collisionCache) {
        m_collisionCache->clear();
    }
}

// ============================================================================
// 实体管理
// ============================================================================

EntityId ServerWorld::spawnEntity(std::unique_ptr<Entity> entity)
{
    if (!entity) {
        return 0;
    }

    entity->setWorld(this);
    EntityId id = m_entityManager.addEntity(std::move(entity));

    Entity* addedEntity = m_entityManager.getEntity(id);
    if (addedEntity) {
        m_entityTracker.trackEntity(addedEntity);
    }

    spdlog::debug("Spawned entity with ID {}", id);
    return id;
}

std::unique_ptr<Entity> ServerWorld::removeEntity(EntityId id)
{
    auto entity = m_entityManager.removeEntity(id);
    if (entity) {
        spdlog::debug("Removed entity with ID {}", id);
    }
    return entity;
}

Entity* ServerWorld::getEntity(EntityId id)
{
    return m_entityManager.getEntity(id);
}

const Entity* ServerWorld::getEntity(EntityId id) const
{
    return m_entityManager.getEntity(id);
}

bool ServerWorld::hasEntity(EntityId id) const
{
    return m_entityManager.hasEntity(id);
}

size_t ServerWorld::entityCount() const
{
    return m_entityManager.entityCount();
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

        EntityId entityId = m_entityManager.addEntity(std::move(entity));
        if (entityId != 0) {
            Entity* addedEntity = m_entityManager.getEntity(entityId);
            if (addedEntity) {
                m_entityTracker.trackEntity(addedEntity);
            }
            ++spawnedCount;
        }
    }

    return spawnedCount;
}

// ============================================================================
// Tick调度便捷方法
// ============================================================================

void ServerWorld::scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay,
                                     world::tick::TickPriority priority)
{
    if (m_tickManager) {
        m_tickManager->scheduleBlockTick(pos, block, delay, priority);
    }
}

void ServerWorld::scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay,
                                     world::tick::TickPriority priority)
{
    if (m_tickManager) {
        m_tickManager->scheduleFluidTick(pos, fluid, delay, priority);
    }
}

// ============================================================================
// StarLightLightingProvider 接口实现
// ============================================================================

IChunk* ServerWorld::getChunkForLight(ChunkCoord x, ChunkCoord z)
{
    return m_chunkManager ? m_chunkManager->getChunk(x, z) : nullptr;
}

const IChunk* ServerWorld::getChunkForLight(ChunkCoord x, ChunkCoord z) const
{
    return m_chunkManager ? m_chunkManager->getChunk(x, z) : nullptr;
}

const BlockState* ServerWorld::getBlockStateForLight(const BlockPos& pos) const
{
    return getBlockState(pos.x, pos.y, pos.z);
}

IWorld* ServerWorld::getWorld()
{
    return this;
}

const IWorld* ServerWorld::getWorld() const
{
    return this;
}

void ServerWorld::markLightChanged(LightType type, const SectionPos& pos)
{
    MC_TRACE_INSTANT("server.lighting", "ServerWorld::markLightChanged",
               "Type", (type == LightType::SKY) ? "Sky" : "Block",
               "Section", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z));

    if (m_chunkManager) {
        ChunkData* chunk = m_chunkManager->getChunk(pos.x, pos.z);
        if (chunk) {
            chunk->setDirty(true);
        }
    }

    syncLightDataToChunk(type, pos);

    if (m_onLightChanged) {
        m_onLightChanged(type, pos);
    }
}

bool ServerWorld::hasSkyLight() const
{
    return getDimensionType().hasSkyLight();
}

DimensionType ServerWorld::getDimensionType() const
{
    switch (m_config.dimension) {
        case 0:
            return DimensionType::overworld();
        case 1:
            return DimensionType::nether();
        case 2:
            return DimensionType::theEnd();
        default:
            return DimensionType::overworld();
    }
}

i32 ServerWorld::getMinBuildHeight() const
{
    return world::MIN_BUILD_HEIGHT;
}

i32 ServerWorld::getMaxBuildHeight() const
{
    return world::MAX_BUILD_HEIGHT;
}

i32 ServerWorld::getSectionCount() const
{
    return world::CHUNK_SECTIONS;
}

void ServerWorld::syncLightDataToChunk(LightType type, const SectionPos& pos)
{
    if (!m_lightManager || !m_chunkManager) {
        return;
    }

    ChunkData* chunk = m_chunkManager->getChunk(pos.x, pos.z);
    if (!chunk) {
        return;
    }

    const i32 sectionIndex = pos.y;
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

    NibbleArray& targetArray = (type == LightType::SKY)
        ? section->skyLightNibble()
        : section->blockLightNibble();
    targetArray.data() = std::move(data);
}

} // namespace mc::server