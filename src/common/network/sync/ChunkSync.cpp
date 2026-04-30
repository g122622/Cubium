#include "ChunkSync.hpp"
#include "../../world/block/Block.hpp"
#include "../../world/WorldConstants.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace mc::network {

// ============================================================================
// ChunkSerializer 实现
// ============================================================================

Result<std::vector<u8>> ChunkSerializer::serializeChunk(const ChunkData& chunk) {
    PacketSerializer ser;

    // 写入区块坐标
    ser.writeI32(chunk.x());
    ser.writeI32(chunk.z());

    // 写入区块段位掩码
    u16 sectionMask = calculateSectionMask(chunk);
    ser.writeU16(sectionMask);

    // 写入高度图
    for (i32 i = 0; i < 256; ++i) {
        ser.writeU8(static_cast<u8>(chunk.getHighestBlock(i % 16, i / 16)));
    }

    // 写入生物群系
    const auto biomeData = chunk.getBiomes().serialize();
    ser.writeU8(static_cast<u8>(BiomeContainer::BIOME_SIZE));
    ser.writeBytes(biomeData);

    // 写入区块段数据
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if ((sectionMask & (1 << i)) == 0) continue;

        const ChunkSection* section = chunk.getSection(i);
        if (!section) continue;

        auto sectionData = ChunkSerializer::serializeSection(*section);
        if (sectionData.empty()) {
            return Error(ErrorCode::InvalidData, "Failed to serialize section");
        }

        ser.writeU16(static_cast<u16>(sectionData.size()));
        ser.writeBytes(sectionData);
    }

    return ser.buffer();
}

std::vector<u8> ChunkSerializer::serializeSection(const ChunkSection& section) {
    std::vector<u8> data;
    // 预分配空间: 2字节blockCount + 4096*4字节方块数据 + 2048字节天空光照 + 2048字节方块光照
    constexpr size_t SECTION_DATA_SIZE = 2 + ChunkSection::VOLUME * 4 + NibbleArray::BYTE_SIZE * 2;
    data.reserve(SECTION_DATA_SIZE);

    // 写入非空方块数量
    u16 blockCount = section.getBlockCount();
    data.push_back(static_cast<u8>(blockCount >> 8));
    data.push_back(static_cast<u8>(blockCount & 0xFF));

    // 写入方块状态ID (u32格式)
    for (i32 i = 0; i < ChunkSection::VOLUME; ++i) {
        u32 stateId = section.getBlockStateIdFast(i);
        // 写入4字节
        data.push_back(static_cast<u8>(stateId & 0xFF));
        data.push_back(static_cast<u8>((stateId >> 8) & 0xFF));
        data.push_back(static_cast<u8>((stateId >> 16) & 0xFF));
        data.push_back(static_cast<u8>((stateId >> 24) & 0xFF));
    }

    // 写入天空光照数据（从NibbleArray获取实际数据）
    const NibbleArray& skyLight = section.skyLightNibble();
    const std::vector<u8>& skyData = skyLight.data();
    if (skyData.empty()) {
        // 如果为空，填充默认值15（全亮）
        data.insert(data.end(), NibbleArray::BYTE_SIZE, 0xFF);
    } else {
        data.insert(data.end(), skyData.begin(), skyData.end());
    }

    // 写入方块光照数据（从NibbleArray获取实际数据）
    const NibbleArray& blockLight = section.blockLightNibble();
    const std::vector<u8>& blockData = blockLight.data();
    if (blockData.empty()) {
        // 如果为空，填充默认值0（无光照）
        data.insert(data.end(), NibbleArray::BYTE_SIZE, 0x00);
    } else {
        data.insert(data.end(), blockData.begin(), blockData.end());
    }

    return data;
}

