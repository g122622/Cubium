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
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/gen/carver/WorldCarver.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"

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

void initializeAllHeightmaps(std::unordered_map<HeightmapType, Heightmap>& heightmaps)
{
    for (HeightmapType type : ALL_HEIGHTMAP_TYPES) {
        heightmaps[type] = Heightmap(type);
    }
}

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

ChunkPrimer::ChunkPrimer(ChunkCoord x, ChunkCoord z)
    : m_x(x)
    , m_z(z)
    , m_data(std::make_shared<ChunkData>(x, z))
    , m_chunkStatus(&ChunkStatuses::EMPTY)
    , m_status(ChunkLoadStatus::Empty)
{
    initializeAllHeightmaps(m_heightmaps);
}

ChunkPrimer::ChunkPrimer(std::unique_ptr<ChunkData> data)
    : m_x(data->x())
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

const ChunkSection* const* ChunkPrimer::getSections() const
{
    return m_data->getSections();
}

// ============================================================================
// 高度图
// ============================================================================

BlockCoord ChunkPrimer::getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    auto it = m_heightmaps.find(type);
    if (it != m_heightmaps.end()) {
        // Heightmap 内部保存的是"最高方块上方一格"的 Y，所以这里要减 1
        // 才是实际的方块坐标。OceanFloorWG 也遵循同一语义。
        return it->second.getHeight(x, z) - 1;
    }
    MC_ASSERT_RELEASE(false);
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
    auto it = m_heightmaps.find(type);
    if (it == m_heightmaps.end()) {
        it = m_heightmaps.emplace(type, Heightmap(type)).first;
    }
    return it->second;
}

const Heightmap& ChunkPrimer::getHeightmap(HeightmapType type) const
{
    static Heightmap dummy(HeightmapType::WorldSurface);
    auto it = m_heightmaps.find(type);
    return it != m_heightmaps.end() ? it->second : dummy;
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

                    auto& heightmap = m_heightmaps[ALL_HEIGHTMAP_TYPES[i]];
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
    // 先重置指定类型的高度图
    for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
        if (hasFlag(types, flag)) {
            auto it = m_heightmaps.find(type);
            if (it != m_heightmaps.end()) {
                it->second.setAll(mc::world::MAX_BUILD_HEIGHT);
            }
        }
    }

    // 从方块数据重新计算
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
                    auto it = m_heightmaps.find(type);
                    if (it != m_heightmaps.end()) {
                        it->second.update(x, y, z, state);
                    }
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

    // 标记为完全生成
    m_data->setBiomes(m_biomes);
    m_data->setFullyGenerated(true);
    m_data->setStatus(ChunkLoadStatus::Generated); // 设置 ChunkData 的状态

    // 将后处理位置从 ProtoChunk 传输到 LevelChunk
    m_data->addPackedPostProcessing(m_postProcessingSections);

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

    // 检查是否有尚未创建的高度图需要先 prime
    bool needsPrime = false;
    for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
        if (hasFlag(flags, flag) && m_heightmaps.find(type) == m_heightmaps.end()) {
            needsPrime = true;
            break;
        }
    }

    // 如果有缺失的高度图，先从方块数据初始化它们
    if (needsPrime) {
        primeHeightmaps(flags);
        return; // primeHeightmaps 已经完整计算了所有指定类型的高度图
    }

    // 增量更新已存在的高度图
    for (const auto& [type, flag] : HEIGHTMAP_MAPPINGS) {
        if (!hasFlag(flags, flag)) {
            continue;
        }
        auto it = m_heightmaps.find(type);
        if (it != m_heightmaps.end()) {
            it->second.update(x, y, z, state);
        }
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

} // namespace mc::world::chunk
