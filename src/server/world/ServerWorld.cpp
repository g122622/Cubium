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
#include "ServerChunkManager.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/player/SpawnLocationHelper.hpp"
#include "common/entity/inventory/INamedContainerProvider.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/core/CoordConverter.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include "server/core/TimeManager.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "weather/WeatherManager.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

#pragma pop_macro("BYTE_SIZE")

namespace mc::server {

using mc::ChunkPos;
using mc::ChunkSection;
using mc::IChunk;
using mc::LightType;
using mc::NibbleArray;
using mc::SectionPos;
using mc::StarLightLightingProvider;
using mc::WorldLightManager;
using mc::util::core::CoordConverter;

// ============================================================================
// ServerWorld 实现
// ============================================================================

ServerWorld::ServerWorld(const ServerWorldConfig& config)
    : m_config(config)
{
    MC_TRACE_EVENT("server.initialization", "ServerWorld::Constructor", "dimension", config.dimension);
}

ServerWorld::ServerWorld(const ServerWorldConfig& config, std::unique_ptr<ServerChunkManager> chunkManager)
    : m_config(config)
    , m_chunkManager(std::move(chunkManager))
{
    MC_TRACE_EVENT("server.initialization", "ServerWorld::Constructor", "dimension", config.dimension);
    MC_ASSERT_RELEASE(m_chunkManager != nullptr);
    m_chunkManager->setViewDistance(m_config.viewDistance);
}

ServerWorld::~ServerWorld()
{
    shutdown();
}

Result<void> ServerWorld::initialize()
{
    MC_TRACE_EVENT("server.initialization", "ServerWorld::initialize");
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

    // 先保存所有脏数据
    if (m_storage != nullptr && m_storage->isOpen()) {
        auto saveResult = saveAll();
        if (saveResult.failed()) {
            spdlog::error("Failed to save world: {}", saveResult.error().message());
        } else {
            spdlog::info("Saved {} cached sections during shutdown", saveResult.value());
        }
    }

    // 先清理袭击管理器（可能引用村庄）
    m_raidManager.reset();
    // 再清理村庄管理器
    m_villageManager.reset();

    // 先停止区块管理器，避免后台生成/加载回调在世界子系统拆除后继续触发方块更新。
    if (m_chunkManager) {
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
    MC_TRACE_EVENT("server.world", "ServerWorld::saveAll");

    if (m_storage == nullptr || !m_storage->isOpen()) {
        return Error(ErrorCode::InvalidState, "Storage not open");
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
    // 参考 MC 1.16.5: DimensionGeneratorSettings.func_236227_h_()
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

    // 直接使用 m_chunkManager->getChunkSync()，确保出生点区块已加载
    ChunkPos spawnChunk(0, 0);
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

// ============================================================================
// 区块管理
// ============================================================================

// 注意：非const版本的 getChunk 提供可变访问，供需要修改区块的场景使用。
// const版本是 IWorld 接口实现。
ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z)
{
    if (m_chunkManager) {
        return m_chunkManager->tryToGetChunkInMem(x, z);
    }
    return nullptr;
}

const ChunkData* ServerWorld::getChunk(ChunkCoord x, ChunkCoord z) const
{
    if (m_chunkManager) {
        return m_chunkManager->tryToGetChunkInMem(x, z);
    }
    return nullptr;
}

bool ServerWorld::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    if (m_chunkManager) {
        return m_chunkManager->hasChunkInMem(x, z);
    }
    return false;
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

    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    return chunk->getBlockState(localX, y, localZ);
}

// ============================================================================
// 修改世界中的方块
// 注意：会自动给客户端发包，不要在外部调用后再发一次！
// ============================================================================
bool ServerWorld::setBlockState(i32 x, i32 y, i32 z, const BlockState* state)
{
    MC_TRACE_EVENT("server.world",
        "ServerWorld::setBlockState",
        "x",
        x,
        "y",
        y,
        "z",
        z,
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(x, y, z).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    const BlockPos changedPos(x, y, z);

    {
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::DebugWorldCheck", "x", x, "y", y, "z", z);

        // 调试世界禁止方块修改
        if (isDebugWorld()) {
            return false;
        }
    }

    ChunkCoord chunkX = CoordConverter::blockToChunk(x);
    ChunkCoord chunkZ = CoordConverter::blockToChunk(z);
    ChunkData* chunk = nullptr;

    {
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::ChunkLookup", "chunkX", chunkX, "chunkZ", chunkZ);

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

    i32 localX = x - chunkX * 16;
    i32 localZ = z - chunkZ * 16;

    const BlockState* oldState = nullptr;
    const BlockState* newState = nullptr;

    {
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::CanonicalizeState", "x", x, "y", y, "z", z);

        oldState = canonicalizeState(chunk->getBlockState(localX, y, localZ));
        newState = canonicalizeState(state);
        if (newState != nullptr && newState->isAir()) {
            newState = airState;
        }
    }

    {
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::StateComparison", "x", x, "y", y, "z", z);

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
        MC_TRACE_EVENT("server.world",
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
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::OldBlockCallbacks", "x", x, "y", y, "z", z);

        // 通知村庄管理器方块移除（如果旧方块存在且不是空气）
        if (m_villageManager && !oldIsAir) {
            m_villageManager->onBlockRemoved(changedPos);
        }

        if (!oldIsAir && blockTypeChanged) {
            Block& oldBlock = const_cast<Block&>(oldState->getBlock());
            oldBlock.onBlockRemoved(*this, changedPos, *oldState);
        }
    }

    const BlockState* currentState = canonicalizeState(chunk->getBlockState(localX, y, localZ));
    if (currentState != newState) {
        return true;
    }

    {
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::NewBlockCallbacks", "x", x, "y", y, "z", z);

        if (m_onBlockChanged) {
            m_onBlockChanged(changedPos, currentState ? currentState->stateId() : 0u);
        }

        if (!newIsAir && blockTypeChanged) {
            Block& newBlock = const_cast<Block&>(newState->getBlock());
            newBlock.onBlockAdded(*this, changedPos, *newState);
        }

        // MC 1.16.5: 新方块有方块实体时创建
        // 参考: net.minecraft.world.chunk.Chunk.setBlockState
        if (!newIsAir && newState->getBlock().hasBlockEntity()) {
            Block& newBlock = const_cast<Block&>(newState->getBlock());
            auto blockEntity = newBlock.createBlockEntity(changedPos);
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
        sourceBlock = &const_cast<Block&>(sourceState->getBlock());
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
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::NeighborUpdates", "x", x, "y", y, "z", z);

        for (const auto& neighbor : NEIGHBOR_DELTAS) {
            const BlockPos neighborPos(x + neighbor.dx, y + neighbor.dy, z + neighbor.dz);
            const BlockState* neighborState =
                canonicalizeState(getBlockState(neighborPos.x, neighborPos.y, neighborPos.z));

            const BlockState* updatedState = nullptr;

            {
                MC_TRACE_EVENT("server.world",
                    "ServerWorld::setBlockState::NeighborUpdatePostPlacement",
                    "x",
                    neighborPos.x,
                    "y",
                    neighborPos.y,
                    "z",
                    neighborPos.z);

                if (neighborState != nullptr && !neighborState->isAir() && newState != nullptr) {
                    Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
                    BlockState updatedStateValue = neighborBlock.updatePostPlacement(*neighborState,
                        Directions::opposite(neighbor.direction),
                        *newState,
                        *this,
                        neighborPos,
                        changedPos);

                    updatedState = blockRegistry.getBlockState(updatedStateValue.stateId());
                    if (updatedState == nullptr && updatedStateValue.isAir()) {
                        updatedState = airState;
                    }
                }
            }

            if (updatedState != nullptr && updatedState != neighborState) {
                setBlockState(neighborPos, updatedState);
                neighborState = canonicalizeState(getBlockState(neighborPos));
            }

            {
                MC_TRACE_EVENT("server.world",
                    "ServerWorld::setBlockState::NeighborChanged",
                    "x",
                    neighborPos.x,
                    "y",
                    neighborPos.y,
                    "z",
                    neighborPos.z);

                if (sourceBlock != nullptr && neighborState != nullptr && !neighborState->isAir()) {
                    Block& neighborBlock = const_cast<Block&>(neighborState->getBlock());
                    neighborBlock.neighborChanged(*this, neighborPos, *sourceBlock, changedPos, false);
                }
            }
        }
    }

    {
        MC_TRACE_EVENT("server.world",
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
            m_lightManager->checkBlock(changedPos.x, changedPos.y, changedPos.z);

            if (newLightLevel > oldLightLevel) {
                m_lightManager->onBlockEmissionIncrease(changedPos.x, changedPos.y, changedPos.z, newLightLevel);
            }
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

        fluid::Fluid& fluid = const_cast<fluid::Fluid&>(fluidState->getFluid());
        m_tickManager->scheduleFluidTick(pos, fluid, fluid.getTickDelay(*this), world::tick::TickPriority::Normal);
    };

    {
        MC_TRACE_EVENT("server.world", "ServerWorld::setBlockState::FluidScheduling", "x", x, "y", y, "z", z);

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
        // 注意：在 MC 1.16.5 中，如果区块未加载，方块实体会丢失
        // 这里我们直接释放实体以避免内存泄漏
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
    MC_TRACE_EVENT("server.tick", "ServerWorld::tick");

    // 时间由外部 TimeManager 管理，不再自增 tick 计数

    // 区块 tick - 包括区块内实体、方块随机刻、区块状态更新等
    if (m_chunkManager) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::ChunkTick");
        m_chunkManager->tick();
    }

    // 光照更新 - 限制每 tick 最多处理 32768 个区块，避免过长卡顿
    if (m_lightManager && m_lightManager->hasLightWork()) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::LightManager");
        m_lightManager->tick(32768, true, true);
    }

    // 获取当前 tick
    u64 currentTick = m_timeManager ? m_timeManager->currentTick() : 0;
    i64 gameTime = m_timeManager ? m_timeManager->dayTime() : 0;

    if (m_storage) {
        m_storage->tickAutoSave(currentTick);
    }

    // 调试世界不执行计划刻
    if (!isDebugWorld() && m_tickManager) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::TickManager");
        m_tickManager->tick(currentTick);
    }

    // 调试世界不执行随机刻
    if (!isDebugWorld()) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::EnvironmentTick");
        // 从游戏规则获取随机刻速度
        i32 randomTickSpeed = m_gameRules.getInt(world::gamerule::GameRuleKeys::RANDOM_TICK_SPEED);
        tickEnvironment(randomTickSpeed);
    }

    // 调试世界不执行红石清理
    if (!isDebugWorld()) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::RedstoneTick");
        // 定期清理红石火把烧毁记录（每 200 tick）
        if (currentTick % 200 == 0) {
            world::redstone::RedstoneSystem::instance().cleanupBurnoutRecords(currentTick);
        }
    }

    // 调试世界不执行天气 tick
    if (!isDebugWorld() && m_weatherManager) {
        m_weatherManager->tick();
    }

    // 检查全员睡眠状态
    if (m_allPlayersSleeping) {
        checkSleepStatus();
    }

    // 更新村庄系统（流言衰减、边界重算等）
    if (m_villageManager) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::VillageTick");
        m_villageManager->tick(gameTime);
    }

    // 更新袭击系统（波次推进、掠夺者生成等）
    if (m_raidManager) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::RaidTick");
        m_raidManager->tick();
    }

    // 更新村庄围攻系统（僵尸围村）
    // 调试世界不执行村庄围攻
    if (!isDebugWorld()) {
        MC_TRACE_EVENT("server.tick", "ServerWorld::tick::VillageSiege");
        m_villageSiege.tick(*this, true); // spawnHostiles = true
    }

    // 更新世界边界（渐变动画）
    m_worldBorder.tick();

    // EntityManager 由 MinecraftServer 驱动
    // EntityTracker 和 ItemPickupManager 由 MinecraftServer::tickEntities() 驱动
}

// ============================================================================
// 随机刻系统
// ============================================================================

void ServerWorld::tickEnvironment(i32 randomTickSpeed)
{
    if (randomTickSpeed <= 0 || !m_chunkManager) {
        return;
    }

    MC_TRACE_EVENT("server.tick", "ServerWorld::tickEnvironment", "randomTickSpeed", randomTickSpeed);

    // 遍历所有已加载区块
    // 参考: MC 1.16.5 ServerWorld.tickEnvironment()
    m_chunkManager->forEachLoadedChunk([this, randomTickSpeed](ChunkData& chunk) {
        MC_TRACE_EVENT("server.tick", "tickChunk", "x", chunk.x(), "z", chunk.z());

        // 获取区块起始坐标（方块坐标）
        i32 chunkX = chunk.x() * 16;
        i32 chunkZ = chunk.z() * 16;

        // 遍历区块中的每个段
        for (i32 sectionIndex = 0; sectionIndex < world::CHUNK_SECTIONS; ++sectionIndex) {
            const ChunkSection* section = chunk.getSection(sectionIndex);
            if (!section || !section->needsRandomTickAny()) {
                continue;
            }

            // 区块段的 Y 起始坐标
            i32 sectionY = sectionIndex * 16 + world::MIN_BUILD_HEIGHT;

            // 对每个 randomTickSpeed，选择一个随机位置执行 tick
            for (i32 i = 0; i < randomTickSpeed; ++i) {
                BlockPos pos = getBlockRandomPos(chunkX, sectionY, chunkZ);

                // 获取方块状态
                const BlockState* blockState = chunk.getBlockState(pos.x - chunkX, pos.y, pos.z - chunkZ);

                if (blockState) {
                    // 执行方块随机刻
                    if (blockState->getBlock().ticksRandomly()) {
                        Block& block = const_cast<Block&>(blockState->getBlock());
                        block.randomTick(*this, pos, const_cast<BlockState&>(*blockState), m_random);
                    }

                    // 执行流体随机刻
                    const fluid::FluidState* fluidState = blockState->getFluidState();
                    if (fluidState && !fluidState->isEmpty() && fluidState->getFluid().ticksRandomly()) {
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
    // MC 1.16.5 风格的随机位置生成
    // 使用 LCG (Linear Congruential Generator) 确保分布均匀
    // 参考: net.minecraft.world.World.getBlockRandomPos
    m_updateLCG = m_updateLCG * 3 + 1013904223;
    i32 i = static_cast<i32>(m_updateLCG >> 2);

    // 计算 x, y, z 偏移
    // x = chunkX + (i & 15)          -> 范围 [0, 15]
    // y = sectionY + ((i >> 16) & 15) -> 范围 [0, 15]
    // z = chunkZ + ((i >> 8) & 15)   -> 范围 [0, 15]

    return BlockPos(chunkX + (i & 15), sectionY + ((i >> 16) & 15), chunkZ + ((i >> 8) & 15));
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
                for (i32 z = 0; z < 16; ++z) {
                    for (i32 x = 0; x < 16; ++x) {
                        i32 wx = cx * 16 + x;
                        i32 wz = cz * 16 + z;

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
                for (i32 z = 0; z < 16; ++z) {
                    for (i32 x = 0; x < 16; ++x) {
                        i32 wx = cx * 16 + x;
                        i32 wz = cz * 16 + z;

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

        // MC 1.16.5: 观察者模式的玩家不计入
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

        // MC 1.16.5: 观察者模式的玩家不计入距离检查
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
    } else {
        // 理论上不应该发生，addEntity 成功后应该能通过 getEntity 获取到实体。这里做个断言以便排查潜在问题。
        MC_ASSERT_RELEASE(false);
    }

    // spdlog::debug("Spawned entity with ID {}", id);
    return id;
}

// 注意：此方法不仅移除实体，还负责取消追踪。
// 调用者应使用此方法而非直接调用 entityManager().removeEntity()，
// 以确保实体追踪器状态正确更新。
std::unique_ptr<Entity> ServerWorld::removeEntity(EntityId id)
{
    auto entity = m_entityManager.removeEntity(id);
    if (entity) {
        // 从追踪器中移除
        m_entityTracker.untrackEntity(id);
        // spdlog::debug("Removed entity with ID {}", id);
    } else {
        spdlog::error("Attempted to remove non-existent entity with ID {}", id);
    }
    return entity;
}

// IWorld 接口实现：委托给 EntityManager
Entity* ServerWorld::getEntity(EntityId id)
{
    return m_entityManager.getEntity(id);
}

const Entity* ServerWorld::getEntity(EntityId id) const
{
    return m_entityManager.getEntity(id);
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
    MC_TRACE_EVENT("server.lighting",
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
    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
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

    ChunkData* chunk = m_chunkManager->tryToGetChunkInMem(pos.x, pos.z);
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

    NibbleArray& targetArray = (type == LightType::SKY) ? section->skyLightNibble() : section->blockLightNibble();
    targetArray.data() = std::move(data);
}

// ============================================================================
// 粒子接口实现
// ============================================================================

void ServerWorld::addParticle(
    client::renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity)
{
    // 服务端不生成粒子，而是广播给附近玩家
    if (m_onBroadcastParticle) {
        m_onBroadcastParticle(type, pos, velocity, Vector3(0.0f, 0.0f, 0.0f), 1);
    }
}

void ServerWorld::addParticle(client::renderer::trident::particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
{
    // 服务端不生成粒子，而是广播给附近玩家
    if (m_onBroadcastParticle) {
        m_onBroadcastParticle(type, pos, velocity, offset, count);
    }
}

bool ServerWorld::shouldSpawnParticleAt(const Vector3& pos, f32 maxDistance) const
{
    // 服务端总是返回 true，广播系统会根据玩家距离决定是否发送
    return true;
}

void ServerWorld::broadcastEntityStatus(EntityId entityId, u8 status)
{
    if (m_onBroadcastEntityStatus) {
        m_onBroadcastEntityStatus(entityId, status);
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

    // 广播爆炸包给客户端
    // 参考 MC 1.16.5: 发送给爆炸点 64 格范围内的玩家
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

    spdlog::debug("ServerWorld: skipped to morning (dayTime {} -> {})", currentTime, newTime);
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
    auto players = m_entityManager.getEntitiesByType(entity::EntityTypeIdNumber::PLAYER);
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
    auto players = m_entityManager.getEntitiesByType(entity::EntityTypeIdNumber::PLAYER);
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
    // 参考 MC 1.16.5: CriteriaTriggers.PLACED_BLOCK.trigger()
    event::BlockPlaceEvent event{currentTick(), playerId, pos, state, item};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onZombieVillagerCured(const std::string& starterUuid, Entity* zombie, Entity* villager)
{
    // 发布 CuredZombieVillagerEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.CURED_ZOMBIE_VILLAGER.trigger()
    event::CuredZombieVillagerEvent event{currentTick(), starterUuid, zombie, villager};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onChanneledLightning(PlayerId casterId, const std::vector<Entity*>& victims)
{
    // 发布 ChanneledLightningEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.CHANNELED_LIGHTNING.trigger()
    event::ChanneledLightningEvent event{currentTick(), casterId, victims};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onBredAnimals(PlayerId playerId, Entity* child, Entity* parent1, Entity* parent2)
{
    // 发布 BredAnimalsEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.BRED_ANIMALS.trigger()
    event::BredAnimalsEvent event{currentTick(), playerId, child, parent1, parent2};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onPlayerDestroyItem(PlayerId playerId, const ItemStack& item, i32 slot, Hand hand)
{
    // 发布 PlayerDestroyItemEvent 用于进度触发
    // 参考 MC 1.16.5: Forge PlayerDestroyItemEvent
    // 参考 MC 1.16.5: CriteriaTriggers.ITEM_DURABILITY_CHANGED
    event::PlayerDestroyItemEvent event{currentTick(), playerId, item, slot, hand};
    event::ServerEventBus::instance().publish(event);
}

void ServerWorld::onConsumeItem(PlayerId playerId, const ItemStack& item)
{
    // 发布 ConsumeItemEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.CONSUME_ITEM.trigger()
    event::ConsumeItemEvent consumeEvent{currentTick(), playerId, item};
    event::ServerEventBus::instance().publish(consumeEvent);
}

void ServerWorld::onItemDurabilityChange(PlayerId playerId, const ItemStack& item, i32 oldDurability, i32 newDurability)
{
    // 发布 ItemDurabilityEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.ITEM_DURABILITY_CHANGED.trigger()
    event::ItemDurabilityEvent durabilityEvent{currentTick(), playerId, item, oldDurability, newDurability};
    event::ServerEventBus::instance().publish(durabilityEvent);
}

void ServerWorld::onEnchantItem(PlayerId playerId, const ItemStack& item, i32 levels)
{
    // 发布 EnchantItemEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.ENCHANTED_ITEM.trigger()
    event::EnchantItemEvent enchantEvent{currentTick(), playerId, item, levels, levels};
    event::ServerEventBus::instance().publish(enchantEvent);
}

void ServerWorld::onFilledBucket(PlayerId playerId, const ItemStack& bucket)
{
    // 发布 FilledBucketEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.FILLED_BUCKET.trigger()
    event::FilledBucketEvent bucketEvent{currentTick(), playerId, bucket};
    event::ServerEventBus::instance().publish(bucketEvent);
}

void ServerWorld::onEnterBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state)
{
    // 发布 EnterBlockEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.ENTER_BLOCK.trigger()
    if (state == nullptr) {
        return;
    }
    event::EnterBlockEvent enterEvent{currentTick(), playerId, pos, state};
    event::ServerEventBus::instance().publish(enterEvent);
}

void ServerWorld::onSlideDownBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state)
{
    // 发布 SlideDownBlockEvent 用于进度触发
    // 参考 MC 1.16.5: CriteriaTriggers.SLIDE_DOWN_BLOCK.trigger()
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
    // 参考 MC 1.16.5: CriteriaTriggers.BEE_NEST_DESTROYED.trigger()
    if (state == nullptr) {
        return;
    }
    event::BeeNestDestroyedEvent beeEvent{currentTick(), playerId, pos, state, tool, numBeesInside};
    event::ServerEventBus::instance().publish(beeEvent);
}

// ============================================================================
// 结构定位
// ============================================================================

namespace {

/**
 * @brief 将 StructureType 转换为结构名称
 *
 * 参考 MC 1.16.5 Structure 注册表
 */
const char* structureTypeToName(world::gen::structure::StructureType type)
{
    using namespace world::gen::structure;
    switch (type) {
        case StructureType::Shipwreck:
            return "shipwreck";
        case StructureType::OceanRuin:
            return "ocean_ruin";
        case StructureType::BuriedTreasure:
            return "buried_treasure";
        case StructureType::Village:
            return "village";
        case StructureType::Stronghold:
            return "stronghold";
        case StructureType::Mineshaft:
            return "mineshaft";
        case StructureType::Monument:
            return "ocean_monument";
        case StructureType::Temple:
            return "temple"; // 包括沙漠神殿、丛林神庙等
        case StructureType::RuinedPortal:
            return "ruined_portal";
        case StructureType::WoodlandMansion:
            return "woodland_mansion";
        case StructureType::Fortress:
            return "fortress";
        case StructureType::Bastion:
            return "bastion";
        case StructureType::EndCity:
            return "end_city";
        default:
            return nullptr;
    }
}

} // anonymous namespace

std::optional<BlockPos> ServerWorld::findNearestStructure(
    const BlockPos& center, world::gen::structure::StructureType structureType, i32 maxDistance, bool skipExisting)
{
    MC_UNUSED(skipExisting); // 当前实现不使用此参数

    // 从注册表获取结构定义
    const char* structureName = structureTypeToName(structureType);
    if (structureName == nullptr) {
        return std::nullopt;
    }

    const world::gen::structure::Structure* structure = world::gen::structure::StructureRegistry::get(structureName);
    if (structure == nullptr) {
        return std::nullopt;
    }

    // 获取区块生成器
    IChunkGenerator* generator = m_chunkManager ? m_chunkManager->generator() : nullptr;
    if (generator == nullptr) {
        return std::nullopt;
    }

    // 获取结构间距设置
    auto settings = structure->separationSettings();

    // 将方块坐标转换为区块坐标
    i32 centerChunkX = center.x >> 4;
    i32 centerChunkZ = center.z >> 4;

    // 将最大距离转换为区块范围
    // MC 1.16.5: maxDistance 是区块数的平方根
    i32 chunkRadius = (maxDistance + 15) >> 4; // 向上取整到区块

    // 参考 MC 1.16.5 Structure.func_236388_a_
    // 螺旋搜索：从中心向外扩展
    i32 spacing = settings.spacing;
    i64 worldSeed = static_cast<i64>(m_config.seed);

    std::optional<BlockPos> nearestPos;
    f64 nearestDistSq = static_cast<f64>(maxDistance * maxDistance) + 1.0;

    // 创建共享随机数生成器
    math::Random rng;

    // 螺旋搜索
    for (i32 l = 0; l <= chunkRadius; ++l) {
        for (i32 i1 = -l; i1 <= l; ++i1) {
            bool isEdge1 = (i1 == -l || i1 == l);

            for (i32 j1 = -l; j1 <= l; ++j1) {
                bool isEdge2 = (j1 == -l || j1 == l);

                // 只处理边缘（螺旋的外围）
                if (!isEdge1 && !isEdge2) {
                    continue;
                }

                // 计算候选区块坐标（按 spacing 缩放）
                i32 candidateChunkX = centerChunkX + spacing * i1;
                i32 candidateChunkZ = centerChunkZ + spacing * j1;

                // 使用结构静态方法检查是否在此区块生成结构
                i32 startX, startZ;
                bool hasStructure = world::gen::structure::Structure::findStructureStart(worldSeed,
                    candidateChunkX,
                    candidateChunkZ,
                    settings,
                    startX,
                    startZ,
                    structure->useUniformSpacing());

                if (!hasStructure) {
                    continue;
                }

                // 检查该区块是否已加载或可加载
                // 尝试获取区块来验证结构是否实际存在
                const ChunkData* chunk = getChunk(startX >> 4, startZ >> 4);
                if (chunk != nullptr) {
                    // 检查区块是否有该结构的起点
                    // 注意：如果区块已生成，结构起点应该在 ChunkPrimer 中
                    // 但 ChunkData 可能没有这个信息，所以我们使用种子计算的位置
                }

                // 计算距离
                i32 dx = startX - center.x;
                i32 dz = startZ - center.z;
                f64 distSq = static_cast<f64>(dx * dx + dz * dz);

                // 检查是否在范围内且比之前找到的更近
                if (distSq < nearestDistSq) {
                    nearestDistSq = distSq;
                    nearestPos = BlockPos(startX, 0, startZ);
                }
            }
        }
    }

    return nearestPos;
}

} // namespace mc::server
