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

#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "server/world/chunk/pipeline/ChunkStatus.hpp"
#include "server/world/gen/carver/CarvingMask.hpp"
#include "server/world/gen/density/NoiseChunk.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace mc::trace;

namespace mc::world::chunk {

namespace {

constexpr std::array<std::pair<HeightmapType, HeightmapFlag>, HEIGHTMAP_TYPE_COUNT> _heightmapMappings{{
    {HeightmapType::WorldSurfaceWG, HeightmapFlag::WORLD_SURFACE_WG},
    {HeightmapType::OceanFloorWG, HeightmapFlag::OCEAN_FLOOR_WG},
    {HeightmapType::WorldSurface, HeightmapFlag::WORLD_SURFACE},
    {HeightmapType::OceanFloor, HeightmapFlag::OCEAN_FLOOR},
    {HeightmapType::MotionBlocking, HeightmapFlag::MOTION_BLOCKING},
    {HeightmapType::MotionBlockingNoLeaves, HeightmapFlag::MOTION_BLOCKING_NO_LEAVES},
    {HeightmapType::LightBlocking, HeightmapFlag::LIGHT_BLOCKING},
}};

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

ChunkPrimer::ChunkPrimer(ChunkCoord x, ChunkCoord z)
    : m_memTrack(this)
    , m_x(x)
    , m_z(z)
    , m_data(std::make_shared<ChunkData>(x, z))
    , m_chunkStatus(&ChunkStatuses::EMPTY)
    , m_persistedStatus(&ChunkStatuses::EMPTY)
    , m_status(ChunkLoadStatus::Empty)
{}

ChunkPrimer::ChunkPrimer(std::unique_ptr<ChunkData> data)
    : m_memTrack(this)
    , m_x(data->x())
    , m_z(data->z())
    , m_data(std::move(data))
    , m_chunkStatus(&ChunkStatuses::FULL)
    , m_persistedStatus(&ChunkStatuses::FULL)
    , m_status(ChunkLoadStatus::Loaded)
{
    m_data->initializeMissingHeightmaps();
    m_data->getBiomes().compact();
}

ChunkPrimer::ChunkPrimer(std::shared_ptr<ChunkData> data)
    : m_memTrack(this)
    , m_x(data->x())
    , m_z(data->z())
    , m_data(std::move(data))
    , m_chunkStatus(&ChunkStatuses::FULL)
    , m_persistedStatus(&ChunkStatuses::FULL)
    , m_status(ChunkLoadStatus::Loaded)
{
    m_data->initializeMissingHeightmaps();
    m_data->getBiomes().compact();
}

// ============================================================================
// 方块访问
// ============================================================================

const BlockState* ChunkPrimer::getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (!_isValidBlockCoord(x, y, z)) {
        return BlockRegistry::instance().airState();
    }
    return m_data->getBlockState(x, y, z);
}

void ChunkPrimer::setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (!_isValidBlockCoord(x, y, z)) [[unlikely]] {
        return;
    }
    // 生成态 ChunkPrimer 由状态管线串行推进、尚未发布到 m_chunks，无并发读者，
    // 经无锁路径直写以消除每方块一次无竞争 shared_mutex 开销（FillNoiseCells 主要热点）。
    // 安全性前提详见 ChunkData.hpp setBlockStateUnlocked 注释。
    //
    // 生成写入跳过运行时整列重扫，由本阶段的高度图标志增量维护同一份权威存储。
    m_data->_setBlockStateUnlockedGen(x, y, z, state);
    m_modified = true;

    // setBlockState 根据当前 ChunkStatus.heightmapsAfter() 自动更新高度图。
    // placement 与运行时均读取 ChunkData 的权威高度图，更新范围由生成阶段决定。
    _updateHeightmapsForCurrentStatus(x, y, z, state);
}

u32 ChunkPrimer::getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (!_isValidBlockCoord(x, y, z)) {
        return 0; // Air
    }
    return m_data->getBlockStateId(x, y, z);
}

