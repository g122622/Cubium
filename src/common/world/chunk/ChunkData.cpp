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

// TODO: include 路径应改为不使用 ../ 的形式，但这会影响其他文件，暂时保持现状
// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "ChunkData.hpp"
#include "../biome/Biome.hpp"
#include "../block/BlockRegistry.hpp"
#include "../blockentity/BlockEntity.hpp"
#include "../fluid/Fluid.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>

#pragma pop_macro("BYTE_SIZE")

namespace mc {

// ============================================================================
// ChunkSection 实现
// ============================================================================

ChunkSection::ChunkSection()
    : m_blockStates(VOLUME, 0)            // 默认所有方块为空气 (stateId = 0)
    , m_skyLight(NibbleArray::filled(15)) // 默认天空光照全亮
    , m_blockLight()                      // 默认方块光照无光（空数组，返回0）
{}

u32 ChunkSection::getBlockStateId(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return 0; // 空气
    }
    return m_blockStates[blockIndex(x, y, z)];
}

void ChunkSection::setBlockStateIdFast(i32 index, u32 stateId)
{
    if (index < 0 || index >= static_cast<i32>(m_blockStates.size())) {
        return;
    }

    const size_t actualIndex = static_cast<size_t>(index);
    u32 oldStateId = m_blockStates[actualIndex];
    const BlockState* oldState = Block::getBlockState(oldStateId);
    const BlockState* newState = Block::getBlockState(stateId);

    bool oldIsAir = oldState ? oldState->isAir() : true;
    bool newIsAir = newState ? newState->isAir() : true;

    if (oldIsAir && !newIsAir) {
        ++m_blockCount;
    } else if (!oldIsAir && newIsAir) {
        --m_blockCount;
    }

    if (oldState && oldState->getBlock().ticksRandomly()) {
        --m_blockTickRefCount;
    }
    if (newState && newState->getBlock().ticksRandomly()) {
        ++m_blockTickRefCount;
    }

    if (oldState) {
        const fluid::FluidState* oldFluid = oldState->getFluidState();
        if (oldFluid && !oldFluid->isEmpty()) {
            --m_fluidRefCount;
        }
    }
    if (newState) {
        const fluid::FluidState* newFluid = newState->getFluidState();
        if (newFluid && !newFluid->isEmpty()) {
            ++m_fluidRefCount;
        }
    }

    m_blockStates[actualIndex] = stateId;
}

void ChunkSection::setBlockStateId(i32 x, i32 y, i32 z, u32 stateId)
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return;
    }
    i32 index = blockIndex(x, y, z);
    u32 oldStateId = m_blockStates[index];

    // 获取旧状态和新状态来判断是否是空气
    const BlockState* oldState = Block::getBlockState(oldStateId);
    const BlockState* newState = Block::getBlockState(stateId);

    bool oldIsAir = oldState ? oldState->isAir() : true;
    bool newIsAir = newState ? newState->isAir() : true;

    if (oldIsAir && !newIsAir) {
        m_blockCount++;
    } else if (!oldIsAir && newIsAir) {
        m_blockCount--;
    }

    // 更新随机刻计数器
    if (oldState && oldState->getBlock().ticksRandomly()) {
        --m_blockTickRefCount;
    }
    if (newState && newState->getBlock().ticksRandomly()) {
        ++m_blockTickRefCount;
    }

    // 更新流体计数器
    if (oldState) {
        const fluid::FluidState* oldFluid = oldState->getFluidState();
        if (oldFluid && !oldFluid->isEmpty()) {
            --m_fluidRefCount;
        }
    }
    if (newState) {
        const fluid::FluidState* newFluid = newState->getFluidState();
        if (newFluid && !newFluid->isEmpty()) {
            ++m_fluidRefCount;
        }
    }

    m_blockStates[index] = stateId;
    m_needsRecalculate = true;
}