Result<std::unique_ptr<ChunkData>> ChunkSerializer::deserializeChunk(
    ChunkCoord x, ChunkCoord z,
    const std::vector<u8>& data
) {
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
    auto maskResult = deser.readU16();
    if (maskResult.failed()) {
        return maskResult.error();
    }
    u16 sectionMask = maskResult.value();

    // 跳过高度图 (256 bytes)
    auto heightmapResult = deser.readBytes(256);
    if (heightmapResult.failed()) {
        return heightmapResult.error();
    }

    // 读取生物群系
    auto biomeCountResult = deser.readU8();
    if (biomeCountResult.failed()) {
        return biomeCountResult.error();
    }

    const u8 biomeCount = biomeCountResult.value();
    if (biomeCount > 0) {
        auto biomeDataResult = deser.readBytes(static_cast<size_t>(biomeCount) * sizeof(BiomeId));
        if (biomeDataResult.failed()) {
            return biomeDataResult.error();
        }

        const auto& biomeData = biomeDataResult.value();
        auto biomeContainerResult = BiomeContainer::deserialize(biomeData.data(), biomeData.size());
        if (biomeContainerResult.failed()) {
            return biomeContainerResult.error();
        }

        chunk->setBiomes(std::move(biomeContainerResult.value()));
    }

    // 读取区块段
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if ((sectionMask & (1 << i)) == 0) continue;

        auto sizeResult = deser.readU16();
        if (sizeResult.failed()) {
            return sizeResult.error();
        }
        u16 sectionSize = sizeResult.value();

        auto sectionDataResult = deser.readBytes(sectionSize);
        if (sectionDataResult.failed()) {
            return sectionDataResult.error();
        }

        ChunkSection* section = chunk->createSection(i);
        if (!section) {
            return Error(ErrorCode::OutOfMemory, "Failed to create section");
        }

        // 解析段数据
        const auto& sectionData = sectionDataResult.value();
        if (sectionData.size() < 2) {
            return Error(ErrorCode::InvalidData, "Section data too small");
        }

        // 读取方块数据 (u32状态ID格式)
        size_t offset = 2;  // 跳过 blockCount
        int sectionBlocks = 0;
        for (i32 j = 0; j < ChunkSection::VOLUME && offset + 3 < sectionData.size(); ++j) {
            u32 stateId = static_cast<u32>(sectionData[offset]) |
                         (static_cast<u32>(sectionData[offset + 1]) << 8) |
                         (static_cast<u32>(sectionData[offset + 2]) << 16) |
                         (static_cast<u32>(sectionData[offset + 3]) << 24);
            section->setBlockStateIdFast(j, stateId);

            // 检查是否为非空气方块
            const BlockState* state = Block::getBlockState(stateId);
            if (state && !state->isAir()) {
                sectionBlocks++;
            }
            offset += 4;
        }

        // 更新 blockCount
        section->setBlockCount(static_cast<u16>(sectionBlocks));

        // 读取光照数据
        // 天空光照: 2048字节 (每方块4位)
        // 方块光照: 2048字节 (每方块4位)
        constexpr size_t LIGHT_DATA_SIZE = NibbleArray::BYTE_SIZE * 2;  // 4096字节

        if (offset + LIGHT_DATA_SIZE <= sectionData.size()) {
            // 读取天空光照
            NibbleArray& skyLight = section->skyLightNibble();
            skyLight.data().resize(NibbleArray::BYTE_SIZE);
            std::memcpy(skyLight.data().data(), sectionData.data() + offset, NibbleArray::BYTE_SIZE);
            offset += NibbleArray::BYTE_SIZE;

            // 读取方块光照
            NibbleArray& blockLight = section->blockLightNibble();
            blockLight.data().resize(NibbleArray::BYTE_SIZE);
            std::memcpy(blockLight.data().data(), sectionData.data() + offset, NibbleArray::BYTE_SIZE);
        }
    }

    chunk->setFullyGenerated(true);
    chunk->setLoaded(true);

    return chunk;
}

Result<std::unique_ptr<ChunkSection>> ChunkSerializer::deserializeChunkSection(
    const u8* data, size_t size
) {
    // 最小大小: 2字节blockCount + 4096*4字节方块数据 + 2048字节天空光照 + 2048字节方块光照
    constexpr size_t MIN_SIZE = 2 + ChunkSection::VOLUME * 4 + NibbleArray::BYTE_SIZE * 2;
    if (size < MIN_SIZE) {
        return Error(ErrorCode::InvalidData, "Section data too small for light data");
    }

    auto section = std::make_unique<ChunkSection>();

    // 读取非空方块数量
    u16 blockCount = (static_cast<u16>(data[0]) << 8) | data[1];

    // 读取方块数据 (u32状态ID格式)
    size_t offset = 2;
    for (i32 j = 0; j < ChunkSection::VOLUME; ++j) {
        u32 stateId = static_cast<u32>(data[offset]) |
                     (static_cast<u32>(data[offset + 1]) << 8) |
                     (static_cast<u32>(data[offset + 2]) << 16) |
                     (static_cast<u32>(data[offset + 3]) << 24);
        section->setBlockStateIdFast(j, stateId);
        offset += 4;
    }

    // 读取天空光照
    NibbleArray& skyLight = section->skyLightNibble();
    skyLight.data().resize(NibbleArray::BYTE_SIZE);
    std::memcpy(skyLight.data().data(), data + offset, NibbleArray::BYTE_SIZE);
    offset += NibbleArray::BYTE_SIZE;

    // 读取方块光照
    NibbleArray& blockLight = section->blockLightNibble();
    blockLight.data().resize(NibbleArray::BYTE_SIZE);
    std::memcpy(blockLight.data().data(), data + offset, NibbleArray::BYTE_SIZE);

    section->setBlockCount(blockCount);

    return section;
}