void ChunkPrimer::setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId)
{
    if (!_isValidBlockCoord(x, y, z)) {
        return;
    }
    // 生成态无锁直写 + 跳过 ChunkData::updateHeightMap 整列重扫，同 setBlockState（见其注释）。
    m_data->_setBlockStateIdUnlockedGen(x, y, z, stateId);
    m_modified = true;

    // 与 setBlockState 相同，需要根据当前状态更新高度图
    // 按当前阶段维护 ChunkData 中的权威槽位。
    const BlockState* state = m_data->getBlockState(x, y, z);
    _updateHeightmapsForCurrentStatus(x, y, z, state);
}

// ============================================================================
// 区块段访问
// ============================================================================

ChunkSection* ChunkPrimer::getSection(i32 index)
{
    return m_data->getSection(index);
}

const ChunkSection* ChunkPrimer::getSection(i32 index) const
{
    return m_data->getSection(index);
}

bool ChunkPrimer::hasSection(i32 index) const
{
    return m_data->hasSection(index);
}

ChunkSection* ChunkPrimer::createSection(i32 index)
{
    m_modified = true;
    return m_data->createSection(index);
}

std::array<const ChunkSection*, mc::world::CHUNK_SECTIONS> ChunkPrimer::getSections() const
{
    return m_data->getSections();
}

// ============================================================================
// 高度图
// ============================================================================

BlockCoord ChunkPrimer::getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    const BlockCoord height = getHeightmap(type).getHeight(x, z);
    return height != Heightmap::NO_BLOCK_SENTINEL ? height - 1 : mc::world::MIN_BUILD_HEIGHT;
}

BlockCoord ChunkPrimer::getHeightmapFirstAvailable(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    return getHeightmap(type).getHeight(x, z);
}

void ChunkPrimer::updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    m_data->updateHeightmap(type, x, y, z, state);
}

// ============================================================================
// 生成阶段管理
// ============================================================================

void ChunkPrimer::setChunkStatus(const ChunkStatus& status)
{
    m_chunkStatus = &status;
    m_modified = true;
}

void ChunkPrimer::setPersistedStatus(const ChunkStatus& target)
{
    // ProtoChunk.setPersistedStatus()
    // 只允许向前推进
    if (target.isAfter(*m_persistedStatus)) {
        m_persistedStatus = &target;
    }
    // 同时推进 chunkStatus（如果 chunkStatus 落后于 persistedStatus）
    if (m_chunkStatus->isBefore(target)) {
        m_chunkStatus = &target;
    }
    m_modified = true;
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId ChunkPrimer::getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    return getBiomes().getBiomeAtBlock(x, y, z);
}

// ============================================================================
// 高度图管理
// ============================================================================

Heightmap& ChunkPrimer::getHeightmap(HeightmapType type)
{
    return m_data->getHeightmap(type);
}

const Heightmap& ChunkPrimer::getHeightmap(HeightmapType type) const
{
    return std::as_const(*m_data).getHeightmap(type);
}

void ChunkPrimer::updateAllHeightmaps()
{
    std::array<bool, HEIGHTMAP_TYPE_COUNT> enabled;
    enabled.fill(true);
    m_data->rebuildHeightmaps(enabled);
}

void ChunkPrimer::markPosForPostprocessing(BlockCoord x, BlockCoord y, BlockCoord z)
{
    // ProtoChunk.markPosForPostprocessing
    // 将位置打包为短整型并按区块段索引存储
    const i32 sectionIndex = mc::world::toSectionIndex(y);
    if (sectionIndex >= 0 && sectionIndex < mc::world::CHUNK_SECTIONS) [[likely]] {
        const u16 packed = packToLocal(x, y, z);
        m_postProcessingSections[sectionIndex].push_back(packed);
    }
}