const BlockState* ChunkSection::getBlockState(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return nullptr;
    }

    u32 stateId = getBlockStateId(x, y, z);
    return Block::getBlockState(stateId);
}

void ChunkSection::rebuildTickCounters()
{
    m_blockTickRefCount = 0;
    m_fluidRefCount = 0;

    for (u32 stateId : m_blockStates) {
        const BlockState* state = Block::getBlockState(stateId);
        if (state == nullptr) {
            continue;
        }

        if (!state->isAir() && state->getBlock().ticksRandomly()) {
            ++m_blockTickRefCount;
        }

        const fluid::FluidState* fluidState = state->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            ++m_fluidRefCount;
        }
    }
}

void ChunkSection::setBlockState(i32 x, i32 y, i32 z, const BlockState* state)
{
    u32 stateId = state ? state->stateId() : 0;
    setBlockStateId(x, y, z, stateId);
}

u8 ChunkSection::getSkyLight(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return 15;
    }
    return m_skyLight.get(x, y, z);
}

void ChunkSection::setSkyLight(i32 x, i32 y, i32 z, u8 light)
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return;
    }
    m_skyLight.set(x, y, z, std::min(light, static_cast<u8>(15)));
}

u8 ChunkSection::getBlockLight(i32 x, i32 y, i32 z) const
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return 0;
    }
    return m_blockLight.get(x, y, z);
}

void ChunkSection::setBlockLight(i32 x, i32 y, i32 z, u8 light)
{
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || z < 0 || z >= SIZE) {
        return;
    }
    m_blockLight.set(x, y, z, std::min(light, static_cast<u8>(15)));
}

std::vector<u8> ChunkSection::serialize() const
{
    // 格式: 块数量 + 方块状态ID + 天空光照 + 方块光照
    // 注意：NibbleArray::BYTE_SIZE = 2048 = VOLUME / 2
    constexpr size_t SECTION_DATA_SIZE = 2 + VOLUME * sizeof(u32) + NibbleArray::BYTE_SIZE * 2;

    std::vector<u8> data(SECTION_DATA_SIZE);
    u8* out = data.data();

    // 块数量
    *out++ = static_cast<u8>(m_blockCount >> 8);
    *out++ = static_cast<u8>(m_blockCount & 0xFF);

    // 方块状态ID (u32) - 以小端序写入，与网络同步格式保持一致
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    std::memcpy(out, m_blockStates.data(), m_blockStates.size() * sizeof(u32));
    out += m_blockStates.size() * sizeof(u32);
#else
    for (u32 stateId : m_blockStates) {
        *out++ = static_cast<u8>(stateId & 0xFF);
        *out++ = static_cast<u8>((stateId >> 8) & 0xFF);
        *out++ = static_cast<u8>((stateId >> 16) & 0xFF);
        *out++ = static_cast<u8>((stateId >> 24) & 0xFF);
    }
#endif

    // 天空光照
    const auto& skyLightData = m_skyLight.data();
    if (!skyLightData.empty()) {
        std::memcpy(out, skyLightData.data(), NibbleArray::BYTE_SIZE);
    } else {
        // 如果为空，写入全亮数据
        std::fill_n(out, NibbleArray::BYTE_SIZE, 0xFF);
    }
    out += NibbleArray::BYTE_SIZE;

    // 方块光照
    const auto& blockLightData = m_blockLight.data();
    if (!blockLightData.empty()) {
        std::memcpy(out, blockLightData.data(), NibbleArray::BYTE_SIZE);
    } else {
        // 如果为空，写入全黑数据
        std::fill_n(out, NibbleArray::BYTE_SIZE, 0x00);
    }

    return data;
}

