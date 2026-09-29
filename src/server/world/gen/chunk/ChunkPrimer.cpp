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
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
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

/**
 * @brief 从方块数据重建高度图（只处理 enabled 标记为 true 的类型）
 *
 * 生成路径（ChunkPrimer::updateAllHeightmaps）与存档加载路径
 * （ChunkPrimer::_rebuildHeightmapsIntoChunkData）共用本实现，避免两处判定逻辑漂移。
 *
 * 自顶向下扫描每列，第一个计入该类型的方块写入 Y+1；enabled 的类型全部判定完毕后
 * 提前跳出该列。enabled 为 false 的槽位保持原值不动——生成路径传全 true（等价于
 * 全量重建），存档路径只传尚未初始化的类型，从而保留存档里已有的真实高度图。
 */
void rebuildHeightmaps(const ChunkData& data,
    std::array<Heightmap, HEIGHTMAP_TYPE_COUNT>& heightmaps,
    const std::array<bool, HEIGHTMAP_TYPE_COUNT>& enabled)
{
    // 重建前先重置 enabled 的槽位，避免雕刻/替换方块后残留旧高度。
    // 按枚举值直接索引（enabled 与 heightmaps 都以 HeightmapType 为下标），
    // 不依赖 ALL_HEIGHTMAP_TYPES 的元素顺序。
    for (size_t i = 0; i < heightmaps.size(); ++i) {
        if (enabled[i]) {
            heightmaps[i] = Heightmap(static_cast<HeightmapType>(i));
        }
    }

    // resolved 的初始状态：enabled 的类型待判定（false），disabled 的类型直接视为已定值。
    // 每列从该模板出发，扫描中把命中的类型翻成 true，全翻完即可跳出该列。
    std::array<bool, ALL_HEIGHTMAP_TYPES.size()> initialResolved{};
    i32 totalUnresolved = 0;
    for (size_t i = 0; i < ALL_HEIGHTMAP_TYPES.size(); ++i) {
        const size_t typeIndex = static_cast<size_t>(ALL_HEIGHTMAP_TYPES[i]);
        initialResolved[i] = !enabled[typeIndex];
        if (enabled[typeIndex]) {
            ++totalUnresolved;
        }
    }
    if (totalUnresolved == 0) {
        return; // 无需重建任何类型
    }

    for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
            auto resolved = initialResolved;
            i32 unresolvedCount = totalUnresolved;

            for (i32 y = mc::world::MAX_BUILD_HEIGHT - 1; y >= mc::world::MIN_BUILD_HEIGHT; --y) {
                if (unresolvedCount <= 0) {
                    break;
                }

                const BlockState* state = data.getBlockState(x, y, z);
                if (!state || state->isAir()) {
                    continue;
                }

                for (size_t i = 0; i < ALL_HEIGHTMAP_TYPES.size(); ++i) {
                    if (resolved[i]) {
                        continue;
                    }

                    auto& heightmap = heightmaps[static_cast<size_t>(ALL_HEIGHTMAP_TYPES[i])];
                    if (heightmap.update(x, y, z, state)) {
                        resolved[i] = true;
                        --unresolvedCount;
                    }
                }
            }
        }
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
    , m_persistedStatus(&ChunkStatuses::EMPTY)
    , m_status(ChunkLoadStatus::Empty)
    , m_biomes(std::make_unique<BiomeContainer>())
    , m_heightmaps(std::make_unique<std::array<Heightmap, HEIGHTMAP_TYPE_COUNT>>())
{
    initializeAllHeightmaps(*m_heightmaps);
}

ChunkPrimer::ChunkPrimer(std::unique_ptr<ChunkData> data)
    : m_memTrack(this)
    , m_x(data->x())
    , m_z(data->z())
    , m_data(std::move(data))
    , m_chunkStatus(&ChunkStatuses::FULL)
    , m_persistedStatus(&ChunkStatuses::FULL)
    , m_status(ChunkLoadStatus::Loaded)
{
    _rebuildHeightmapsIntoChunkData();
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
    _rebuildHeightmapsIntoChunkData();
}