void ChunkPrimer::addPackedPostProcessing(const std::vector<u16>& packedPositions, i32 sectionIndex)
{
    if (sectionIndex >= 0 && sectionIndex < mc::world::CHUNK_SECTIONS) {
        auto& section = m_postProcessingSections[sectionIndex];
        section.reserve(section.size() + packedPositions.size());
        section.insert(section.end(), packedPositions.begin(), packedPositions.end());
    }
}

void ChunkPrimer::addPendingBlockTick(
    const BlockPos& pos, const Block& block, u64 scheduledTick, i32 delay, world::tick::TickPriority priority)
{
    // 对齐 ServerTickList::scheduleTick 的去重语义：同一位置 + 同一方块只保留最早的一条，
    // 重复调度忽略（保留已有条目的时间与优先级）。
    for (const auto& tick : m_pendingBlockTicks) {
        if (tick.pos == pos && tick.block == &block) {
            return;
        }
    }
    m_pendingBlockTicks.push_back(PendingBlockTick{pos, &block, scheduledTick, delay > 0 ? delay : 0, priority});
}

void ChunkPrimer::addPendingFluidTick(
    const BlockPos& pos, const fluid::Fluid& fluid, u64 scheduledTick, i32 delay, world::tick::TickPriority priority)
{
    // 与方块刻一致：同一位置 + 同一流体只保留最早的一条。
    for (const auto& tick : m_pendingFluidTicks) {
        if (tick.pos == pos && tick.fluid == &fluid) {
            return;
        }
    }
    m_pendingFluidTicks.push_back(PendingFluidTick{pos, &fluid, scheduledTick, delay > 0 ? delay : 0, priority});
}

void ChunkPrimer::primeHeightmaps(HeightmapFlag types)
{
    std::array<bool, HEIGHTMAP_TYPE_COUNT> enabled{};
    for (const auto& [type, flag] : _heightmapMappings) {
        enabled[static_cast<size_t>(type)] = hasFlag(types, flag);
    }
    m_data->rebuildHeightmaps(enabled);
}

// ============================================================================
// 转换方法
// ============================================================================

std::shared_ptr<ChunkData> ChunkPrimer::toChunkData()
{
    MC_ASSERT_RELEASE(!m_data->isFullyGenerated());
    updateAllHeightmaps();
    m_data->getBiomes().compact();

    m_data->setFullyGenerated(true);
    m_data->setStatus(ChunkLoadStatus::Generated); // 设置 ChunkData 的状态

    // 将后处理位置从 ProtoChunk 传输到 LevelChunk
    m_data->addPackedPostProcessing(m_postProcessingSections);

    // 数据已复制到 ChunkData，释放 primer 中的后处理位置向量
    for (auto& section : m_postProcessingSections) {
        section.clear();
        section.shrink_to_fit();
    }

    // 将待执行方块刻/流体刻转移到 ChunkData（区块发布后由 ServerChunkManager 转交 TickManager）
    m_data->setPendingBlockTicks(std::exchange(m_pendingBlockTicks, {}));
    m_data->setPendingFluidTicks(std::exchange(m_pendingFluidTicks, {}));

    // 设置状态
    m_status = ChunkLoadStatus::Generated;
    m_chunkStatus = &ChunkStatuses::FULL;

    // 清空生成的实体数据（调用者应该在调用此方法之前提取）
    m_spawnedEntities.clear();

    // 非破坏性：返回 m_data 的共享副本，ChunkPrimer 仍持有同一份 ChunkData。
    // 对齐 Moonrise：FULL 完成后 currentChunk（ChunkPrimer）仍存活供邻居引用，
    // 直到 holder 卸载；同一份 ChunkData 发布到内存缓存供游戏逻辑访问。
    return m_data;
}

