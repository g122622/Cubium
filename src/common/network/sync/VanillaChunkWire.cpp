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

#include "common/network/sync/VanillaChunkWire.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/backend/java/mappings/JavaBlockStateIdMap.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/JavaBlockEntityTypeIdMap.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/data/PalettedContainer.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 内部常量与工具
// ============================================================================

namespace {

/// vanilla 1.21.11 全局调色板位数(=ceillog2(全局注册表大小),固定值)。
/// block state 全局注册表 ~29671 → ceillog2=15;biome 全局注册表 66 → ceillog2=7。
constexpr int kBlockGlobalPaletteBits = 15;
constexpr int kBiomeGlobalPaletteBits = 7;

/// 等价 Java Mth.ceillog2:n<=1 返回 0;n>=2 返回 ceil(log2(n))。
int ceillog2(int n)
{
    if (n <= 1) {
        return 0;
    }
    int k = 0;
    int p = 1;
    while (p < n) {
        p <<= 1;
        ++k;
    }
    return k;
}

/// 主世界 heightmap 的 bits(=ceillog2(chunkHeight+1)=ceillog2(385)=9)。
constexpr int kHeightmapBits = 9;

/// 计算给定 bits 下 storage 的 long 数(vanilla SimpleBitStorage 公式)。
/// longCount = ceil(entryCount / floor(64/bits))。
int storageLongCount(int bits, int entryCount)
{
    if (bits <= 0) {
        return 0;
    }
    const int valuesPerLong = 64 / bits;
    return (entryCount + valuesPerLong - 1) / valuesPerLong;
}

/**
 * @brief 把 entryCount 个全局 id 按 vanilla PalettedContainer 规则打包成 PalettedContainerWire
 *
 * @param globalIds 长度 entryCount 的全局 id 数组(block state globalId 或 biome registry id)
 * @param entryCount 4096(blocks) 或 64(biomes)
 * @param isBlock true=blocks 阈值,false=biomes 阈值
 */
mc::network::ir::play::PalettedContainerWire packPalettedContainer(const u32* globalIds, int entryCount, bool isBlock)
{
    using PCW = mc::network::ir::play::PalettedContainerWire;

    // 收集去重 palette(storage 里存 palette 下标,故需稳定映射)。
    std::vector<u32> palette;
    std::unordered_map<u32, u32> globalIdToPaletteIndex;
    for (int i = 0; i < entryCount; ++i) {
        const u32 gid = globalIds[i];
        if (globalIdToPaletteIndex.find(gid) == globalIdToPaletteIndex.end()) {
            globalIdToPaletteIndex[gid] = static_cast<u32>(palette.size());
            palette.push_back(gid);
        }
    }
    const int size = static_cast<int>(palette.size());

    // 决定 bits 与 paletteKind(对齐 vanilla Strategy.getConfigurationForBitCount)。
    int bits;
    bool isGlobal = false;
    if (size == 1) {
        bits = 0; // SingleValue
    } else {
        const int needed = ceillog2(size);
        if (isBlock) {
            // blocks: 2-16→4(Linear);17-32→5;33-64→6;65-128→7;129-256→8(HashMap);≥257→Global(15)
            if (needed <= 4) {
                bits = 4;
            } else if (needed <= 8) {
                bits = needed;
            } else {
                bits = kBlockGlobalPaletteBits;
                isGlobal = true;
            }
        } else {
            // biomes: 2→1;3-4→2;5-8→3(Linear);≥9→Global(7)
            if (needed <= 3) {
                bits = needed;
            } else {
                bits = kBiomeGlobalPaletteBits;
                isGlobal = true;
            }
        }
    }

    PCW wire;
    wire.bits = static_cast<u8>(bits);

    // palette payload
    if (size == 1) {
        // SingleValue:paletteGlobalIds 含 1 个全局 id,storage 空
        wire.paletteGlobalIds = {palette[0]};
    } else if (!isGlobal) {
        // Linear/HashMap:paletteGlobalIds 为 size 个全局 id
        wire.paletteGlobalIds = palette;
    }
    // Global:paletteGlobalIds 空,storage 直接存全局 id

    // storage(LSB-first long[])
    if (bits > 0) {
        const int longCount = storageLongCount(bits, entryCount);
        wire.storage.assign(static_cast<size_t>(longCount), 0);
        const u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
        const int valuesPerLong = 64 / bits;
        for (int i = 0; i < entryCount; ++i) {
            const u32 palId = isGlobal ? globalIds[i] : globalIdToPaletteIndex[globalIds[i]];
            const int cell = i / valuesPerLong;
            const int offset = (i - cell * valuesPerLong) * bits;
            wire.storage[static_cast<size_t>(cell)] |= (static_cast<u64>(palId) & mask) << offset;
        }
    }
    return wire;
}

/**
 * @brief 反向:从 PalettedContainerWire 还原 entryCount 个全局 id
 */
std::vector<u32> unpackPalettedContainer(const mc::network::ir::play::PalettedContainerWire& wire, int entryCount)
{
    std::vector<u32> globalIds(static_cast<size_t>(entryCount), 0);
    const int bits = wire.bits;

    if (bits == 0) {
        // SingleValue:paletteGlobalIds[0] 填满
        const u32 v = wire.paletteGlobalIds.empty() ? 0 : wire.paletteGlobalIds[0];
        std::fill(globalIds.begin(), globalIds.end(), v);
        return globalIds;
    }

    const int valuesPerLong = 64 / bits;
    const u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    // Global 判定:palette 空。Global 时 storage 存的就是全局 id 本身。
    const bool globalByEmptyPalette = wire.paletteGlobalIds.empty();

    for (int i = 0; i < entryCount; ++i) {
        const int cell = i / valuesPerLong;
        const int offset = (i - cell * valuesPerLong) * bits;
        const u64 raw = (cell < static_cast<int>(wire.storage.size()))
            ? ((wire.storage[static_cast<size_t>(cell)] >> offset) & mask)
            : 0;
        const u32 palId = static_cast<u32>(raw);
        if (globalByEmptyPalette) {
            globalIds[static_cast<size_t>(i)] = palId;
        } else if (palId < wire.paletteGlobalIds.size()) {
            globalIds[static_cast<size_t>(i)] = wire.paletteGlobalIds[palId];
        } else {
            globalIds[static_cast<size_t>(i)] = 0; // 越界兜底 air/plains
        }
    }
    return globalIds;
}

/**
 * @brief 打包单个 heightmap 为 9-bit LSB-first long[](主世界 37 long)
 *
 * @param heights 256 个值,语义=(最高方块Y+1);空列用 NO_BLOCK_SENTINEL(-65)
 * @param chunkMinY 主世界 -64
 * @return 37 个 u64(vanilla wire 由 codec 加 VarInt 前缀)
 */
std::vector<u64> packHeightmap(const std::array<BlockCoord, Heightmap::SIZE>& heights, int chunkMinY)
{
    constexpr int entryCount = Heightmap::SIZE;                         // 256
    const int longCount = storageLongCount(kHeightmapBits, entryCount); // =37
    std::vector<u64> data(static_cast<size_t>(longCount), 0);
    constexpr int valuesPerLong = 64 / kHeightmapBits; // =7
    constexpr u64 mask = (1ULL << kHeightmapBits) - 1;
    for (int i = 0; i < entryCount; ++i) {
        // vanilla 值 = (highestBlockY+1) - chunkMinY;空列(sentinel 或 <=minY) → 0。
        int v = heights[static_cast<size_t>(i)];
        if (v <= chunkMinY) {
            v = 0; // 空列 / sentinel(NO_BLOCK_SENTINEL=-65 < minY)
        } else {
            v = v - chunkMinY;
        }
        const int cell = i / valuesPerLong;
        const int offset = (i - cell * valuesPerLong) * kHeightmapBits;
        data[static_cast<size_t>(cell)] |= (static_cast<u64>(v) & mask) << offset;
    }
    return data;
}

/**
 * @brief 反向:从 9-bit long[] 还原 256 个 (Y+1) 值(转回绝对 Y+1 语义)
 */
std::array<BlockCoord, Heightmap::SIZE> unpackHeightmap(const std::vector<u64>& data, int chunkMinY)
{
    std::array<BlockCoord, Heightmap::SIZE> heights{};
    constexpr int valuesPerLong = 64 / kHeightmapBits; // =7
    constexpr u64 mask = (1ULL << kHeightmapBits) - 1;
    for (int i = 0; i < Heightmap::SIZE; ++i) {
        const int cell = i / valuesPerLong;
        const int offset = (i - cell * valuesPerLong) * kHeightmapBits;
        const u64 raw =
            (cell < static_cast<int>(data.size())) ? ((data[static_cast<size_t>(cell)] >> offset) & mask) : 0;
        // 还原绝对 Y+1:vanilla 值 0 表示空列 → 用 chunkMinY(项目 sentinel 语义)
        heights[static_cast<size_t>(i)] = static_cast<BlockCoord>(raw) + chunkMinY;
    }
    return heights;
}

/// heightmap type(项目枚举)→ vanilla wire 的 Heightmap.Types id(1/4/5)
u8 heightmapTypeId(HeightmapType type)
{
    switch (type) {
        case HeightmapType::WorldSurface:
            return 1;
        case HeightmapType::MotionBlocking:
            return 4;
        case HeightmapType::MotionBlockingNoLeaves:
            return 5;
        default:
            return 0; // 不发(OceanFloor/WG/LightBlocking 等)
    }
}

/// 在 vector<u64> 表示的 BitSet 里置 bit i(按需扩容)。
void bitSetSet(std::vector<u64>& bitset, int bitIndex)
{
    const size_t wordIndex = static_cast<size_t>(bitIndex) / 64;
    if (wordIndex >= bitset.size()) {
        bitset.resize(wordIndex + 1, 0);
    }
    bitset[wordIndex] |= (1ULL << (static_cast<size_t>(bitIndex) % 64));
}

/// 裁剪 BitSet 尾部全 0 的 long(vanilla 最小数组形式)。
void trimBitSet(std::vector<u64>& bitset)
{
    while (!bitset.empty() && bitset.back() == 0) {
        bitset.pop_back();
    }
}

} // namespace