Result<std::unique_ptr<ChunkSection>> ChunkSection::deserialize(const u8* data, size_t size)
{
    // 新格式大小: 2 + VOLUME * 4 + BYTE_SIZE * 2
    constexpr size_t expectedSize = 2 + VOLUME * sizeof(u32) + NibbleArray::BYTE_SIZE * 2;
    if (size < expectedSize) [[unlikely]] {
        std::stringstream ss;
        ss << "Invalid section data size, expected at least " << expectedSize << " bytes, got " << size << " bytes";
        return Error(ErrorCode::InvalidArgument, ss.str());
    }

    auto section = std::make_unique<ChunkSection>();
    size_t offset = 0;

    // 块数量
    section->m_blockCount = (static_cast<u16>(data[offset]) << 8) | data[offset + 1];
    offset += 2;

    // 方块状态ID - 使用小端序与 ChunkSerializer::serializeSection 保持一致
    const size_t blockStateBytes = VOLUME * sizeof(u32);
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    std::memcpy(section->m_blockStates.data(), data + offset, blockStateBytes);
    offset += blockStateBytes;
#else
    for (size_t i = 0; i < VOLUME; ++i) {
        section->m_blockStates[i] = static_cast<u32>(data[offset]) | (static_cast<u32>(data[offset + 1]) << 8) |
            (static_cast<u32>(data[offset + 2]) << 16) | (static_cast<u32>(data[offset + 3]) << 24);
        offset += 4;
    }
#endif

    // 天空光照
    auto& skyLightData = section->m_skyLight.data();
    skyLightData.resize(NibbleArray::BYTE_SIZE);
    std::memcpy(skyLightData.data(), data + offset, NibbleArray::BYTE_SIZE);
    offset += NibbleArray::BYTE_SIZE;

    // 方块光照
    auto& blockLightData = section->m_blockLight.data();
    blockLightData.resize(NibbleArray::BYTE_SIZE);
    std::memcpy(blockLightData.data(), data + offset, NibbleArray::BYTE_SIZE);

    section->rebuildTickCounters();
    return std::move(section);
}

void ChunkSection::fill(u32 stateId)
{
    for (size_t i = 0; i < VOLUME; ++i) {
        m_blockStates[i] = stateId;
    }

    const BlockState* state = Block::getBlockState(stateId);
    m_blockCount = (state && !state->isAir()) ? VOLUME : 0;
    rebuildTickCounters();
    m_needsRecalculate = true;
}

// ============================================================================
// ChunkData 实现
// ============================================================================

ChunkData::ChunkData()
{
    m_heightMap.fill(0);
    initLightData();
}

ChunkData::ChunkData(ChunkCoord x, ChunkCoord z)
    : m_x(x)
    , m_z(z)
{
    m_heightMap.fill(0);
    initLightData();
}

ChunkData::~ChunkData() = default;

const BlockState* ChunkData::getBlockState(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return nullptr; // 空气
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return nullptr; // 空气
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
    return section->getBlockState(x, localY, z);
}

void ChunkData::setBlockState(BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (!state || state->isAir()) {
            return; // 不需要创建段来设置空气
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
    section->setBlockState(x, localY, z, state);
    m_dirty = true;

    // 更新高度图
    if (y >= m_heightMap[x * world::CHUNK_WIDTH + z]) {
        updateHeightMap(x, z);
    }
}

u32 ChunkData::getBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return 0; // 空气
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return 0; // 空气
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
    return section->getBlockStateId(x, localY, z);
}

void ChunkData::setBlockStateId(BlockCoord x, BlockCoord y, BlockCoord z, u32 stateId)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (stateId == 0) {
            return; // 不需要创建段来设置空气
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
    section->setBlockStateId(x, localY, z, stateId);
    m_dirty = true;

    // 更新高度图
    if (y >= m_heightMap[x * world::CHUNK_WIDTH + z]) {
        updateHeightMap(x, z);
    }
}

BlockCoord ChunkData::getHighestBlock(BlockCoord x, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH) {
        return -1;
    }
    return m_heightMap[x * world::CHUNK_WIDTH + z];
}

