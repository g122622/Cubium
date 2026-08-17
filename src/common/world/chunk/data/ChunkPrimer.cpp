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

#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::chunk {

namespace {

constexpr std::array<HeightmapType, 7> ALL_HEIGHTMAP_TYPES = {
    HeightmapType::WorldSurface,
    HeightmapType::OceanFloor,
    HeightmapType::MotionBlocking,
    HeightmapType::MotionBlockingNoLeaves,
    HeightmapType::WorldSurfaceWG,
    HeightmapType::OceanFloorWG,
    HeightmapType::LightBlocking,
};

struct TypeAndFlag {
    HeightmapType type;
    HeightmapFlag flag;
};

static const TypeAndFlag HEIGHTMAP_MAPPINGS[] = {
    {HeightmapType::WorldSurfaceWG, HeightmapFlag::WORLD_SURFACE_WG},
    {HeightmapType::OceanFloorWG, HeightmapFlag::OCEAN_FLOOR_WG},
    {HeightmapType::WorldSurface, HeightmapFlag::WORLD_SURFACE},
    {HeightmapType::OceanFloor, HeightmapFlag::OCEAN_FLOOR},
    {HeightmapType::MotionBlocking, HeightmapFlag::MOTION_BLOCKING},
    {HeightmapType::MotionBlockingNoLeaves, HeightmapFlag::MOTION_BLOCKING_NO_LEAVES},
    {HeightmapType::LightBlocking, HeightmapFlag::LIGHT_BLOCKING},
};

void initializeAllHeightmaps(std::array<Heightmap, HEIGHTMAP_TYPE_COUNT>& heightmaps)
{
    // 按 index 反向 cast 回 HeightmapType 设类型，与 ChunkData::_initHeightmaps 一致。
    for (size_t i = 0; i < heightmaps.size(); ++i) {
        heightmaps[i] = Heightmap(static_cast<HeightmapType>(i));
    }
}

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
    , m_status(ChunkLoadStatus::Empty)
{
    initializeAllHeightmaps(m_heightmaps);
}

ChunkPrimer::ChunkPrimer(std::unique_ptr<ChunkData> data)
    : m_memTrack(this)
    , m_x(data->x())
    , m_z(data->z())
    , m_data(std::move(data))
    , m_chunkStatus(&ChunkStatuses::FULL)
    , m_status(ChunkLoadStatus::Loaded)
{
    MC_ASSERT_RELEASE(m_data != nullptr);
    initializeAllHeightmaps(m_heightmaps);
    updateAllHeightmaps();
}

ChunkPrimer::ChunkPrimer(std::shared_ptr<ChunkData> data)
    : m_memTrack(this)
    , m_x(data->x())
    , m_z(data->z())
    , m_data(std::move(data))
    , m_chunkStatus(&ChunkStatuses::FULL)
    , m_status(ChunkLoadStatus::Loaded)
{
    MC_ASSERT_RELEASE(m_data != nullptr);
    initializeAllHeightmaps(m_heightmaps);
    updateAllHeightmaps();
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
    if (!_isValidBlockCoord(x, y, z)) {
        return;
    }
    m_data->setBlockState(x, y, z, state);
    m_modified = true;

    // setBlockState 根据当前 ChunkStatus.heightmapsAfter() 自动更新高度图
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
    m_data->setBlockStateId(x, y, z, stateId);
    m_modified = true;

    // 与 setBlockState 相同，需要根据当前状态更新高度图
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
    // Heightmap 内部保存的是"最高方块上方一格"的 Y+1，所以这里要减 1
    // 才是实际的方块坐标。OceanFloorWG 也遵循同一语义。
    // 无方块列返回 NO_BLOCK_SENTINEL，回退为 MIN_BUILD_HEIGHT。
    const Heightmap& heightmap = m_heightmaps[static_cast<size_t>(type)];
    const BlockCoord height = heightmap.getHeight(x, z);
    return height != Heightmap::NO_BLOCK_SENTINEL ? height - 1 : mc::world::MIN_BUILD_HEIGHT;
}

BlockCoord ChunkPrimer::getHeightmapFirstAvailable(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    // 直接返回 Heightmap 内部存储值（最高方块 Y+1，或 NO_BLOCK_SENTINEL 表示空列），
    // 不做空列→MIN_BUILD_HEIGHT 的合并，供 HeightmapPlacement 等需要精确识别空列的
    // 调用方使用（对齐 MC Heightmap.getFirstAvailable）。
    const Heightmap& heightmap = m_heightmaps[static_cast<size_t>(type)];
    return heightmap.getHeight(x, z);
}

void ChunkPrimer::updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    auto& heightmap = getHeightmap(type);
    heightmap.update(x, y, z, state);
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
    return m_biomes.getBiomeAtBlock(x, y, z);
}

// ============================================================================
// 光源位置
// ============================================================================

