#include "IChunk.hpp"
#include "../block/Block.hpp"
#include "ChunkData.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// BiomeContainer 实现
// ============================================================================

void BiomeContainer::setBiome(i32 x, i32 y, i32 z, BiomeId biome)
{
    if (x >= 0 && x < BIOME_WIDTH && y >= 0 && y < BIOME_HEIGHT && z >= 0 && z < BIOME_DEPTH) {
        const i32 index = y * BIOME_WIDTH * BIOME_DEPTH + z * BIOME_WIDTH + x;
        m_biomes[static_cast<size_t>(index)] = biome;
    }
}

BiomeId BiomeContainer::getBiome(i32 x, i32 y, i32 z) const
{
    if (x >= 0 && x < BIOME_WIDTH && y >= 0 && y < BIOME_HEIGHT && z >= 0 && z < BIOME_DEPTH) {
        const i32 index = y * BIOME_WIDTH * BIOME_DEPTH + z * BIOME_WIDTH + x;
        return m_biomes[static_cast<size_t>(index)];
    }
    return 0; // 默认生物群系
}

BiomeId BiomeContainer::getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    // 将方块坐标映射到生物群系采样点
    // 16 个方块对应 4 个采样点，所以每 4 个方块对应一个采样点
    const i32 biomeX = std::clamp(x >> 2, 0, BIOME_WIDTH - 1);
    const i32 biomeY = std::clamp(y >> 4, 0, BIOME_HEIGHT - 1); // Y 轴每段 16 方块
    const i32 biomeZ = std::clamp(z >> 2, 0, BIOME_DEPTH - 1);
    return getBiome(biomeX, biomeY, biomeZ);
}

std::vector<u8> BiomeContainer::serialize() const
{
    std::vector<u8> data;
    data.reserve(BIOME_SIZE * sizeof(BiomeId));
    for (BiomeId biome : m_biomes) {
        data.push_back(static_cast<u8>(biome & 0xFF));
        data.push_back(static_cast<u8>((biome >> 8) & 0xFF));
    }
    return data;
}

Result<BiomeContainer> BiomeContainer::deserialize(const u8* data, size_t size)
{
    const size_t expectedSize = BIOME_SIZE * sizeof(BiomeId);
    if (size < expectedSize) {
        return Error(ErrorCode::InvalidArgument, "BiomeContainer deserialize: data too small");
    }

    BiomeContainer container;
    for (size_t i = 0; i < BIOME_SIZE; ++i) {
        const u16 low = data[i * 2];
        const u16 high = data[i * 2 + 1];
        container.m_biomes[i] = static_cast<BiomeId>(low | (high << 8));
    }
    return container;
}

// ============================================================================
// Heightmap 实现
// ============================================================================

Heightmap::Heightmap(HeightmapType type)
    : m_type(type)
{
    m_heights.fill(0);
}

bool Heightmap::update(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= 16 || z < 0 || z >= 16) {
        return false;
    }

    const i32 index = z * 16 + x;
    const BlockCoord currentHeight = m_heights[static_cast<size_t>(index)];

    // 只有当新方块高于当前高度且是阻挡方块时才更新
    if (y >= currentHeight && isOpaque(state)) {
        m_heights[static_cast<size_t>(index)] = y + 1; // 高度图存储的是 Y+1（即上方空气方块的位置）
        return true;
    }

    return false;
}

BlockCoord Heightmap::getHeight(BlockCoord x, BlockCoord z) const
{
    if (x < 0 || x >= 16 || z < 0 || z >= 16) {
        return 0;
    }
    const i32 index = z * 16 + x;
    return m_heights[static_cast<size_t>(index)];
}

void Heightmap::setData(const std::array<BlockCoord, SIZE>& data)
{
    m_heights = data;
}

bool Heightmap::isOpaque(const BlockState* state) const
{
    if (!state) {
        return false;
    }

    // 获取方块
    const Block& block = state->owner();

    switch (m_type) {
        case HeightmapType::WorldSurface:
        case HeightmapType::WorldSurfaceWG:
            // 最高非空气方块
            return !block.isAir(*state);

        case HeightmapType::OceanFloor:
        case HeightmapType::OceanFloorWG:
            // 最高固体方块（排除水、岩浆等流体）
            return block.isSolid(*state);

        case HeightmapType::MotionBlocking:
            // 阻挡运动的方块（固体、水、岩浆等）
            return block.isSolid(*state) || state->isLiquid();

        case HeightmapType::MotionBlockingNoLeaves:
            // 阻挡运动但不包括树叶
            return (block.isSolid(*state) || state->isLiquid()) && (&block.material() != &Material::LEAVES) &&
                (&block.material() != &Material::PLANT);

        case HeightmapType::LightBlocking:
            // 阻挡光照的方块（不透明方块，透明度 > 0）
            // 参考 MC 1.16.5: 阻挡光照传播的方块
            return block.isSolid(*state) && state->getOpacity() > 0;

        default:
            return !block.isAir(*state);
    }
}

} // namespace mc