void ChunkPrimer::_rebuildHeightmapsIntoChunkData()
{
    MC_ASSERT_RELEASE(m_data != nullptr);

    // 只重建"尚未被填充"的类型：外来格式（Java Anvil / Bedrock）的高度图已由读取器
    // 经 setHeightmapFromStorage 写入并置位 m_heightmapInitialized，重算会覆盖存档真实值；
    // native 段格式不持久化高度图，对应槽位为未初始化，必须在此重建，否则
    // getTopBlockY 会回退到未初始化的 WorldSurface 槽位，使 ServerWorld::getHeight
    // 退化为 MIN_BUILD_HEIGHT。
    std::array<bool, HEIGHTMAP_TYPE_COUNT> enabled{};
    for (size_t i = 0; i < HEIGHTMAP_TYPE_COUNT; ++i) {
        enabled[i] = !m_data->isHeightmapInitialized(static_cast<HeightmapType>(i));
    }

    // 临时高度图数组只在构造期存在，写入 ChunkData 后随栈帧归还，不进入稳态驻留集。
    // 无需预初始化：rebuildHeightmaps 会重置所有 enabled 槽位，disabled 槽位不会被写入。
    std::array<Heightmap, HEIGHTMAP_TYPE_COUNT> rebuilt;
    rebuildHeightmaps(*m_data, rebuilt, enabled);

    // 存档区块已是完整持久化状态，重建结果整列写入 ChunkData 并置位 m_heightmapInitialized
    // （setHeightmapFromStorage 绕过 _isOpaque 判定）。
    for (size_t i = 0; i < rebuilt.size(); ++i) {
        if (!enabled[i]) {
            continue;
        }
        m_data->setHeightmapFromStorage(static_cast<HeightmapType>(i), rebuilt[i].getData());
    }
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
    // 使用 Gen 变体（跳过 ChunkData::updateHeightMap 整列重扫）：FULL 收尾之前，所有高度图
    // 读取都走 ChunkPrimer::m_heightmaps（下方 _updateHeightmapsForCurrentStatus 维护），
    // ChunkData::m_heightmaps 尚无人读；其 final 高度图由 primeHeightmaps/updateAllHeightmaps
    // 在 toChunkData 收尾时全量写入。跳过整列重扫省去每方块 384 次 getBlockState +
    // 5×384 次 Heightmap::update，是 setBlockState 单次耗时的 60-75%。
    // 详见 ChunkData.hpp _setBlockStateUnlockedGen 注释。
    m_data->_setBlockStateUnlockedGen(x, y, z, state);
    m_modified = true;

    // setBlockState 根据当前 ChunkStatus.heightmapsAfter() 自动更新高度图。
    // FEATURES 阶段 placement 读取本区块高度图，故收尾前必须维护 ChunkPrimer::m_heightmaps；
    // 收尾后本地副本已释放，_updateHeightmapsForCurrentStatus 会转而维护 ChunkData。
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
    // （收尾前维护 ChunkPrimer::m_heightmaps，收尾后转由 ChunkData 承接）
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
    // 本地高度图为空有两个来源：FULL 收尾（toChunkData 释放）与存档构造（从不分配）。
    // 两者此时 ChunkData 的 7 张高度图都已全量写入并置位 m_heightmapInitialized，委托即可。
    // 副作用：此后世界编辑走 ChunkData::updateHeightMap，邻居读到的是实时值而非冻结快照。
    if (!m_heightmaps) {
        return m_data->getTopBlockY(type, x, z);
    }
    // Heightmap 内部保存的是"最高方块上方一格"的 Y+1，所以这里要减 1
    // 才是实际的方块坐标。OceanFloorWG 也遵循同一语义。
    // 无方块列返回 NO_BLOCK_SENTINEL，回退为 MIN_BUILD_HEIGHT。
    const Heightmap& heightmap = (*m_heightmaps)[static_cast<size_t>(type)];
    const BlockCoord height = heightmap.getHeight(x, z);
    return height != Heightmap::NO_BLOCK_SENTINEL ? height - 1 : mc::world::MIN_BUILD_HEIGHT;
}

BlockCoord ChunkPrimer::getHeightmapFirstAvailable(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    // 本地高度图为空时委托 ChunkData，理由同 getTopBlockY。
    if (!m_heightmaps) {
        return m_data->getHeightmapFirstAvailable(type, x, z);
    }
    // 直接返回 Heightmap 内部存储值（最高方块 Y+1，或 NO_BLOCK_SENTINEL 表示空列），
    // 不做空列→MIN_BUILD_HEIGHT 的合并，供 HeightmapPlacement 等需要精确识别空列的
    // 调用方使用（对齐 MC Heightmap.getFirstAvailable）。
    const Heightmap& heightmap = (*m_heightmaps)[static_cast<size_t>(type)];
    return heightmap.getHeight(x, z);
}