void ChunkPrimer::releaseGenOnlyData(const ChunkStatus& afterStatus)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen,
        "ChunkPrimer::releaseGenOnlyData",
        "x",
        m_x,
        "z",
        m_z,
        "afterStatus",
        afterStatus.name(),
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(m_x, m_z).toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

    // CARVERS（含）之后释放 m_noiseChunk 和 m_carvingMask
    // 审计结论（NoiseChunkGenerator.cpp）：
    //   - m_noiseChunk 最后在 applyCarvers（CARVERS）中读取（aquifer/CarvingContext）
    //   - m_carvingMask 仅在 applyCarvers 中使用（WorldCarver::carve 读写 isCarved/setCarved）
    //   FEATURES/LIGHT/SPAWN/FULL 阶段无任何读取，可安全释放
    if (afterStatus.ordinal() >= ChunkStatuses::CARVERS_ORDINAL) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen,
            "ChunkPrimer::releaseGenOnlyData::ReleaseNoiseAndCarvingMask",
            "x",
            m_x,
            "z",
            m_z);
        m_noiseChunk.reset();
        m_carvingMask.reset();
    }

    // FEATURES（含）之后释放 m_structureReferences
    // 审计结论：
    //   - m_structureReferences 最后在 placeFeatures（FEATURES）中读取（查找要放置的结构）
    //     _buildBeardifier 在 BIOMES/NOISE 阶段读取，早于 FEATURES
    //   - INITIALIZE_LIGHT/LIGHT/SPAWN/FULL 阶段无任何读取
    if (afterStatus.ordinal() >= ChunkStatuses::FEATURES_ORDINAL) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.World.ChunkGen,
            "ChunkPrimer::releaseGenOnlyData::ReleaseStructureReferences",
            "x",
            m_x,
            "z",
            m_z);
        m_structureReferences.clear();
        m_structureReferences.rehash(0); // 释放桶内存
    }

    // m_postProcessingSections：在 toChunkData（FULL 阶段）复制到 ChunkData 后释放
    //   FEATURES 阶段后 markPosForPostprocessing 不再写入，但数据需要在 toChunkData 时转移
    //   因此不在此处释放，而是在 toChunkData 中清空

    // m_structureStarts：不能释放——邻居在 STRUCTURE_REFERENCES（半径8）、FEATURES、
    //   _buildBeardifier（BIOMES/NOISE）中通过 getIntersectingStructures/getStructureStart 读取。
    //   必须存活到 holder 卸载。
}

// ============================================================================
// 静态工具方法
// ============================================================================

u16 ChunkPrimer::packToLocal(BlockCoord x, BlockCoord y, BlockCoord z) noexcept
{
    return static_cast<u16>((x & mc::world::CHUNK_MASK) | ((y & mc::world::CHUNK_MASK) << mc::world::SECTION_SHIFT) |
        ((z & mc::world::CHUNK_MASK) << (mc::world::SECTION_SHIFT * 2)));
}

void ChunkPrimer::unpackFromLocal(
    u16 packed, i32 yOffset, ChunkCoord chunkX, ChunkCoord chunkZ, BlockCoord& x, BlockCoord& y, BlockCoord& z) noexcept
{
    x = (packed & mc::world::CHUNK_MASK) + (chunkX << mc::world::CHUNK_SHIFT);
    y = ((packed >> mc::world::SECTION_SHIFT) & mc::world::CHUNK_MASK) + (yOffset << mc::world::SECTION_SHIFT);
    z = ((packed >> (mc::world::SECTION_SHIFT * 2)) & mc::world::CHUNK_MASK) + (chunkZ << mc::world::CHUNK_SHIFT);
}

// ============================================================================
// 辅助方法
// ============================================================================

bool ChunkPrimer::_isValidBlockCoord(BlockCoord x, BlockCoord y, BlockCoord z) noexcept
{
    return x >= 0 && x < mc::world::CHUNK_WIDTH && y >= mc::world::MIN_BUILD_HEIGHT &&
        y < mc::world::MAX_BUILD_HEIGHT && z >= 0 && z < mc::world::CHUNK_WIDTH;
}

