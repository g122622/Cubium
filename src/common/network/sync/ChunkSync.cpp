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

// macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "ChunkSync.hpp"
#include "../../world/WorldConstants.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

using namespace mc::trace;

namespace mc::network {

// ============================================================================
// 高度图网络同步辅助
// ============================================================================

// 扩展块携带的 final 高度图类型（含 WorldSurface，按位掩码序）。
// WorldSurface 虽已有独立的 256 字节 u8 块（向后兼容，但有损且 deserialize 曾跳过），
// 仍在此以 i16 无损重传，保证客户端负 Y / Y>255 场景下 WorldSurface 也准确。
constexpr std::array<HeightmapType, 5> FINAL_HEIGHTMAP_TYPES = {
    HeightmapType::WorldSurface,
    HeightmapType::OceanFloor,
    HeightmapType::MotionBlocking,
    HeightmapType::MotionBlockingNoLeaves,
    HeightmapType::LightBlocking,
};

// 哨兵：高度图原始存储为 NO_BLOCK_SENTINEL（MIN_BUILD_HEIGHT-1，主世界 -65）时，
// 网络上用 INT16_MIN 标记"该列无方块"（与磁盘格式 ChunkData::serialize 一致）。
constexpr i16 HEIGHTMAP_NETWORK_SENTINEL = INT16_MIN;

// 把高度图原始值（Y+1 或 NO_BLOCK_SENTINEL）转为网络 i16
[[nodiscard]] static i16 encodeHeightmapValue(BlockCoord raw)
{
    return raw == Heightmap::NO_BLOCK_SENTINEL ? HEIGHTMAP_NETWORK_SENTINEL : static_cast<i16>(raw);
}

// 把网络 i16 转回高度图原始值
[[nodiscard]] static BlockCoord decodeHeightmapValue(i16 encoded)
{
    return encoded == HEIGHTMAP_NETWORK_SENTINEL ? Heightmap::NO_BLOCK_SENTINEL : static_cast<BlockCoord>(encoded);
}

// ============================================================================
// ChunkSerializer 实现
// ============================================================================

Result<std::vector<u8>> ChunkSerializer::serializeChunk(const ChunkData& chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network,
        "ChunkSerializer::serializeChunk",
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(chunk.x(), chunk.z()).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    const auto biomeData = chunk.getBiomes().serialize();
    const u32 sectionMask = calculateSectionMask(chunk);

    constexpr size_t heightmapSize = static_cast<size_t>(world::CHUNK_WIDTH) * world::CHUNK_WIDTH;

    // 计算扩展高度图块：1 字节存在位掩码 + 每个已初始化 final 类型 256×sizeof(i16) 字节
    u8 heightmapPresenceMask = 0;
    size_t heightmapExtraSize = 1;
    for (size_t i = 0; i < FINAL_HEIGHTMAP_TYPES.size(); ++i) {
        if (chunk.isHeightmapInitialized(FINAL_HEIGHTMAP_TYPES[i])) {
            heightmapPresenceMask |= static_cast<u8>(1U << i);
            heightmapExtraSize += heightmapSize * sizeof(i16);
        }
    }

    size_t expectedSize = 4 + 4 + 4 + heightmapSize + 4 + biomeData.size();
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if ((sectionMask & (1U << i)) == 0) continue;

        const ChunkSection* section = chunk.getSection(i);
        if (!section) continue;

        expectedSize += 2 + calculateSectionSize(*section);
    }
    expectedSize += 8;                  // inhabitedTime (i64)
    expectedSize += heightmapExtraSize; // 扩展高度图块

    PacketSerializer ser(expectedSize);

    // 写入区块坐标
    ser.writeI32(chunk.x());
    ser.writeI32(chunk.z());

    // 写入区块段位掩码
    ser.writeU32(sectionMask);

