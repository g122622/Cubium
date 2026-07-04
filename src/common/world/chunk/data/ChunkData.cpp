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

#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

namespace mc::world::chunk {

// ============================================================================
// ChunkData 实现
// ============================================================================

ChunkData::ChunkData()
{
    _initHeightmaps();
    initLightData();
}

ChunkData::ChunkData(ChunkCoord x, ChunkCoord z)
    : m_x(x)
    , m_z(z)
{
    _initHeightmaps();
    initLightData();
}

ChunkData::~ChunkData() = default;

void ChunkData::_initHeightmaps()
{
    // 按枚举顺序为每个槽位设置正确的类型，使 m_heightmaps[type] 直接可用
    for (size_t i = 0; i < m_heightmaps.size(); ++i) {
        m_heightmaps[i] = Heightmap(static_cast<HeightmapType>(i));
    }
    // WorldSurface 槽位始终视为已填充：构造后即为空高度图(全 0)，
    // 且 setBlockState/updateHeightMap 会持续维护它。
    m_heightmapInitialized.fill(false);
    m_heightmapInitialized[static_cast<size_t>(HeightmapType::WorldSurface)] = true;
}

const BlockState* ChunkData::getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return nullptr; // 空气
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return nullptr; // 空气
    }

    i32 localY = mc::world::toSectionLocalY(y);
    return section->getBlockState(x, localY, z);
}

void ChunkData::setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (!state || state->isAir()) {
            return; // 不需要创建段来设置空气
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = mc::world::toSectionLocalY(y);
    section->setBlockState(x, localY, z, state);
    m_dirty = true;

    // 更新高度图（WorldSurface 槽位作为快速查询缓存）
    // currentTop 为最高方块 Y（Heightmap 存储 Y+1，故 getHeight-1；无方块时为 0）
    const BlockCoord rawHeight = m_heightmaps[static_cast<size_t>(HeightmapType::WorldSurface)].getHeight(x, z);
    const BlockCoord currentTop = rawHeight > 0 ? rawHeight - 1 : 0;
    if (y >= currentTop) {
        updateHeightMap(x, z);
    }
}

u32 ChunkData::getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return 0; // 空气
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return 0; // 空气
    }

    i32 localY = mc::world::toSectionLocalY(y);
    return section->getBlockStateId(x, localY, z);
}

void ChunkData::setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (stateId == 0) {
            return; // 不需要创建段来设置空气
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = mc::world::toSectionLocalY(y);
    section->setBlockStateId(x, localY, z, stateId);
    m_dirty = true;

    // 更新高度图（WorldSurface 槽位作为快速查询缓存）
    // currentTop 为最高方块 Y（Heightmap 存储 Y+1，故 getHeight-1；无方块时为 0）
    const BlockCoord rawHeight = m_heightmaps[static_cast<size_t>(HeightmapType::WorldSurface)].getHeight(x, z);
    const BlockCoord currentTop = rawHeight > 0 ? rawHeight - 1 : 0;
    if (y >= currentTop) {
        updateHeightMap(x, z);
    }
}

BlockCoord ChunkData::getHighestBlock(BlockCoord x, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return -1;
    }
    // WorldSurface 高度图存储 Y+1，最高方块 Y = getHeight - 1（getHeight 为 0 表示无方块，回退为 0）
    const BlockCoord height = m_heightmaps[static_cast<size_t>(HeightmapType::WorldSurface)].getHeight(x, z);
    return height > 0 ? height - 1 : 0;
}

BlockCoord ChunkData::getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return mc::world::MIN_BUILD_HEIGHT;
    }

    const size_t typeIndex = static_cast<size_t>(type);
    // 未被显式填充的类型回退到 WorldSurface 槽位
    const Heightmap& heightmap = m_heightmapInitialized[typeIndex]
        ? m_heightmaps[typeIndex]
        : m_heightmaps[static_cast<size_t>(HeightmapType::WorldSurface)];

    const BlockCoord height = heightmap.getHeight(x, z);
    return height > 0 ? height - 1 : mc::world::MIN_BUILD_HEIGHT;
}