// ============================================================================
// 服务端:ChunkData → IR
// ============================================================================

Result<mc::network::ir::play::LevelChunkWithLight> VanillaChunkWire::buildLevelChunkWithLightIR(const ChunkData& chunk)
{
    mc::network::ir::play::LevelChunkWithLight ir;
    ir.x = chunk.x();
    ir.z = chunk.z();

    const int chunkMinY = mc::world::MIN_BUILD_HEIGHT;

    // 1. heightmaps:3 个 CLIENT 类型(仅发 isHeightmapInitialized 的)
    {
        const HeightmapType clientTypes[] = {
            HeightmapType::WorldSurface,
            HeightmapType::MotionBlocking,
            HeightmapType::MotionBlockingNoLeaves,
        };
        for (const auto type : clientTypes) {
            if (!chunk.isHeightmapInitialized(type)) {
                continue;
            }
            mc::network::ir::play::HeightmapEntryWire entry;
            entry.typeId = heightmapTypeId(type);
            entry.data = packHeightmap(chunk.getHeightmapData(type), chunkMinY);
            ir.heightmaps.push_back(std::move(entry));
        }
    }

    // 2. sections:24 段
    auto& blockStateMap = mc::network::backend::java::JavaBlockStateIdMap::instance();
    auto& biomeMap = mc::world::biome::JavaBiomeRegistryIdMap::instance();
    ir.sections.reserve(static_cast<size_t>(mc::world::CHUNK_SECTIONS));
    for (int sec = 0; sec < mc::world::CHUNK_SECTIONS; ++sec) {
        mc::network::ir::play::ChunkSectionWire sw;
        const ChunkSection* section = chunk.getSection(sec);
        if (section == nullptr) {
            // 空段:nonEmptyBlockCount=0,states 全 air(0),biomes 全 plains
            sw.nonEmptyBlockCount = 0;
            {
                std::vector<u32> airIds(static_cast<size_t>(ChunkSection::VOLUME), 0);
                sw.states = packPalettedContainer(airIds.data(), ChunkSection::VOLUME, true);
            }
            {
                const u32 plainsId = biomeMap.toJavaRegistryId(mc::world::biome::Biomes::Plains);
                std::vector<u32> biomeIds(static_cast<size_t>(BiomeContainer::SECTION_BIOME_SIZE), plainsId);
                sw.biomes = packPalettedContainer(biomeIds.data(), BiomeContainer::SECTION_BIOME_SIZE, false);
            }
            ir.sections.push_back(std::move(sw));
            continue;
        }

        sw.nonEmptyBlockCount = static_cast<i16>(section->getBlockCount());

        // states:导出 4096 个 stateId → globalId → 打包
        {
            std::vector<u32> globalIds(static_cast<size_t>(ChunkSection::VOLUME), 0);
            for (int i = 0; i < ChunkSection::VOLUME; ++i) {
                const u32 stateId = section->blockStates().get(i);
                globalIds[static_cast<size_t>(i)] = blockStateMap.toJavaGlobalId(stateId);
            }
            sw.states = packPalettedContainer(globalIds.data(), ChunkSection::VOLUME, true);
        }

        // biomes:从 BiomeContainer 取 64 entry → registryId → 打包
        {
            std::vector<u32> globalIds(static_cast<size_t>(BiomeContainer::SECTION_BIOME_SIZE), 0);
            const BiomeContainer& biomes = chunk.getBiomes();
            for (int y = 0; y < 4; ++y) {
                for (int z = 0; z < 4; ++z) {
                    for (int x = 0; x < 4; ++x) {
                        const mc::BiomeId bid = biomes.getBiome(sec, x, y, z);
                        const int idx = y * 16 + z * 4 + x;
                        globalIds[static_cast<size_t>(idx)] = biomeMap.toJavaRegistryId(bid);
                    }
                }
            }
            sw.biomes = packPalettedContainer(globalIds.data(), BiomeContainer::SECTION_BIOME_SIZE, false);
        }

        ir.sections.push_back(std::move(sw));
    }

    // 3. blockEntities
    auto& beTypeMap = mc::JavaBlockEntityTypeIdMap::instance();
    {
        const auto entities = chunk.getAllBlockEntities();
        for (const BlockEntity* entity : entities) {
            if (entity == nullptr) {
                continue;
            }
            mc::network::ir::play::BlockEntityInfoWire bew;
            const BlockPos pos = entity->getPos();
            bew.packedXZ =
                static_cast<u8>((static_cast<u32>(pos.localX() & 0xF) << 4) | static_cast<u32>(pos.localZ() & 0xF));
            bew.y = static_cast<i16>(pos.y);
            bew.typeRegistryId = beTypeMap.toJavaRegistryId(entity->getType());
            bew.tag = std::make_shared<mc::nbt::CompoundTag>(entity->getUpdateTag());
            ir.blockEntities.push_back(std::move(bew));
        }
    }

    // 4. lightData:26 段 sky/block nibbles
    {
        constexpr int lightSections = mc::world::CHUNK_SECTIONS + 2; // 26
        const auto& skyNibbles = chunk.skyNibbles();
        const auto& blockNibbles = chunk.blockNibbles();

        auto buildMasks = [](const std::array<mc::SWMRNibbleArray, 26>& nibbles,
                              std::vector<u64>& yMask,
                              std::vector<u64>& emptyMask,
                              std::vector<std::vector<u8>>& updates) {
            yMask.clear();
            emptyMask.clear();
            updates.clear();
            for (int i = 0; i < lightSections; ++i) {
                const auto& nibble = nibbles[static_cast<size_t>(i)];
                if (nibble.isNullVisible() || nibble.isUninitializedVisible()) {
                    continue; // 无数据,跳过(两 mask 都不 set)
                }
                std::vector<u8> bytes = nibble.toByteArray();
                if (bytes.empty()) {
                    // 空段(全 0):标 emptyMask,不发 nibble
                    bitSetSet(emptyMask, i);
                } else {
                    bitSetSet(yMask, i);
                    updates.push_back(std::move(bytes));
                }
            }
            trimBitSet(yMask);
            trimBitSet(emptyMask);
        };

        buildMasks(skyNibbles, ir.lightMasks[0], ir.lightMasks[2], ir.lightUpdates[0]);
        buildMasks(blockNibbles, ir.lightMasks[1], ir.lightMasks[3], ir.lightUpdates[1]);
    }

    return ir;
}