BlockCoord ChunkData::getTopBlockY(HeightmapType type, BlockCoord x, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH) {
        MC_ASSERT_RELEASE(false);
    }

    // 检查是否有特定类型的高度图
    auto it = m_heightmaps.find(type);
    if (it != m_heightmaps.end()) {
        return it->second.getHeight(x, z) - 1;
    }

    // 默认使用基本高度图
    return m_heightMap[x * world::CHUNK_WIDTH + z];
}

void ChunkData::updateHeightmap(HeightmapType type, BlockCoord x, BlockCoord y, BlockCoord z, const BlockState* state)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH) {
        return;
    }

    // 获取或创建高度图
    auto& heightmap = m_heightmaps[type];
    if (heightmap.getType() != type) {
        heightmap = Heightmap(type);
    }

    heightmap.update(x, y, z, state);

    // 同时更新基本高度图（WorldSurface 类型）
    if (type == HeightmapType::WorldSurface || type == HeightmapType::WorldSurfaceWG) {
        const BlockCoord height = heightmap.getHeight(x, z);
        m_heightMap[x * world::CHUNK_WIDTH + z] = height > 0 ? height - 1 : 0;
    }
}

BiomeId ChunkData::getBiomeAtBlock(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return Biomes::Plains;
    }

    return m_biomes.getBiomeAtBlock(x, y, z);
}

void ChunkData::updateHeightMap(BlockCoord x, BlockCoord z)
{
    // 从上向下查找最高的非空气方块
    for (BlockCoord y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
        const BlockState* state = getBlockState(x, y, z);
        if (state && !state->isAir()) {
            m_heightMap[x * world::CHUNK_WIDTH + z] = y;
            return;
        }
    }
    m_heightMap[x * world::CHUNK_WIDTH + z] = world::MIN_BUILD_HEIGHT;
}

ChunkSection* ChunkData::getSection(i32 index)
{
    if (index < 0 || index >= world::CHUNK_SECTIONS) {
        return nullptr;
    }
    return m_sections[index].get();
}

const ChunkSection* ChunkData::getSection(i32 index) const
{
    if (index < 0 || index >= world::CHUNK_SECTIONS) {
        return nullptr;
    }
    return m_sections[index].get();
}

bool ChunkData::hasSection(i32 index) const
{
    if (index < 0 || index >= world::CHUNK_SECTIONS) {
        return false;
    }
    return m_sections[index] != nullptr;
}

ChunkSection* ChunkData::createSection(i32 index)
{
    if (index < 0 || index >= world::CHUNK_SECTIONS) {
        return nullptr;
    }
    if (!m_sections[index]) {
        m_sections[index] = std::make_unique<ChunkSection>();
        m_dirty = true;
    }
    return m_sections[index].get();
}

void ChunkData::removeSection(i32 index)
{
    if (index >= 0 && index < world::CHUNK_SECTIONS) {
        m_sections[index].reset();
        m_dirty = true;
    }
}