void ChunkData::updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return;
    }

    const size_t typeIndex = static_cast<size_t>(type);
    Heightmap& heightmap = m_heightmaps[typeIndex];
    heightmap.update(x, y, z, state);
    m_heightmapInitialized[typeIndex] = true;
}

BiomeId ChunkData::getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return Biomes::Plains;
    }

    return m_biomes.getBiomeAtBlock(x, y, z);
}

void ChunkData::updateHeightMap(BlockCoord x, BlockCoord z)
{
    // 从上向下查找最高的非空气方块，写入 WorldSurface 高度图槽位
    Heightmap& worldSurface = m_heightmaps[static_cast<size_t>(HeightmapType::WorldSurface)];
    for (BlockCoord y = mc::world::MAX_BUILD_HEIGHT - 1; y >= mc::world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* state = getBlockState(x, y, z);
        if (state && !state->isAir()) {
            // Heightmap 内部存储 Y+1（上方空气方块位置）
            worldSurface.setHeight(x, z, y + 1);
            return;
        }
    }
    // 无方块：高度为 0（getHighestBlock 据此回退为 0）
    worldSurface.setHeight(x, z, 0);
}

ChunkSection* ChunkData::getSection(i32 index)
{
    if (index < 0 || index >= mc::world::CHUNK_SECTIONS) {
        return nullptr;
    }
    return m_sections[index].get();
}

const ChunkSection* ChunkData::getSection(i32 index) const
{
    if (index < 0 || index >= mc::world::CHUNK_SECTIONS) {
        return nullptr;
    }
    return m_sections[index].get();
}

bool ChunkData::hasSection(i32 index) const
{
    if (index < 0 || index >= mc::world::CHUNK_SECTIONS) {
        return false;
    }
    return m_sections[index] != nullptr;
}

ChunkSection* ChunkData::createSection(i32 index)
{
    if (index < 0 || index >= mc::world::CHUNK_SECTIONS) {
        return nullptr;
    }
    if (!m_sections[index]) {
        m_sections[index] = std::make_unique<ChunkSection>();
        m_dirty = true;
    }
    return m_sections[index].get();
}

const ChunkSection* const* ChunkData::getSections() const
{
    // 更新指针数组
    for (size_t i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
        m_sectionPtrs[i] = m_sections[i].get();
    }
    return m_sectionPtrs.data();
}