    // 写入高度图（向后兼容的有损 WorldSurface u8 块；扩展块以 i16 无损重传全部 final 类型）
    std::array<u8, world::CHUNK_WIDTH * world::CHUNK_WIDTH> heightmapData{};
    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            const size_t index = static_cast<size_t>(z * world::CHUNK_WIDTH + x);
            heightmapData[index] = static_cast<u8>(chunk.getHighestBlock(x, z));
        }
    }
    ser.writeBytes(heightmapData.data(), heightmapData.size());

    // 写入生物群系数据（u32长度 + 原始数据）
    ser.writeU32(static_cast<u32>(biomeData.size()));
    ser.writeBytes(biomeData);

    // 写入区块段数据
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if ((sectionMask & (1U << i)) == 0) continue;

        const ChunkSection* section = chunk.getSection(i);
        if (!section) continue;

        auto sectionData = ChunkSerializer::serializeSection(*section);
        if (sectionData.empty()) {
            return Error(ErrorCode::InvalidData, "Failed to serialize section");
        }

        ser.writeU16(static_cast<u16>(sectionData.size()));
        ser.writeBytes(sectionData);
    }

    // 写入居住时间（8字节）
    ser.writeI64(chunk.inhabitedTime());

    // 写入扩展高度图块（位于包尾，旧客户端读 inhabitedTime 后停止，自动忽略）
    ser.writeU8(heightmapPresenceMask);
    for (size_t i = 0; i < FINAL_HEIGHTMAP_TYPES.size(); ++i) {
        if ((heightmapPresenceMask & static_cast<u8>(1U << i)) == 0) {
            continue;
        }
        const auto& data = chunk.getHeightmapData(FINAL_HEIGHTMAP_TYPES[i]);
        for (size_t j = 0; j < data.size(); ++j) {
            ser.writeI16(encodeHeightmapValue(data[j]));
        }
    }

    return std::move(ser.buffer());
}

std::vector<u8> ChunkSerializer::serializeSection(const ChunkSection& section)
{
    return section.serialize();
}