void ChunkPrimer::addLightPosition(BlockCoord x, BlockCoord y, BlockCoord z)
{
    m_lightPositions.push_back(x);
    m_lightPositions.push_back(y);
    m_lightPositions.push_back(z);
}

// ============================================================================
// 高度图管理
// ============================================================================

Heightmap& ChunkPrimer::getHeightmap(HeightmapType type)
{
    return m_heightmaps[static_cast<size_t>(type)];
}

const Heightmap& ChunkPrimer::getHeightmap(HeightmapType type) const
{
    return m_heightmaps[static_cast<size_t>(type)];
}

void ChunkPrimer::updateAllHeightmaps()
{
    // 每次重建前先重置所有高度图，避免雕刻/替换方块后残留旧高度。
    initializeAllHeightmaps(m_heightmaps);

    for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
            std::array<bool, ALL_HEIGHTMAP_TYPES.size()> resolved{};
            i32 unresolvedCount = static_cast<i32>(ALL_HEIGHTMAP_TYPES.size());

            for (i32 y = mc::world::MAX_BUILD_HEIGHT - 1; y >= mc::world::MIN_BUILD_HEIGHT; --y) {
                if (unresolvedCount <= 0) {
                    break;
                }

                const BlockState* state = m_data->getBlockState(x, y, z);
                if (!state || state->isAir()) {
                    continue;
                }

                for (size_t i = 0; i < ALL_HEIGHTMAP_TYPES.size(); ++i) {
                    if (resolved[i]) {
                        continue;
                    }

                    auto& heightmap = m_heightmaps[static_cast<size_t>(ALL_HEIGHTMAP_TYPES[i])];
                    if (heightmap.update(x, y, z, state)) {
                        resolved[i] = true;
                        --unresolvedCount;
                    }
                }
            }
        }
    }
}

void ChunkPrimer::markPosForPostprocessing(BlockCoord x, BlockCoord y, BlockCoord z)
{
    // ProtoChunk.markPosForPostprocessing
    // 将位置打包为短整型并按区块段索引存储
    const i32 sectionIndex = mc::world::toSectionIndex(y);
    if (sectionIndex >= 0 && sectionIndex < mc::world::CHUNK_SECTIONS) {
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

void ChunkPrimer::initializeLightSources()
{
    // INITIALIZE_LIGHT 阶段：遍历区块中所有方块，找到亮度 > 0 的方块
    // 注册到光照引擎。光照系统完整集成后，此处应将光源位置注册到 WorldLightManager。
    // 当前实现：标记方块级光源位置到 ChunkData 的 nibble array 中
    for (i32 sectionY = 0; sectionY < mc::world::CHUNK_SECTIONS; ++sectionY) {
        for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
            for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
                for (i32 y = 0; y < mc::world::CHUNK_SECTION_HEIGHT; ++y) {
                    const i32 worldY = sectionY * mc::world::CHUNK_SECTION_HEIGHT + y + mc::world::MIN_BUILD_HEIGHT;
                    const BlockState* state = m_data->getBlockState(x, worldY, z);
                    if (state && state->lightLevel() > 0) {
                        // 标记此位置的方块光照到区块光照数据
                        m_data->setBlockLight(x, worldY, z, state->lightLevel());
                    }
                }
            }
        }
    }
}

void ChunkPrimer::primeHeightmaps(HeightmapFlag types)
{
    // 先重置指定类型的高度图为"无方块"（哨兵值）
    for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
        if (hasFlag(types, flag)) {
            m_heightmaps[static_cast<size_t>(type)].setAll(Heightmap::NO_BLOCK_SENTINEL);
        }
    }

    // 从方块数据重新计算：自顶向下扫描，第一个阻挡方块写入 Y+1。
    // update 内部检查 y >= currentHeight，currentHeight 为哨兵(MIN_BUILD_HEIGHT-1)时
    // 任何合法 y 都满足条件，从而正确记录最高方块。
    for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
            for (i32 y = mc::world::MAX_BUILD_HEIGHT - 1; y >= mc::world::MIN_BUILD_HEIGHT; --y) {
                const BlockState* state = m_data->getBlockState(x, y, z);
                if (!state || state->isAir()) {
                    continue;
                }

                for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
                    if (!hasFlag(types, flag)) {
                        continue;
                    }
                    m_heightmaps[static_cast<size_t>(type)].update(x, y, z, state);
                }
            }
        }
    }
}

// ============================================================================
// 转换方法
// ============================================================================