std::vector<u8> ChunkData::serialize() const
{
    std::vector<u8> data;

    // 头部: 位置 + 标志
    data.push_back(static_cast<u8>(m_x >> 24));
    data.push_back(static_cast<u8>(m_x >> 16));
    data.push_back(static_cast<u8>(m_x >> 8));
    data.push_back(static_cast<u8>(m_x & 0xFF));

    data.push_back(static_cast<u8>(m_z >> 24));
    data.push_back(static_cast<u8>(m_z >> 16));
    data.push_back(static_cast<u8>(m_z >> 8));
    data.push_back(static_cast<u8>(m_z & 0xFF));

    u8 flags = 0;
    if (m_fullyGenerated) flags |= 0x01;
    if (m_dirty) flags |= 0x02;
    data.push_back(flags);

    // 区块段掩码
    u32 sectionMask = 0;
    for (size_t i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
        if (m_sections[i]) {
            sectionMask |= (1U << i);
        }
    }
    data.push_back(static_cast<u8>(sectionMask >> 24));
    data.push_back(static_cast<u8>((sectionMask >> 16) & 0xFF));
    data.push_back(static_cast<u8>((sectionMask >> 8) & 0xFF));
    data.push_back(static_cast<u8>(sectionMask & 0xFF));

    // 生物群系数据
    auto biomeData = m_biomes.serialize();
    const u16 biomeSize = static_cast<u16>(biomeData.size());
    data.push_back(static_cast<u8>(biomeSize >> 8));
    data.push_back(static_cast<u8>(biomeSize & 0xFF));
    data.insert(data.end(), biomeData.begin(), biomeData.end());

    // 序列化每个段
    for (size_t i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
        if (m_sections[i]) {
            auto sectionData = m_sections[i]->serialize();
            // 写入段大小
            u32 sectionSize = static_cast<u32>(sectionData.size());
            data.push_back(static_cast<u8>(sectionSize >> 24));
            data.push_back(static_cast<u8>(sectionSize >> 16));
            data.push_back(static_cast<u8>(sectionSize >> 8));
            data.push_back(static_cast<u8>(sectionSize & 0xFF));
            // 写入段数据
            data.insert(data.end(), sectionData.begin(), sectionData.end());
        }
    }

    // 高度图（WorldSurface 槽位，磁盘格式：按 x*W+z 顺序，每列最高方块 Y，i16 大端）
    const Heightmap& worldSurface = m_heightmaps[static_cast<size_t>(HeightmapType::WorldSurface)];
    for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
            const BlockCoord rawHeight = worldSurface.getHeight(x, z);
            // Heightmap 存储 Y+1，回写为最高方块 Y；无方块为 0
            const BlockCoord highest = rawHeight > 0 ? rawHeight - 1 : 0;
            data.push_back(static_cast<u8>(highest >> 8));
            data.push_back(static_cast<u8>(highest & 0xFF));
        }
    }

    // 居住时间（8字节，大端序）
    for (int i = 56; i >= 0; i -= 8) {
        data.push_back(static_cast<u8>(m_inhabitedTime >> i));
    }

    return data;
}