void ChunkPrimer::_updateHeightmapsForCurrentStatus(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    const HeightmapFlag flags = m_persistedStatus->heightmaps();
    for (const auto& [type, flag] : _heightmapMappings) {
        if (hasFlag(flags, flag)) {
            m_data->updateHeightmap(type, x, y, z, state);
        }
    }
}

// ============================================================================
// 雕刻掩码
// ============================================================================

CarvingMask& ChunkPrimer::carvingMask(i32 minY, i32 height)
{
    if (!m_carvingMask) {
        m_carvingMask = std::make_unique<CarvingMask>(minY, height);
    } else {
        // 掩码在本次生成中只创建一次；几何必须与创建时一致，否则索引会整体错位。
        MC_ASSERT_RELEASE(m_carvingMask->getMinY() == minY);
        MC_ASSERT_RELEASE(m_carvingMask->getHeight() == height);
    }
    return *m_carvingMask;
}

mc::world::gen::density::NoiseChunk& ChunkPrimer::getOrCreateNoiseChunk(
    std::function<std::unique_ptr<mc::world::gen::density::NoiseChunk>()> factory)
{
    if (!m_noiseChunk) {
        m_noiseChunk = factory();
    }
    return *m_noiseChunk;
}

ChunkPrimer::~ChunkPrimer() = default;

ChunkPrimer::ChunkPrimer(ChunkPrimer&& other) noexcept
    : m_memTrack() // 默认构造为非活跃，body 中重绑定
    , m_x(other.m_x)
    , m_z(other.m_z)
    , m_data(std::move(other.m_data))
    , m_chunkStatus(other.m_chunkStatus)
    , m_persistedStatus(other.m_persistedStatus)
    , m_status(other.m_status)
    , m_modified(other.m_modified)
    , m_spawnedEntities(std::move(other.m_spawnedEntities))
    , m_structureStarts(std::move(other.m_structureStarts))
    , m_structureReferences(std::move(other.m_structureReferences))
    , m_carvingMask(std::move(other.m_carvingMask))
    , m_postProcessingSections(std::move(other.m_postProcessingSections))
    , m_pendingBlockTicks(std::move(other.m_pendingBlockTicks))
    , m_pendingFluidTicks(std::move(other.m_pendingFluidTicks))
    , m_noiseChunk(std::move(other.m_noiseChunk))
{
    // 对象级追踪重绑定：释放源地址、分配目标地址（守卫不可移动，故在 body 处理，
    // 初始化列表中默认构造为非活跃）。若不重绑定，move 后源地址仍留在 Tracy 活跃集，
    // 堆复用该地址时触发 MemAllocTwice 硬失败。
    other.m_memTrack.unbind();
    m_memTrack.bind(this);
}

ChunkPrimer& ChunkPrimer::operator=(ChunkPrimer&& other) noexcept
{
    if (this != &other) {
        m_x = other.m_x;
        m_z = other.m_z;
        m_data = std::move(other.m_data);
        m_chunkStatus = other.m_chunkStatus;
        m_persistedStatus = other.m_persistedStatus;
        m_status = other.m_status;
        m_modified = other.m_modified;
        m_spawnedEntities = std::move(other.m_spawnedEntities);
        m_structureStarts = std::move(other.m_structureStarts);
        m_structureReferences = std::move(other.m_structureReferences);
        m_carvingMask = std::move(other.m_carvingMask);
        m_postProcessingSections = std::move(other.m_postProcessingSections);
        m_pendingBlockTicks = std::move(other.m_pendingBlockTicks);
        m_pendingFluidTicks = std::move(other.m_pendingFluidTicks);
        m_noiseChunk = std::move(other.m_noiseChunk);

        // 对象级追踪重绑定（同 move ctor 语义）：释放双方旧地址、目标重新绑定新地址
        m_memTrack.unbind();
        other.m_memTrack.unbind();
        m_memTrack.bind(this);
    }
    return *this;
}

} // namespace mc::world::chunk