const ChunkSection* const* ChunkData::getSections() const
{
    // 更新指针数组
    for (size_t i = 0; i < world::CHUNK_SECTIONS; ++i) {
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
    for (size_t i = 0; i < world::CHUNK_SECTIONS; ++i) {
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
    for (size_t i = 0; i < world::CHUNK_SECTIONS; ++i) {
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

    // 高度图
    for (BlockCoord h : m_heightMap) {
        data.push_back(static_cast<u8>(h >> 8));
        data.push_back(static_cast<u8>(h & 0xFF));
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
    for (size_t i = 0; i < world::CHUNK_SECTIONS; ++i) {
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

    // 高度图
    if (offset + world::CHUNK_WIDTH * world::CHUNK_WIDTH * 2 > size) {
        return Error(ErrorCode::InvalidArgument, "Height map data missing");
    }
    for (size_t i = 0; i < world::CHUNK_WIDTH * world::CHUNK_WIDTH; ++i) {
        chunk->m_heightMap[i] =
            (static_cast<BlockCoord>(data[offset]) << 8) | static_cast<BlockCoord>(data[offset + 1]);
        offset += 2;
    }

    chunk->m_loaded = true;
    return std::move(chunk);
}

void ChunkData::fill(BlockCoord minY, BlockCoord maxY, u32 stateId)
{
    for (BlockCoord y = minY; y < maxY; y += world::CHUNK_SECTION_HEIGHT) {
        i32 sectionIndex = world::toSectionIndex(y);
        if (sectionIndex >= 0 && sectionIndex < world::CHUNK_SECTIONS) {
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
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return 15; // 边界外默认全亮
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return 15; // 未创建的段默认全亮
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
    return section->getSkyLight(x, localY, z);
}

void ChunkData::setSkyLight(BlockCoord x, BlockCoord y, BlockCoord z, u8 light)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (light == 15) {
            return; // 默认就是15，不需要创建段
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
    section->setSkyLight(x, localY, z, light);
}

u8 ChunkData::getBlockLight(BlockCoord x, BlockCoord y, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return 0; // 边界外默认无光
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    const auto& section = m_sections[sectionIndex];

    if (!section) {
        return 0; // 未创建的段默认无光
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
    return section->getBlockLight(x, localY, z);
}

void ChunkData::setBlockLight(BlockCoord x, BlockCoord y, BlockCoord z, u8 light)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT || z < 0 ||
        z >= world::CHUNK_WIDTH) {
        return;
    }

    i32 sectionIndex = (y - world::MIN_BUILD_HEIGHT) / world::CHUNK_SECTION_HEIGHT;
    auto& section = m_sections[sectionIndex];

    if (!section) {
        if (light == 0) {
            return; // 默认就是0，不需要创建段
        }
        section = std::make_unique<ChunkSection>();
    }

    i32 localY = (y - world::MIN_BUILD_HEIGHT) % world::CHUNK_SECTION_HEIGHT;
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

/**
 * @brief 将方块位置转换为唯一的64位键
 * @param pos 方块位置
 * @return 64位键
 */
i64 posToKey(const BlockPos& pos)
{
    // 使用 21 位存储 x 和 z，22 位存储 y（支持 -64 到 319 的范围）
    // 总共 64 位: x(21) | y(22) | z(21)
    u32 x = static_cast<u32>(pos.x) & 0x1FFFFF; // 21 位
    u32 y = static_cast<u32>(pos.y) & 0x3FFFFF; // 22 位
    u32 z = static_cast<u32>(pos.z) & 0x1FFFFF; // 21 位
    return (static_cast<i64>(x) << 43) | (static_cast<i64>(y) << 21) | static_cast<i64>(z);
}

/**
 * @brief 检查位置是否在当前区块内
 * @param chunkX 区块 X 坐标
 * @param chunkZ 区块 Z 坐标
 * @param pos 方块位置
 * @return 如果位置在区块内返回 true
 */
bool isPosInChunk(ChunkCoord chunkX, ChunkCoord chunkZ, const BlockPos& pos)
{
    ChunkCoord posChunkX = pos.x >> 4;
    ChunkCoord posChunkZ = pos.z >> 4;
    return posChunkX == chunkX && posChunkZ == chunkZ;
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

void ChunkData::addPackedPostProcessing(const std::vector<u16>* sections)
{
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if (!sections[i].empty()) {
            auto& section = m_postProcessingSections[i];
            section.reserve(section.size() + sections[i].size());
            section.insert(section.end(), sections[i].begin(), sections[i].end());
        }
    }
}

void ChunkData::clearPostProcessingForSection(i32 sectionIndex)
{
    if (sectionIndex >= 0 && sectionIndex < world::CHUNK_SECTIONS) {
        m_postProcessingSections[sectionIndex].clear();
    }
}

void ChunkData::clearAllPostProcessing()
{
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        m_postProcessingSections[i].clear();
    }
}

} // namespace mc