Result<std::unique_ptr<ChunkData>> ChunkData::deserialize(const u8* data, size_t size)
{
    if (size < 13) {
        return Error(ErrorCode::InvalidArgument, "Invalid chunk data size");
    }

    auto chunk = std::make_unique<ChunkData>();
    size_t offset = 0;

    // 位置
    chunk->m_x = (static_cast<ChunkCoord>(data[offset]) << 24) | (static_cast<ChunkCoord>(data[offset + 1]) << 16) |
        (static_cast<ChunkCoord>(data[offset + 2]) << 8) | static_cast<ChunkCoord>(data[offset + 3]);
    offset += 4;

    chunk->m_z = (static_cast<ChunkCoord>(data[offset]) << 24) | (static_cast<ChunkCoord>(data[offset + 1]) << 16) |
        (static_cast<ChunkCoord>(data[offset + 2]) << 8) | static_cast<ChunkCoord>(data[offset + 3]);
    offset += 4;

    // 标志
    u8 flags = data[offset++];
    chunk->m_fullyGenerated = (flags & 0x01) != 0;
    chunk->m_dirty = (flags & 0x02) != 0;

    // 区块段掩码
    u32 sectionMask = (static_cast<u32>(data[offset]) << 24) | (static_cast<u32>(data[offset + 1]) << 16) |
        (static_cast<u32>(data[offset + 2]) << 8) | data[offset + 3];
    offset += 4;

    // 生物群系数据
    if (offset + 2 > size) {
        return Error(ErrorCode::InvalidArgument, "Biome data header missing");
    }
    const u16 biomeSize = (static_cast<u16>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    if (offset + biomeSize > size) {
        return Error(ErrorCode::InvalidArgument, "Biome data truncated");
    }

    if (biomeSize > 0) {
        auto biomeResult = BiomeContainer::deserialize(data + offset, biomeSize);
        if (biomeResult.failed()) {
            return biomeResult.error();
        }
        chunk->m_biomes = std::move(biomeResult.value());
    }
    offset += biomeSize;

    // 读取每个段
    for (size_t i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
        if (sectionMask & (1U << i)) {
            if (offset + 4 > size) {
                return Error(ErrorCode::InvalidArgument, "Invalid section size");
            }
            u32 sectionSize = (static_cast<u32>(data[offset]) << 24) | (static_cast<u32>(data[offset + 1]) << 16) |
                (static_cast<u32>(data[offset + 2]) << 8) | static_cast<u32>(data[offset + 3]);
            offset += 4;

            if (offset + sectionSize > size) {
                return Error(ErrorCode::InvalidArgument, "Section data truncated");
            }

            auto sectionResult = ChunkSection::deserialize(data + offset, sectionSize);
            if (sectionResult.failed()) {
                return sectionResult.error();
            }
            chunk->m_sections[i] = sectionResult.value();
            offset += sectionSize;
        }
    }

    // 高度图（WorldSurface 槽位，磁盘格式：按 x*W+z 顺序，每列最高方块 Y，i16 大端）
    if (offset + mc::world::CHUNK_WIDTH * mc::world::CHUNK_WIDTH * 2 > size) {
        return Error(ErrorCode::InvalidArgument, "Height map data missing");
    }
    {
        Heightmap& worldSurface = chunk->m_heightmaps[static_cast<size_t>(HeightmapType::WorldSurface)];
        for (i32 x = 0; x < mc::world::CHUNK_WIDTH; ++x) {
            for (i32 z = 0; z < mc::world::CHUNK_WIDTH; ++z) {
                // 磁盘存的是最高方块 Y，Heightmap 内部存储 Y+1；无方块(Y=0)保持 0
                const BlockCoord highest =
                    (static_cast<BlockCoord>(data[offset]) << 8) | static_cast<BlockCoord>(data[offset + 1]);
                offset += 2;
                worldSurface.setHeight(x, z, highest > 0 ? highest + 1 : 0);
            }
        }
        chunk->m_heightmapInitialized[static_cast<size_t>(HeightmapType::WorldSurface)] = true;
    }

    // 居住时间（8字节，大端序）
    if (offset + 8 <= size) {
        chunk->m_inhabitedTime = (static_cast<i64>(data[offset]) << 56) | (static_cast<i64>(data[offset + 1]) << 48) |
            (static_cast<i64>(data[offset + 2]) << 40) | (static_cast<i64>(data[offset + 3]) << 32) |
            (static_cast<i64>(data[offset + 4]) << 24) | (static_cast<i64>(data[offset + 5]) << 16) |
            (static_cast<i64>(data[offset + 6]) << 8) | static_cast<i64>(data[offset + 7]);
        offset += 8;
    }

    chunk->m_loaded = true;
    return std::move(chunk);
}

void ChunkData::fill(BlockCoord minY, BlockCoord maxY, u32 stateId)
{
    for (BlockCoord y = minY; y < maxY; y += mc::world::CHUNK_SECTION_HEIGHT) {
        i32 sectionIndex = mc::world::toSectionIndex(y);
        if (sectionIndex >= 0 && sectionIndex < mc::world::CHUNK_SECTIONS) {
            auto* section = createSection(sectionIndex);
            if (section) {
                section->fill(stateId);
            }
        }
    }
    m_dirty = true;
}

// ============================================================================
// 光照访问
// ============================================================================

u8 ChunkData::getSkyLight(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return 15; // 边界外默认全亮
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return 15; // 未创建的段默认全亮
    }

    i32 localY = mc::world::toSectionLocalY(y);
    return section->getSkyLight(x, localY, z);
}

void ChunkData::setSkyLight(BlockCoord x, BlockCoord y, BlockCoord z, u8 light)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (light == 15) {
            return; // 默认就是15，不需要创建段
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = mc::world::toSectionLocalY(y);
    section->setSkyLight(x, localY, z, light);
}