// ============================================================================
// 客户端:IR → ChunkData
// ============================================================================

Result<std::unique_ptr<ChunkData>> VanillaChunkWire::readLevelChunkWithLightIR(
    const mc::network::ir::play::LevelChunkWithLight& ir)
{
    auto chunk = std::make_unique<ChunkData>(ir.x, ir.z);
    const int chunkMinY = mc::world::MIN_BUILD_HEIGHT;

    auto& blockStateMap = mc::network::backend::java::JavaBlockStateIdMap::instance();
    auto& biomeMap = mc::world::biome::JavaBiomeRegistryIdMap::instance();
    auto& beTypeMap = mc::JavaBlockEntityTypeIdMap::instance();

    // heightmaps
    for (const auto& entry : ir.heightmaps) {
        HeightmapType type = HeightmapType::COUNT;
        switch (entry.typeId) {
            case 1:
                type = HeightmapType::WorldSurface;
                break;
            case 4:
                type = HeightmapType::MotionBlocking;
                break;
            case 5:
                type = HeightmapType::MotionBlockingNoLeaves;
                break;
            default:
                continue;
        }
        std::array<BlockCoord, Heightmap::SIZE> heights = unpackHeightmap(entry.data, chunkMinY);
        chunk->setHeightmapFromStorage(type, heights);
    }

    // sections
    const int sectionCount = static_cast<int>(ir.sections.size());
    for (int sec = 0; sec < sectionCount && sec < mc::world::CHUNK_SECTIONS; ++sec) {
        const auto& sw = ir.sections[static_cast<size_t>(sec)];
        ChunkSection* section = chunk->createSection(sec);
        section->setBlockCount(static_cast<u16>(sw.nonEmptyBlockCount));

        // states
        {
            std::vector<u32> globalIds = unpackPalettedContainer(sw.states, ChunkSection::VOLUME);
            for (int i = 0; i < ChunkSection::VOLUME; ++i) {
                const u32 stateId = blockStateMap.fromJavaGlobalId(globalIds[static_cast<size_t>(i)]);
                section->setBlockStateIdFast(i, stateId);
            }
        }
        // biomes
        {
            std::vector<u32> globalIds = unpackPalettedContainer(sw.biomes, BiomeContainer::SECTION_BIOME_SIZE);
            BiomeContainer& biomes = chunk->getBiomes();
            for (int y = 0; y < 4; ++y) {
                for (int z = 0; z < 4; ++z) {
                    for (int x = 0; x < 4; ++x) {
                        const int idx = y * 16 + z * 4 + x;
                        const u32 regId = globalIds[static_cast<size_t>(idx)];
                        biomes.setBiome(sec, x, y, z, biomeMap.fromJavaRegistryId(regId));
                    }
                }
            }
        }
    }

    // blockEntities
    for (const auto& bew : ir.blockEntities) {
        const int relX = (bew.packedXZ >> 4) & 0xF;
        const int relZ = bew.packedXZ & 0xF;
        const BlockPos pos(ir.x * 16 + relX, bew.y, ir.z * 16 + relZ);
        const BlockEntityType type = beTypeMap.fromJavaRegistryId(bew.typeRegistryId);
        auto entity = mc::blockentity::BlockEntityRegistry::instance().create(type, pos);
        if (entity == nullptr) {
            continue;
        }
        if (bew.tag != nullptr) {
            entity->loadFromNBT(*bew.tag);
        }
        chunk->setBlockEntity(pos, std::move(entity));
    }

    // lightData:客户端一般不消费(不持光照引擎,光照由服务端权威推送),此处不还原 nibbles。

    return chunk;
}

} // namespace mc::world::chunk