size_t ChunkSerializer::calculateChunkSize(const ChunkData& chunk) {
    size_t size = 4 + 4 + 2 + 256 + 1 + chunk.getBiomes().serialize().size(); // 坐标 + 位掩码 + 高度图 + 生物群系

    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if (chunk.hasSection(i)) {
            size += 2 + calculateSectionSize(*chunk.getSection(i));
        }
    }

    return size;
}

size_t ChunkSerializer::calculateSectionSize(const ChunkSection& section) {
    // 新格式: 方块数据 (4096 * 4) + 天空光照 (2048) + 方块光照 (2048)
    (void)section;
    return 2 + ChunkSection::VOLUME * 4 + 2048 + 2048;
}

u16 ChunkSerializer::calculateSectionMask(const ChunkData& chunk) {
    u16 mask = 0;
    for (i32 i = 0; i < world::CHUNK_SECTIONS; ++i) {
        if (chunk.hasSection(i) && !chunk.getSection(i)->isEmpty()) {
            mask |= (1 << i);
        }
    }
    return mask;
}

// ============================================================================
// ChunkView 实现
// ============================================================================

void ChunkView::calculateChunkDiff(
    const std::unordered_set<ChunkId>& currentChunks,
    std::vector<ChunkPos>& chunksToLoad,
    std::vector<ChunkPos>& chunksToUnload
) const {
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
{
}

void PlayerChunkTracker::addLoadedChunk(ChunkCoord x, ChunkCoord z) {
    m_loadedChunks.insert(ChunkId(x, z, 0));
}

void PlayerChunkTracker::removeLoadedChunk(ChunkCoord x, ChunkCoord z) {
    m_loadedChunks.erase(ChunkId(x, z, 0));
}

bool PlayerChunkTracker::hasChunk(ChunkCoord x, ChunkCoord z) const {
    return m_loadedChunks.find(ChunkId(x, z, 0)) != m_loadedChunks.end();
}

void PlayerChunkTracker::updateCenter(ChunkCoord x, ChunkCoord z) {
    m_view.centerX = x;
    m_view.centerZ = z;
}

void PlayerChunkTracker::calculateChunkUpdates(
    std::vector<ChunkPos>& chunksToLoad,
    std::vector<ChunkPos>& chunksToUnload
) {
    m_view.calculateChunkDiff(m_loadedChunks, chunksToLoad, chunksToUnload);
}

void PlayerChunkTracker::setViewDistance(i32 distance) {
    m_view.viewDistance = std::clamp(distance, 2, 32);
}

void PlayerChunkTracker::clear() {
    m_loadedChunks.clear();
}

// ============================================================================
// ChunkSyncManager 实现
// ============================================================================

std::shared_ptr<PlayerChunkTracker> ChunkSyncManager::getTracker(PlayerId playerId) {
    auto it = m_trackers.find(playerId);
    if (it != m_trackers.end()) {
        return it->second;
    }

    auto tracker = std::make_shared<PlayerChunkTracker>(playerId);
    tracker->setViewDistance(m_defaultViewDistance);
    m_trackers[playerId] = tracker;
    return tracker;
}

void ChunkSyncManager::removeTracker(PlayerId playerId) {
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

void ChunkSyncManager::updatePlayerPosition(PlayerId playerId, f64 x, f64 z) {
    auto tracker = getTracker(playerId);
    if (!tracker) return;

    ChunkCoord newChunkX = blockToChunk(x);
    ChunkCoord newChunkZ = blockToChunk(z);

    ChunkCoord oldChunkX = tracker->view().centerX;
    ChunkCoord oldChunkZ = tracker->view().centerZ;

    if (newChunkX != oldChunkX || newChunkZ != oldChunkZ) {
        tracker->updateCenter(newChunkX, newChunkZ);
    }
}

void ChunkSyncManager::calculateUpdates(
    PlayerId playerId,
    std::vector<ChunkPos>& chunksToLoad,
    std::vector<ChunkPos>& chunksToUnload
) {
    chunksToLoad.clear();
    chunksToUnload.clear();

    auto tracker = getTracker(playerId);
    if (!tracker) return;

    tracker->calculateChunkUpdates(chunksToLoad, chunksToUnload);
}

void ChunkSyncManager::markChunkSent(PlayerId playerId, ChunkCoord x, ChunkCoord z) {
    auto tracker = getTracker(playerId);
    if (!tracker) return;

    ChunkId chunkId(x, z, 0);
    tracker->addLoadedChunk(x, z);
    m_chunkSubscribers[chunkId].insert(playerId);
}

void ChunkSyncManager::markChunkUnloaded(PlayerId playerId, ChunkCoord x, ChunkCoord z) {
    auto tracker = getTracker(playerId);
    if (!tracker) return;

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