u8 ChunkData::getBlockLight(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return 0; // 边界外默认无光
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return 0; // 未创建的段默认无光
    }

    i32 localY = mc::world::toSectionLocalY(y);
    return section->getBlockLight(x, localY, z);
}

void ChunkData::setBlockLight(BlockCoord x, BlockCoord y, BlockCoord z, u8 light)
{
    if (x < 0 || x >= mc::world::CHUNK_WIDTH || y < mc::world::MIN_BUILD_HEIGHT || y >= mc::world::MAX_BUILD_HEIGHT ||
        z < 0 || z >= mc::world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = mc::world::toSectionIndex(y);
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (light == 0) {
            return; // 默认就是0，不需要创建段
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = mc::world::toSectionLocalY(y);
    section->setBlockLight(x, localY, z, light);
}

// ============================================================================
// ChunkDataRef 实现
// ============================================================================

ChunkDataRef::ChunkDataRef(ChunkData* data, bool writeAccess)
    : m_data(data)
    , m_writeAccess(writeAccess)
{}

ChunkDataRef::~ChunkDataRef()
{
    // 未来可以添加锁释放
}

ChunkDataRef::ChunkDataRef(ChunkDataRef&& other) noexcept
    : m_data(other.m_data)
    , m_writeAccess(other.m_writeAccess)
{
    other.m_data = nullptr;
    other.m_writeAccess = false;
}

ChunkDataRef& ChunkDataRef::operator=(ChunkDataRef&& other) noexcept
{
    if (this != &other) {
        m_data = other.m_data;
        m_writeAccess = other.m_writeAccess;
        other.m_data = nullptr;
        other.m_writeAccess = false;
    }
    return *this;
}

// ============================================================================
// Starlight 光照数据接口实现
// ============================================================================

void ChunkData::initLightData()
{
    // 初始化空映射（大小为 LIGHT_SECTIONS，包含上下缓冲区）
    m_skyEmptinessMap.fill(false);
    m_blockEmptinessMap.fill(false);
    m_hasSkyEmptinessMap = false;
    m_hasBlockEmptinessMap = false;

    // 初始化 Nibble 数组（延迟初始化状态）
    for (size_t i = 0; i < LIGHT_SECTIONS; ++i) {
        m_skyNibbles[i] = SWMRNibbleArray::createUninitialized();
        m_blockNibbles[i] = SWMRNibbleArray::createUninitialized();
    }

    m_nibblePtrsInitialized = false;
}

void ChunkData::_ensureNibblePtrs() const
{
    if (m_nibblePtrsInitialized) {
        return;
    }

    for (size_t i = 0; i < LIGHT_SECTIONS; ++i) {
        m_skyNibblePtrs[i] = const_cast<SWMRNibbleArray*>(&m_skyNibbles[i]);
        m_blockNibblePtrs[i] = const_cast<SWMRNibbleArray*>(&m_blockNibbles[i]);
    }

    m_nibblePtrsInitialized = true;
}

const bool* ChunkData::getSkyEmptinessMap() const
{
    if (!m_hasSkyEmptinessMap) {
        return nullptr;
    }
    return m_skyEmptinessMap.data();
}

void ChunkData::setSkyEmptinessMap(const bool* map)
{
    if (map == nullptr) {
        m_hasSkyEmptinessMap = false;
        m_skyEmptinessMap.fill(false);
        return;
    }

    std::copy_n(map, LIGHT_SECTIONS, m_skyEmptinessMap.begin());
    m_hasSkyEmptinessMap = true;
}

const bool* ChunkData::getBlockEmptinessMap() const
{
    if (!m_hasBlockEmptinessMap) {
        return nullptr;
    }
    return m_blockEmptinessMap.data();
}

void ChunkData::setBlockEmptinessMap(const bool* map)
{
    if (map == nullptr) {
        m_hasBlockEmptinessMap = false;
        m_blockEmptinessMap.fill(false);
        return;
    }

    std::copy_n(map, LIGHT_SECTIONS, m_blockEmptinessMap.begin());
    m_hasBlockEmptinessMap = true;
}

SWMRNibbleArray* const* ChunkData::getSkyNibbles() const
{
    _ensureNibblePtrs();
    return m_skyNibblePtrs.data();
}

void ChunkData::setSkyNibbles(SWMRNibbleArray* const* nibbles)
{
    if (nibbles == nullptr) {
        return;
    }

    for (size_t i = 0; i < LIGHT_SECTIONS; ++i) {
        if (nibbles[i] != nullptr) {
            m_skyNibbles[i] = std::move(*const_cast<SWMRNibbleArray*>(nibbles[i]));
        }
    }

    m_nibblePtrsInitialized = false;
}

SWMRNibbleArray* const* ChunkData::getBlockNibbles() const
{
    _ensureNibblePtrs();
    return m_blockNibblePtrs.data();
}

void ChunkData::setBlockNibbles(SWMRNibbleArray* const* nibbles)
{
    if (nibbles == nullptr) {
        return;
    }

    for (size_t i = 0; i < LIGHT_SECTIONS; ++i) {
        if (nibbles[i] != nullptr) {
            m_blockNibbles[i] = std::move(*const_cast<SWMRNibbleArray*>(nibbles[i]));
        }
    }

    m_nibblePtrsInitialized = false;
}

// ============================================================================
// 方块实体管理实现
// ============================================================================

namespace {

[[nodiscard]] i64 posToKey(const BlockPos& pos)
{
    const u32 x = static_cast<u32>(pos.x) & 0x1FFFFF;
    const u32 y = static_cast<u32>(pos.y) & 0x3FFFFF;
    const u32 z = static_cast<u32>(pos.z) & 0x1FFFFF;
    return (static_cast<i64>(x) << 43) | (static_cast<i64>(y) << 21) | static_cast<i64>(z);
}

bool isPosInChunk(ChunkCoord chunkX, ChunkCoord chunkZ, const BlockPos& pos)
{
    return pos.chunkX() == chunkX && pos.chunkZ() == chunkZ;
}

} // namespace

BlockEntity* ChunkData::getBlockEntity(const BlockPos& pos)
{
    if (!isPosInChunk(m_x, m_z, pos)) {
        return nullptr;
    }

    auto it = m_blockEntities.find(posToKey(pos));
    if (it == m_blockEntities.end()) {
        return nullptr;
    }
    return it->second.get();
}

const BlockEntity* ChunkData::getBlockEntity(const BlockPos& pos) const
{
    if (!isPosInChunk(m_x, m_z, pos)) {
        return nullptr;
    }

    auto it = m_blockEntities.find(posToKey(pos));
    if (it == m_blockEntities.end()) {
        return nullptr;
    }
    return it->second.get();
}

std::unique_ptr<BlockEntity> ChunkData::setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
{
    // 如果实体为空，直接返回空
    if (!entity) {
        return nullptr;
    }

    // 检查位置是否在当前区块内
    if (!isPosInChunk(m_x, m_z, pos)) {
        // 位置不在区块内，返回传入的实体
        return entity;
    }

    i64 key = posToKey(pos);

    // 查找是否有已存在的实体
    auto it = m_blockEntities.find(key);
    if (it != m_blockEntities.end()) {
        // 替换现有实体
        auto oldEntity = std::move(it->second);
        it->second = std::move(entity);
        m_dirty = true;
        return oldEntity;
    }

    // 添加新实体
    m_blockEntities[key] = std::move(entity);
    m_dirty = true;
    return nullptr;
}

std::unique_ptr<BlockEntity> ChunkData::removeBlockEntity(const BlockPos& pos)
{
    if (!isPosInChunk(m_x, m_z, pos)) {
        return nullptr;
    }

    i64 key = posToKey(pos);
    auto it = m_blockEntities.find(key);
    if (it == m_blockEntities.end()) {
        return nullptr;
    }

    auto entity = std::move(it->second);
    m_blockEntities.erase(it);
    m_dirty = true;
    return entity;
}

bool ChunkData::hasBlockEntity(const BlockPos& pos) const
{
    if (!isPosInChunk(m_x, m_z, pos)) {
        return false;
    }

    return m_blockEntities.find(posToKey(pos)) != m_blockEntities.end();
}

std::vector<BlockEntity*> ChunkData::getAllBlockEntities()
{
    std::vector<BlockEntity*> entities;
    entities.reserve(m_blockEntities.size());
    for (auto& pair : m_blockEntities) {
        entities.push_back(pair.second.get());
    }
    return entities;
}

std::vector<const BlockEntity*> ChunkData::getAllBlockEntities() const
{
    std::vector<const BlockEntity*> entities;
    entities.reserve(m_blockEntities.size());
    for (const auto& pair : m_blockEntities) {
        entities.push_back(pair.second.get());
    }
    return entities;
}

size_t ChunkData::blockEntityCount() const
{
    return m_blockEntities.size();
}

void ChunkData::addLoadedEntity(std::unique_ptr<Entity> entity)
{
    if (entity == nullptr) {
        return;
    }
    m_loadedEntities.push_back(std::move(entity));
}

std::vector<std::unique_ptr<Entity>> ChunkData::takeLoadedEntities()
{
    return std::move(m_loadedEntities);
}

// ============================================================================
// 后处理位置
// ============================================================================

void ChunkData::addPackedPostProcessing(const std::array<std::vector<u16>, mc::world::CHUNK_SECTIONS>& sections)
{
    for (i32 i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
        if (!sections[i].empty()) {
            auto& section = m_postProcessingSections[i];
            section.reserve(section.size() + sections[i].size());
            section.insert(section.end(), sections[i].begin(), sections[i].end());
        }
    }
}

void ChunkData::clearPostProcessingForSection(i32 sectionIndex)
{
    if (sectionIndex >= 0 && sectionIndex < mc::world::CHUNK_SECTIONS) {
        m_postProcessingSections[sectionIndex].clear();
    }
}

void ChunkData::clearAllPostProcessing()
{
    for (i32 i = 0; i < mc::world::CHUNK_SECTIONS; ++i) {
        m_postProcessingSections[i].clear();
    }
}

// ============================================================================
// 游戏事件监听器注册表
// ============================================================================

gameevent::GameEventListenerRegistry* ChunkData::getGameEventListenerRegistry(i32 sectionY)
{
    auto it = m_gameEventListenerRegistries.find(sectionY);
    if (it != m_gameEventListenerRegistries.end()) {
        return it->second.get();
    }
    return nullptr;
}

const gameevent::GameEventListenerRegistry* ChunkData::getGameEventListenerRegistry(i32 sectionY) const
{
    auto it = m_gameEventListenerRegistries.find(sectionY);
    if (it != m_gameEventListenerRegistries.end()) {
        return it->second.get();
    }
    return nullptr;
}

gameevent::GameEventListenerRegistry& ChunkData::getOrCreateGameEventListenerRegistry(
    i32 sectionY, std::function<std::unique_ptr<gameevent::EuclideanGameEventListenerRegistry>(i32)> factory)
{
    auto it = m_gameEventListenerRegistries.find(sectionY);
    if (it != m_gameEventListenerRegistries.end()) {
        return *it->second;
    }

    // 使用工厂函数创建注册表，当为空时自动从映射中移除
    auto registry = factory(sectionY);

    auto& ref = *registry;
    m_gameEventListenerRegistries.emplace(sectionY, std::move(registry));
    return ref;
}

void ChunkData::removeGameEventListenerRegistry(i32 sectionY)
{
    m_gameEventListenerRegistries.erase(sectionY);
}

} // namespace mc::world::chunk

#pragma pop_macro("BYTE_SIZE")