Result<std::unique_ptr<ChunkData>> ChunkSerializer::deserializeChunk(
    ChunkCoord x, ChunkCoord z, const std::vector<u8>& data)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network,
        "ChunkSerializer::deserializeChunk",
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(x, z).toId())](::perfetto::EventContext ctx) { flow(ctx); });

    PacketDeserializer deser(data.data(), data.size());

    auto chunk = std::make_unique<ChunkData>(x, z);

    // 读取区块坐标验证
    auto xResult = deser.readI32();
    if (xResult.failed()) {
        return xResult.error();
    }
    if (xResult.value() != x) {
        return Error(ErrorCode::InvalidData, "Chunk X coordinate mismatch");
    }

    auto zResult = deser.readI32();
    if (zResult.failed()) {
        return zResult.error();
    }
    if (zResult.value() != z) {
        return Error(ErrorCode::InvalidData, "Chunk Z coordinate mismatch");
    }

    // 读取区块段位掩码
    auto maskResult = deser.readU32();
    if (maskResult.failed()) {
        return maskResult.error();
    }
    u32 sectionMask = maskResult.value();

    // 读取向后兼容的有损 WorldSurface u8 块（256 字节）。
    // 扩展块存在时优先用其 i16 无损值覆盖；不存在时回退此块（有损，负 Y/Y>255 截断）。
    std::array<u8, world::CHUNK_WIDTH * world::CHUNK_WIDTH> legacyHeightmapData{};
    auto heightmapResult = deser.readBytesInto(legacyHeightmapData);
    if (heightmapResult.failed()) {
        return heightmapResult.error();
    }

    // 读取生物群系数据（u32长度 + 原始数据）
    auto biomeSizeResult = deser.readU32();
    if (biomeSizeResult.failed()) {
        return biomeSizeResult.error();
    }

    const u32 biomeDataSize = biomeSizeResult.value();
    if (biomeDataSize > 0) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "ChunkSerializer::deserializeChunk.biomes");

        std::vector<u8> biomeData(biomeDataSize);
        auto biomeDataResult = deser.readBytesInto(biomeData.data(), biomeDataSize);
        if (biomeDataResult.failed()) {
            return biomeDataResult.error();
        }

        auto biomeContainerResult = BiomeContainer::deserialize(biomeData.data(), biomeDataSize);
        if (biomeContainerResult.failed()) {
            return biomeContainerResult.error();
        }

        chunk->setBiomes(std::move(biomeContainerResult.value()));
    }

    // 读取区块段
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network, "ChunkSerializer::deserializeChunk.section");

        if ((sectionMask & (1U << i)) == 0) continue;

        auto sizeResult = deser.readU16();
        if (sizeResult.failed()) {
            return sizeResult.error();
        }
        u16 sectionSize = sizeResult.value();

        std::vector<u8> sectionData(sectionSize);
        auto sectionDataResult = deser.readBytesInto(sectionData.data(), sectionData.size());
        if (sectionDataResult.failed()) {
            return sectionDataResult.error();
        }

        auto sectionResult = ChunkSerializer::deserializeChunkSection(sectionData.data(), sectionData.size());
        if (sectionResult.failed()) {
            return sectionResult.error();
        }

        ChunkSection* section = chunk->createSection(i);
        if (!section) {
            return Error(ErrorCode::OutOfMemory, "Failed to create section");
        }

        *section = std::move(*sectionResult.value());
    }

    // 读取居住时间（8字节，向后兼容：如果数据不足则默认为0）
    {
        auto inhabitedResult = deser.readI64();
        if (inhabitedResult.success()) {
            chunk->setInhabitedTime(inhabitedResult.value());
        }
    }

    // 读取扩展高度图块（位于包尾，旧服务端无此块时 remaining==0，跳过回退有损 u8 块）
    bool hasExtendedWorldSurface = false;
    if (deser.hasRemaining(sizeof(u8))) {
        auto maskResult = deser.readU8();
        if (maskResult.success()) {
            const u8 presenceMask = maskResult.value();
            for (size_t i = 0; i < FINAL_HEIGHTMAP_TYPES.size(); ++i) {
                if ((presenceMask & static_cast<u8>(1U << i)) == 0) {
                    continue;
                }
                constexpr size_t count = static_cast<size_t>(world::CHUNK_WIDTH) * world::CHUNK_WIDTH;
                std::array<BlockCoord, Heightmap::SIZE> heights{};
                bool decodeOk = true;
                for (size_t j = 0; j < count; ++j) {
                    auto vResult = deser.readI16();
                    if (vResult.failed()) {
                        decodeOk = false;
                        break;
                    }
                    heights[j] = decodeHeightmapValue(vResult.value());
                }
                if (!decodeOk) {
                    break;
                }
                chunk->setHeightmapFromStorage(FINAL_HEIGHTMAP_TYPES[i], heights);
                if (FINAL_HEIGHTMAP_TYPES[i] == HeightmapType::WorldSurface) {
                    hasExtendedWorldSurface = true;
                }
            }
        }
    }

    // 扩展块无 WorldSurface 时回退有损 u8 块（旧服务端兼容）。
    // u8 存的是最高方块 Y（getHighestBlock，无方块为 0），还原为 Y+1；u8=0 歧义按"Y=0 处有方块"处理。
    if (!hasExtendedWorldSurface) {
        std::array<BlockCoord, Heightmap::SIZE> heights{};
        for (size_t j = 0; j < legacyHeightmapData.size(); ++j) {
            heights[j] = static_cast<BlockCoord>(legacyHeightmapData[j]) + 1;
        }
        chunk->setHeightmapFromStorage(HeightmapType::WorldSurface, heights);
    }

    chunk->setFullyGenerated(true);
    chunk->setLoaded(true);

    return chunk;
}

Result<std::unique_ptr<ChunkSection>> ChunkSerializer::deserializeChunkSection(const u8* data, size_t size)
{
    return ChunkSection::deserialize(data, size);
}

