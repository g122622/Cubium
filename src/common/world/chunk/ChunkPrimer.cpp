#include "ChunkPrimer.hpp"
#include "../block/BlockRegistry.hpp"
#include "../WorldConstants.hpp"
#include <spdlog/spdlog.h>

namespace mc {

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
    , m_data(std::make_unique<ChunkData>(x, z))
    , m_chunkStatus(&ChunkStatuses::EMPTY)
    , m_status(ChunkLoadStatus::Empty)
{
    initializeAllHeightmaps(m_heightmaps);
    initializeCarvingMasks();
}

ChunkPrimer::ChunkPrimer(std::unique_ptr<ChunkData> data)
    : m_x(data ? data->x() : 0)
    , m_z(data ? data->z() : 0)
    , m_data(std::move(data))
    , m_chunkStatus(&ChunkStatuses::FULL)
    , m_status(ChunkLoadStatus::Loaded)
{
    initializeAllHeightmaps(m_heightmaps);
    if (m_data) {
        initializeCarvingMasks();
        updateAllHeightmaps();
    }
}

// ============================================================================
// 方块访问
// ============================================================================

const BlockState* ChunkPrimer::getBlock(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (!isValidBlockCoord(x, y, z)) {
        return BlockRegistry::instance().airState();
    }
    return m_data ? m_data->getBlock(x, y, z) : BlockRegistry::instance().airState();
}

void ChunkPrimer::setBlock(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (!isValidBlockCoord(x, y, z)) {
        return;
    }
    if (m_data) {
        m_data->setBlock(x, y, z, state);
        m_modified = true;
    }
}

u32 ChunkPrimer::getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (!isValidBlockCoord(x, y, z)) {
        return 0; // Air
    }
    return m_data ? m_data->getBlockStateId(x, y, z) : 0;
}

void ChunkPrimer::setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId)
{
    if (!isValidBlockCoord(x, y, z)) {
        return;
    }
    if (m_data) {
        m_data->setBlockStateId(x, y, z, stateId);
        m_modified = true;
    }
}

// ============================================================================
// 区块段访问
// ============================================================================

ChunkSection* ChunkPrimer::getSection(i32 index)
{
    return m_data ? m_data->getSection(index) : nullptr;
}

const ChunkSection* ChunkPrimer::getSection(i32 index) const
{
    return m_data ? m_data->getSection(index) : nullptr;
}

bool ChunkPrimer::hasSection(i32 index) const
{
    return m_data ? m_data->hasSection(index) : false;
}

ChunkSection* ChunkPrimer::createSection(i32 index)
{
    m_modified = true;
    return m_data ? m_data->createSection(index) : nullptr;
}

const ChunkSection* const* ChunkPrimer::getSections() const
{
    return m_data ? m_data->getSections() : nullptr;
}

// ============================================================================
// 高度图
// ============================================================================

BlockCoord ChunkPrimer::getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    auto it = m_heightmaps.find(type);
    if (it != m_heightmaps.end()) {
        // Heightmap 内部保存的是“最高方块上方一格”的 Y，所以这里要减 1
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
    if (!m_data) {
        return;
    }

    // 每次重建前先重置所有高度图，避免雕刻/替换方块后残留旧高度。
    initializeAllHeightmaps(m_heightmaps);

    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            std::array<bool, ALL_HEIGHTMAP_TYPES.size()> resolved{};
            i32 unresolvedCount = static_cast<i32>(ALL_HEIGHTMAP_TYPES.size());

            for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
                if (unresolvedCount <= 0) {
                    break;
                }

                const BlockState* state = m_data->getBlock(x, y, z);
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

// ============================================================================
// 转换方法
// ============================================================================

std::unique_ptr<ChunkData> ChunkPrimer::toChunkData()
{
    // 确保高度图已更新
    updateAllHeightmaps();

    // 标记为完全生成
    if (m_data) {
        m_data->setBiomes(m_biomes);
        m_data->setFullyGenerated(true);
        m_data->setStatus(ChunkLoadStatus::Generated);  // 设置 ChunkData 的状态
    }

    // 设置状态
    m_status = ChunkLoadStatus::Generated;
    m_chunkStatus = &ChunkStatuses::FULL;

    // 清空生成的实体数据（调用者应该在调用此方法之前提取）
    m_spawnedEntities.clear();

    return std::move(m_data);
}

// ============================================================================
// 静态工具方法
// ============================================================================

u16 ChunkPrimer::packToLocal(BlockCoord x, BlockCoord y, BlockCoord z)
{
    return static_cast<u16>((x & 0xF) | ((y & 0xF) << 4) | ((z & 0xF) << 8));
}

void ChunkPrimer::unpackFromLocal(u16 packed, i32 yOffset, ChunkCoord chunkX, ChunkCoord chunkZ,
                                   BlockCoord& x, BlockCoord& y, BlockCoord& z)
{
    x = (packed & 0xF) + (chunkX << 4);
    y = ((packed >> 4) & 0xF) + (yOffset << 4);
    z = ((packed >> 8) & 0xF) + (chunkZ << 4);
}

// ============================================================================
// 辅助方法
// ============================================================================

bool ChunkPrimer::isValidBlockCoord(BlockCoord x, BlockCoord y, BlockCoord z)
{
    return x >= 0 && x < world::CHUNK_WIDTH &&
           y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT &&
           z >= 0 && z < world::CHUNK_WIDTH;
}

void ChunkPrimer::initializeCarvingMasks()
{
    // 雕刻掩码大小为 16x16x256 = 65536
    constexpr size_t carvingMaskSize = world::CHUNK_WIDTH * world::CHUNK_WIDTH * world::CHUNK_HEIGHT;
    m_carvingMaskAir.resize(carvingMaskSize, false);
    m_carvingMaskLiquid.resize(carvingMaskSize, false);
}

} // namespace mc