void ChunkPrimer::updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    // 本地高度图为空时维护职责移交 ChunkData（含 m_heightmapInitialized 置位）
    if (!m_heightmaps) {
        m_data->updateHeightmap(type, x, y, z, state);
        return;
    }
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
    // 本地副本为空有两个来源：FULL 收尾（toChunkData 释放）与存档构造（从不分配，因为
    // 存档已带真实生物群系）。两者都委托底层 ChunkData。邻居在 FEATURES 阶段经
    // WorldGenRegion 逐点读取本区块的 biome，这条路径必须始终有效——若不置空存档路径的
    // 副本，这里会读到未填充的全 0 容器（生物群系 0）而非存档真实值。
    if (!m_biomes) {
        return m_data->getBiomeAtBlock(x, y, z);
    }
    return m_biomes->getBiomeAtBlock(x, y, z);
}

// ============================================================================
// 高度图管理
// ============================================================================

Heightmap& ChunkPrimer::getHeightmap(HeightmapType type)
{
    // 可变访问只在生成期（FULL 收尾之前）合法：收尾后本地高度图已释放，存档路径更是
    // 从未分配，两条路径对高度图的后续维护都由 ChunkData 承担（整列重算 / 构造期写入）。
    // 生成期调用者均早于 FULL。
    MC_ASSERT_RELEASE(m_heightmaps != nullptr);
    return (*m_heightmaps)[static_cast<size_t>(type)];
}

const Heightmap& ChunkPrimer::getHeightmap(HeightmapType type) const
{
    if (!m_heightmaps) {
        return m_data->getHeightmap(type);
    }
    return (*m_heightmaps)[static_cast<size_t>(type)];
}

void ChunkPrimer::updateAllHeightmaps()
{
    // 全量重建只在生成期（含 toChunkData 收尾）调用：收尾后本地高度图已释放，存档路径
    // 则在构造期用临时数组重建一次即可，都不需要（也不允许）再走这里。
    MC_ASSERT_RELEASE(m_heightmaps != nullptr);
    std::array<bool, HEIGHTMAP_TYPE_COUNT> enabled;
    enabled.fill(true);
    rebuildHeightmaps(*m_data, *m_heightmaps, enabled);
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

void ChunkPrimer::primeHeightmaps(HeightmapFlag types)
{
    // FEATURES 之前调用，本地高度图必然仍在
    MC_ASSERT_RELEASE(m_heightmaps != nullptr);
    auto& heightmaps = *m_heightmaps;

    // 先重置指定类型的高度图为"无方块"（哨兵值）
    for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
        if (hasFlag(types, flag)) {
            heightmaps[static_cast<size_t>(type)].setAll(Heightmap::NO_BLOCK_SENTINEL);
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
                    heightmaps[static_cast<size_t>(type)].update(x, y, z, state);
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
    // 收尾前本地副本必然仍在；收尾后二者被释放，二次调用属逻辑错误
    MC_ASSERT_RELEASE(m_heightmaps != nullptr);
    MC_ASSERT_RELEASE(m_biomes != nullptr);

    // 确保高度图已更新
    updateAllHeightmaps();

    // 同步 primer 的全部高度图到 ChunkData。此前 primer 的 m_heightmaps（array）与
    // m_data 的 m_heightmaps（array）是两套存储，生成路径只更新 primer 侧，
    // 导致 ChunkData 的 final 槽位 m_heightmapInitialized 恒为 false、getTopBlockY 回退 WorldSurface。
    // setHeightmapFromStorage 绕过 _isOpaque 整列写入并标记已初始化。
    for (size_t i = 0; i < m_heightmaps->size(); ++i) {
        m_data->setHeightmapFromStorage(static_cast<HeightmapType>(i), (*m_heightmaps)[i].getData());
    }

    // 标记为完全生成
    m_data->setBiomes(*m_biomes);
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

    // 释放 primer 侧的生物群系与高度图副本（合计约 10 KiB/区块）。二者已在上方全量写入
    // m_data 且逐位相同，此后的读取一律经 getBiomeAtBlock/getTopBlockY/
    // getHeightmapFirstAvailable 委托 m_data，邻居经 WorldGenRegion 读取仍得到有效数据。
    // 委托分支只看本地指针是否为空，故必须排在上面的拷贝之后：先置空再拷贝会读到空数据。
    m_biomes.reset();
    m_heightmaps.reset();

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
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(m_x, m_z).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

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

    // FULL 收尾后本地高度图已释放：后续（世界编辑触发的）方块写入改由 ChunkData 维护高度图。
    if (!m_heightmaps) {
        for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
            if (hasFlag(flags, flag)) {
                m_data->updateHeightmap(type, x, y, z, state);
            }
        }
        return;
    }

    // 构造时已全量初始化全部 7 种高度图，槽位恒存在，无需 prime 探测。直接按当前
    // ChunkStatus 要求的类型集合做增量更新。
    for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
        if (!hasFlag(flags, flag)) {
            continue;
        }
        (*m_heightmaps)[static_cast<size_t>(type)].update(x, y, z, state);
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