size_t ChunkSerializer::calculateChunkSize(const ChunkData& chunk)
{
    // 必须与 serializeChunk 的实际写入严格一致，逐字段镜像：
    //   i32 chunkX(4) + i32 chunkZ(4) + u32 sectionMask(4) + 高度图(256)
    //   + u32 生物群系长度(4) + 生物群系数据 + 每个非空 section: u16 长度前缀(2) + sectionData
    //   + i64 inhabitedTime(8) + 扩展高度图块(1 字节掩码 + 每个已初始化 final 类型 256×2)
    // 注意 section 的纳入条件是“存在且非空”（与 calculateSectionMask/serializeChunk 一致），
    // 否则空 section 会被位掩码排除，这里却计入，导致预测偏大。
    const size_t biomeDataSize = chunk.getBiomes().serialize().size();
    constexpr size_t heightmapSize = static_cast<size_t>(world::CHUNK_WIDTH) * world::CHUNK_WIDTH;
    const u32 sectionMask = calculateSectionMask(chunk);

    size_t size = 4 + 4 + 4 + heightmapSize + 4 + biomeDataSize + 8;
    //       ^   ^   ^                ^               ^   ^            ^-- inhabitedTime(i64)
    //       |   |   |                |               |   +-- 生物群系数据
    //       |   |   |                |               +-- u32 生物群系长度
    //       |   |   |                +-- 高度图(256 字节)
    //       |   |   +-- u32 sectionMask
    //       |   +-- i32 chunkZ
    //       +-- i32 chunkX

    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if ((sectionMask & (1U << i)) == 0) continue;

        const ChunkSection* section = chunk.getSection(i);
        if (!section) continue;

        size += 2 + calculateSectionSize(*section); // u16 长度前缀 + sectionData
    }

    // 扩展高度图块：1 字节掩码 + 每个已初始化 final 类型 256×sizeof(i16)
    size += 1;
    for (HeightmapType type : FINAL_HEIGHTMAP_TYPES) {
        if (chunk.isHeightmapInitialized(type)) {
            size += heightmapSize * sizeof(i16);
        }
    }

    return size;
}

size_t ChunkSerializer::calculateSectionSize(const ChunkSection& section)
{
    // 新格式: 方块数据 (4096 * 4) + 天空光照 (2048) + 方块光照 (2048)
    (void)section;
    return 2 + ChunkSection::VOLUME * sizeof(u32) + NibbleArray::BYTE_SIZE * 2;
}

u32 ChunkSerializer::calculateSectionMask(const ChunkData& chunk)
{
    u32 mask = 0;
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        const ChunkSection* section = chunk.getSection(i);
        if (section && !section->isEmpty()) {
            mask |= (1U << i);
        }
    }
    return mask;
}

// ============================================================================
// ChunkView 实现
// ============================================================================

void ChunkView::calculateChunkDiff(const std::unordered_set<ChunkId>& currentChunks,
    std::vector<ChunkPos>& chunksToLoad,
    std::vector<ChunkPos>& chunksToUnload) const
{
    chunksToLoad.clear();
    chunksToUnload.clear();

    // 获取当前视距内的区块（使用输出参数避免分配）
    std::vector<ChunkPos> viewChunks;
    getChunksInView(viewChunks);

    // 构建视距内区块ID集合
    std::unordered_set<ChunkId> viewChunkIds;
    viewChunkIds.reserve(viewChunks.size());
    for (const auto& pos : viewChunks) {
        viewChunkIds.insert(ChunkId(pos.x, pos.z, 0));
    }

    // 找出需要加载的区块（在视距内但不在当前集合中）
    for (const auto& id : viewChunkIds) {
        if (currentChunks.find(id) == currentChunks.end()) {
            chunksToLoad.emplace_back(id.x, id.z);
        }
    }

    // 找出需要卸载的区块（在当前集合中但不在视距内）
    for (const auto& id : currentChunks) {
        if (viewChunkIds.find(id) == viewChunkIds.end()) {
            chunksToUnload.emplace_back(id.x, id.z);
        }
    }
}

// ============================================================================
// PlayerChunkTracker 实现
// ============================================================================

PlayerChunkTracker::PlayerChunkTracker(PlayerId playerId)
    : m_playerId(playerId)
{}

void PlayerChunkTracker::addLoadedChunk(ChunkCoord x, ChunkCoord z)
{
    m_loadedChunks.insert(ChunkId(x, z, 0));
}