std::shared_ptr<ChunkData> ChunkPrimer::toChunkData()
{
    // 确保高度图已更新
    updateAllHeightmaps();

    // 同步 primer 的全部高度图到 ChunkData。此前 primer 的 m_heightmaps（array）与
    // m_data 的 m_heightmaps（array）是两套存储，生成路径只更新 primer 侧，
    // 导致 ChunkData 的 final 槽位 m_heightmapInitialized 恒为 false、getTopBlockY 回退 WorldSurface。
    // setHeightmapFromStorage 绕过 _isOpaque 整列写入并标记已初始化。
    for (size_t i = 0; i < m_heightmaps.size(); ++i) {
        m_data->setHeightmapFromStorage(static_cast<HeightmapType>(i), m_heightmaps[i].getData());
    }

    // 标记为完全生成
    m_data->setBiomes(m_biomes);
    m_data->setFullyGenerated(true);
    m_data->setStatus(ChunkLoadStatus::Generated); // 设置 ChunkData 的状态

    // 将后处理位置从 ProtoChunk 传输到 LevelChunk
    m_data->addPackedPostProcessing(m_postProcessingSections);

    // 数据已复制到 ChunkData，释放 primer 中的后处理位置向量
    for (auto& section : m_postProcessingSections) {
        section.clear();
        section.shrink_to_fit();
    }

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
    // CARVERS（含）之后释放 m_noiseChunk 和 m_carvingMask
    // 审计结论（NoiseChunkGenerator.cpp）：
    //   - m_noiseChunk 最后在 applyCarvers（CARVERS）中读取（aquifer/CarvingContext）
    //   - m_carvingMask 仅在 applyCarvers 中使用（WorldCarver::carve 读写 isCarved/setCarved）
    //   FEATURES/LIGHT/SPAWN/FULL 阶段无任何读取，可安全释放
    if (afterStatus.ordinal() >= ChunkStatuses::CARVERS_ORDINAL) {
        m_noiseChunk.reset();
        m_carvingMask.reset();
    }

    // FEATURES（含）之后释放 m_structureReferences
    // 审计结论：
    //   - m_structureReferences 最后在 placeFeatures（FEATURES）中读取（查找要放置的结构）
    //     _buildBeardifier 在 BIOMES/NOISE 阶段读取，早于 FEATURES
    //   - INITIALIZE_LIGHT/LIGHT/SPAWN/FULL 阶段无任何读取
    if (afterStatus.ordinal() >= ChunkStatuses::FEATURES_ORDINAL) {
        m_structureReferences.clear();
        m_structureReferences.rehash(0); // 释放桶内存
    }

    // m_postProcessingSections：在 toChunkData（FULL 阶段）复制到 ChunkData 后释放
    //   FEATURES 阶段后 markPosForPostprocessing 不再写入，但数据需要在 toChunkData 时转移
    //   因此不在此处释放，而是在 toChunkData 中清空

    // m_structureStarts：不能释放——邻居在 STRUCTURE_REFERENCES（半径8）、FEATURES、
    //   _buildBeardifier（BIOMES/NOISE）中通过 getIntersectingStructures/getStructureStart 读取。
    //   必须存活到 holder 卸载。

    // m_lightPositions：恒为空（addLightPosition 在生产代码中无调用），无需处理
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

    // 构造时已全量初始化全部 7 种高度图，槽位恒存在，无需 prime 探测。直接按当前
    // ChunkStatus 要求的类型集合做增量更新。
    for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
        if (!hasFlag(flags, flag)) {
            continue;
        }
        m_heightmaps[static_cast<size_t>(type)].update(x, y, z, state);
    }
}

// ============================================================================
// 雕刻掩码
// ============================================================================

CarvingMask& ChunkPrimer::carvingMask()
{
    if (!m_carvingMask) {
        m_carvingMask = std::make_unique<CarvingMask>(m_x, m_z);
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
    , m_biomes(std::move(other.m_biomes))
    , m_heightmaps(std::move(other.m_heightmaps))
    , m_lightPositions(std::move(other.m_lightPositions))
    , m_spawnedEntities(std::move(other.m_spawnedEntities))
    , m_structureStarts(std::move(other.m_structureStarts))
    , m_structureReferences(std::move(other.m_structureReferences))
    , m_carvingMask(std::move(other.m_carvingMask))
    , m_postProcessingSections(std::move(other.m_postProcessingSections))
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
        m_biomes = std::move(other.m_biomes);
        m_heightmaps = std::move(other.m_heightmaps);
        m_lightPositions = std::move(other.m_lightPositions);
        m_spawnedEntities = std::move(other.m_spawnedEntities);
        m_structureStarts = std::move(other.m_structureStarts);
        m_structureReferences = std::move(other.m_structureReferences);
        m_carvingMask = std::move(other.m_carvingMask);
        m_postProcessingSections = std::move(other.m_postProcessingSections);
        m_noiseChunk = std::move(other.m_noiseChunk);

        // 对象级追踪重绑定（同 move ctor 语义）：释放双方旧地址、目标重新绑定新地址
        m_memTrack.unbind();
        other.m_memTrack.unbind();
        m_memTrack.bind(this);
    }
    return *this;
}

} // namespace mc::world::chunk