void PlayerChunkTracker::removeLoadedChunk(ChunkCoord x, ChunkCoord z)
{
    m_loadedChunks.erase(ChunkId(x, z, 0));
}

bool PlayerChunkTracker::hasChunk(ChunkCoord x, ChunkCoord z) const
{
    return m_loadedChunks.find(ChunkId(x, z, 0)) != m_loadedChunks.end();
}

void PlayerChunkTracker::updateCenter(ChunkCoord x, ChunkCoord z)
{
    m_view.centerX = x;
    m_view.centerZ = z;
}

void PlayerChunkTracker::calculateChunkUpdates(
    std::vector<ChunkPos>& chunksToLoad, std::vector<ChunkPos>& chunksToUnload)
{
    m_view.calculateChunkDiff(m_loadedChunks, chunksToLoad, chunksToUnload);
}

void PlayerChunkTracker::setViewDistance(i32 distance)
{
    m_view.viewDistance = std::clamp(distance, 2, 32);
}

void PlayerChunkTracker::clear()
{
    m_loadedChunks.clear();
}

// ============================================================================
// ChunkSyncManager 实现
// ============================================================================

std::shared_ptr<PlayerChunkTracker> ChunkSyncManager::getTracker(PlayerId playerId)
{
    auto it = m_trackers.find(playerId);
    if (it != m_trackers.end()) {
        return it->second;
    }

    auto tracker = std::make_shared<PlayerChunkTracker>(playerId);
    tracker->setViewDistance(m_defaultViewDistance);
    m_trackers[playerId] = tracker;
    return tracker;
}

void ChunkSyncManager::removeTracker(PlayerId playerId)
{
    auto it = m_trackers.find(playerId);
    if (it == m_trackers.end()) return;

    // 从区块订阅中移除该玩家
    auto& chunks = it->second->loadedChunks();
    for (const auto& chunkId : chunks) {
        auto subIt = m_chunkSubscribers.find(chunkId);
        if (subIt != m_chunkSubscribers.end()) {
            subIt->second.erase(playerId);
            if (subIt->second.empty()) {
                m_chunkSubscribers.erase(subIt);
            }
        }
    }

    m_trackers.erase(it);
}

void ChunkSyncManager::updatePlayerPosition(PlayerId playerId, f64 x, f64 z)
{
    auto tracker = getTracker(playerId);

    ChunkCoord newChunkX = blockToChunk(x);
    ChunkCoord newChunkZ = blockToChunk(z);

    ChunkCoord oldChunkX = tracker->view().centerX;
    ChunkCoord oldChunkZ = tracker->view().centerZ;

    if (newChunkX != oldChunkX || newChunkZ != oldChunkZ) {
        tracker->updateCenter(newChunkX, newChunkZ);
    }
}

void ChunkSyncManager::calculateUpdates(
    PlayerId playerId, std::vector<ChunkPos>& chunksToLoad, std::vector<ChunkPos>& chunksToUnload)
{
    chunksToLoad.clear();
    chunksToUnload.clear();

    auto tracker = getTracker(playerId);
    tracker->calculateChunkUpdates(chunksToLoad, chunksToUnload);
}

void ChunkSyncManager::markChunkSent(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    auto tracker = getTracker(playerId);

    ChunkId chunkId(x, z, 0);
    tracker->addLoadedChunk(x, z);
    m_chunkSubscribers[chunkId].insert(playerId);
}

void ChunkSyncManager::markChunkUnloaded(PlayerId playerId, ChunkCoord x, ChunkCoord z)
{
    auto tracker = getTracker(playerId);

    ChunkId chunkId(x, z, 0);
    tracker->removeLoadedChunk(x, z);

    auto it = m_chunkSubscribers.find(chunkId);
    if (it != m_chunkSubscribers.end()) {
        it->second.erase(playerId);
        if (it->second.empty()) {
            m_chunkSubscribers.erase(it);
        }
    }
}

} // namespace mc::network

#pragma pop_macro("BYTE_SIZE")
