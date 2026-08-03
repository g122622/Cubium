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

#include "JavaChunkReader.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "common/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <fmt/format.h>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

namespace mc::world::storage::reader::java {

using namespace mc::nbt;
using namespace mc::nbt::tags;

namespace {
const compound_tag* getCompound(const compound_tag& parent, const std::string& name)
{
    auto it = parent.value.find(name);
    if (it == parent.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const compound_tag*>(it->second.get());
}

const list_tag* getList(const compound_tag& parent, const std::string& name)
{
    auto it = parent.value.find(name);
    if (it == parent.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const list_tag*>(it->second.get());
}
} // namespace

JavaChunkReader::JavaChunkReader(JavaBlockStateMapper& blockMapper, JavaBiomeMapper& biomeMapper) noexcept
    : m_blockMapper(blockMapper)
    , m_biomeMapper(biomeMapper)
{}

std::vector<u32> JavaChunkReader::unpackLongArray(
    const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount, bool usePaddedFormat)
{
    if (bitsPerEntry == 0 || data.empty()) {
        return std::vector<u32>(static_cast<size_t>(entryCount), 0);
    }
    return usePaddedFormat ? unpackPaddedLongArray(data, bitsPerEntry, entryCount)
                           : unpackCompactLongArray(data, bitsPerEntry, entryCount);
}

std::vector<u32> JavaChunkReader::unpackCompactLongArray(const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount)
{
    std::vector<u32> result(static_cast<size_t>(entryCount), 0);
    const u64 mask = (1ULL << bitsPerEntry) - 1ULL;
    for (i32 i = 0; i < entryCount; ++i) {
        const i32 bitIndex = i * bitsPerEntry;
        const i32 startLong = bitIndex / 64;
        const i32 endLong = ((i + 1) * bitsPerEntry - 1) / 64;
        const i32 startBitSubIndex = bitIndex % 64;
        if (startLong >= static_cast<i32>(data.size())) {
            break;
        }

        u64 value;
        if (startLong == endLong || endLong >= static_cast<i32>(data.size())) {
            value = (static_cast<u64>(data[static_cast<size_t>(startLong)]) >> startBitSubIndex) & mask;
        } else {
            const i32 endBitSubIndex = 64 - startBitSubIndex;
            value = ((static_cast<u64>(data[static_cast<size_t>(startLong)]) >> startBitSubIndex) |
                        (static_cast<u64>(data[static_cast<size_t>(endLong)]) << endBitSubIndex)) &
                mask;
        }
        result[static_cast<size_t>(i)] = static_cast<u32>(value);
    }
    return result;
}

std::vector<u32> JavaChunkReader::unpackPaddedLongArray(const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount)
{
    std::vector<u32> result(static_cast<size_t>(entryCount), 0);
    const u64 mask = (1ULL << bitsPerEntry) - 1ULL;
    const i32 valuesPerLong = 64 / bitsPerEntry;
    const i32 magicIndex = 3 * (valuesPerLong - 1);
    const u64 divideMul = static_cast<u64>(static_cast<u32>(MAGIC[static_cast<size_t>(magicIndex)]));
    const u64 divideAdd = static_cast<u64>(static_cast<u32>(MAGIC[static_cast<size_t>(magicIndex + 1)]));
    const i32 divideShift = MAGIC[static_cast<size_t>(magicIndex + 2)];
    for (i32 i = 0; i < entryCount; ++i) {
        const i32 longIndex =
            static_cast<i32>(((static_cast<u64>(i) * divideMul + divideAdd) >> 32U) >> static_cast<u32>(divideShift));
        const i32 bitOffset = (i - longIndex * valuesPerLong) * bitsPerEntry;
        if (longIndex >= static_cast<i32>(data.size())) {
            break;
        }
        result[static_cast<size_t>(i)] =
            static_cast<u32>((static_cast<u64>(data[static_cast<size_t>(longIndex)]) >> bitOffset) & mask);
    }
    return result;
}

Result<void> JavaChunkReader::readSection(
    const compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY, bool hasSkyLight)
{
    return readBlockStates(sectionNbt, chunk, sectionY, hasSkyLight);
}

Result<void> JavaChunkReader::readBlockStates(
    const compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY, bool hasSkyLight)
{
    // 检查是否有方块状态数据
    if (sectionNbt.value.count("block_states") == 0 && sectionNbt.value.count("Palette") == 0) {
        // 空section
        return {};
    }

    std::vector<u32> paletteIds;
    const compound_tag* blockStatesNbt = nullptr;

    // 1.18+ 格式：block_states compound
    if (sectionNbt.value.count("block_states") != 0) {
        blockStatesNbt = getCompound(sectionNbt, "block_states");
    }

    // 读取调色板
    const list_tag* paletteNbt = nullptr;
    if (blockStatesNbt && blockStatesNbt->value.count("palette") != 0) {
        paletteNbt = getList(*blockStatesNbt, "palette");
    } else if (sectionNbt.value.count("Palette") != 0) {
        // 1.16.5 格式
        paletteNbt = getList(sectionNbt, "Palette");
    }

    if (!paletteNbt) {
        return {};
    }

    // 解析调色板条目
    std::vector<PaletteEntry> paletteEntries;
    paletteEntries.reserve(paletteNbt->size());
    for (size_t i = 0; i < paletteNbt->size(); ++i) {
        auto entryPtr = (*paletteNbt)[i];
        auto* entryNbt = dynamic_cast<const compound_tag*>(entryPtr.get());
        if (!entryNbt) {
            paletteEntries.push_back({"minecraft:air", {}});
            continue;
        }

        PaletteEntry entry;
        if (entryNbt->value.count("Name") != 0) {
            entry.blockName = entryNbt->get<string_tag>("Name");
        } else {
            entry.blockName = "minecraft:air";
        }

        if (entryNbt->value.count("Properties") != 0) {
            const auto* propsNbt = getCompound(*entryNbt, "Properties");
            if (propsNbt) {
                for (const auto& [key, value] : propsNbt->value) {
                    auto* strValue = dynamic_cast<const string_tag*>(value.get());
                    if (strValue) {
                        entry.properties[key] = strValue->value;
                    }
                }
            }
        }

        paletteEntries.push_back(std::move(entry));
    }

    // 映射到内部 stateId
    paletteIds = m_blockMapper.mapPalette(paletteEntries);

    // 读取方块数据（long array）
    std::vector<u32> blockIndices;

    if (blockStatesNbt && blockStatesNbt->value.count("data") != 0) {
        // 1.18+ 格式
        auto& dataArray = blockStatesNbt->get<longarray_tag>("data");
        i32 bitsPerEntry =
            std::max(4, static_cast<i32>(std::ceil(std::log2(std::max(static_cast<i32>(paletteIds.size()), 2)))));
        blockIndices = unpackLongArray(dataArray, bitsPerEntry, ChunkSection::VOLUME, false);
    } else if (sectionNbt.value.count("BlockStates") != 0) {
        // 1.16.5 格式
        auto& dataArray = sectionNbt.get<longarray_tag>("BlockStates");
        i32 bitsPerEntry =
            std::max(4, static_cast<i32>(std::ceil(std::log2(std::max(static_cast<i32>(paletteIds.size()), 2)))));
        blockIndices = unpackLongArray(dataArray, bitsPerEntry, ChunkSection::VOLUME, true);
    }

    // 如果只有单个调色板条目且无数据数组，填充整个section
    if (blockIndices.empty()) {
        if (paletteIds.size() == 1) {
            blockIndices.resize(ChunkSection::VOLUME, 0);
        } else {
            return {};
        }
    }

    // 创建 ChunkSection 并填充方块状态
    const i32 sectionIndex = world::sectionCoordToIndex(sectionY);
    if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
        return Error(ErrorCode::ChunkCorrupted,
            fmt::format("Java section Y {} maps outside chunk section range [0, {})", sectionY, world::CHUNK_SECTIONS));
    }
    ChunkSection* section = chunk.createSection(sectionIndex);
    if (!section) {
        return Error(ErrorCode::ChunkCorrupted, fmt::format("Failed to create section {} for chunk", sectionY));
    }

    for (i32 y = 0; y < ChunkSection::SIZE; ++y) {
        for (i32 z = 0; z < ChunkSection::SIZE; ++z) {
            for (i32 x = 0; x < ChunkSection::SIZE; ++x) {
                i32 index = y * ChunkSection::SIZE * ChunkSection::SIZE + z * ChunkSection::SIZE + x;
                u32 paletteIndex = (index < static_cast<i32>(blockIndices.size())) ? blockIndices[index] : 0;
                u32 stateId = (paletteIndex < paletteIds.size()) ? paletteIds[paletteIndex] : 0;
                section->setBlockStateId(x, y, z, stateId);
            }
        }
    }

    // 读取光照数据
    readLightData(sectionNbt, *section, hasSkyLight);

    return {};
}

void JavaChunkReader::readLightData(const compound_tag& sectionNbt, ChunkSection& section, bool hasSkyLight)
{
    auto applyNibble = [&section](const bytearray_tag& bytes, bool isSky) {
        if (bytes.value.size() < NibbleArray::BYTE_SIZE) {
            return;
        }
        for (i32 i = 0; i < ChunkSection::VOLUME; ++i) {
            const i32 x = i & 0xF;
            const i32 z = (i >> 4) & 0xF;
            const i32 y = (i >> 8) & 0xF;
            const i32 nibbleIndex = i >> 1;
            const bool lower = (i & 1) == 0;
            const u8 packed = static_cast<u8>(bytes.value[static_cast<size_t>(nibbleIndex)]);
            const u8 value = lower ? (packed & 0x0F) : ((packed >> 4) & 0x0F);
            if (isSky) {
                section.skyLightNibble().set(x, y, z, value);
            } else {
                section.blockLightNibble().set(x, y, z, value);
            }
        }
    };

    // 只在有天空光照的维度（主世界）中加载天空光照数据
    // 对应 MC Java SerializableChunkData.read() 中的 hasSkyLight 门控
    if (hasSkyLight && sectionNbt.value.count("SkyLight") != 0) {
        applyNibble(sectionNbt.get<bytearray_tag>("SkyLight"), true);
    }
    if (sectionNbt.value.count("BlockLight") != 0) {
        applyNibble(sectionNbt.get<bytearray_tag>("BlockLight"), false);
    }
}

BiomeId JavaChunkReader::mapBiomeName(const std::string& biomeName) const
{
    return m_biomeMapper.mapBiome(biomeName);
}

BiomeId JavaChunkReader::mapBiomeId(i32 biomeId) const
{
    return m_biomeMapper.mapBiome(biomeId);
}

Result<std::optional<JavaChunkReader::SectionBiomePalette>> JavaChunkReader::readSectionBiomePalette(
    const compound_tag& sectionNbt) const
{
    const compound_tag* biomesNbt = getCompound(sectionNbt, "biomes");
    if (biomesNbt == nullptr) {
        return std::optional<SectionBiomePalette>{};
    }

    const list_tag* paletteTag = getList(*biomesNbt, "palette");
    if (paletteTag == nullptr || paletteTag->size() == 0) {
        return std::optional<SectionBiomePalette>{};
    }

    i32 sectionY = 0;
    auto yIt = sectionNbt.value.find("Y");
    if (yIt == sectionNbt.value.end()) {
        return Error(ErrorCode::ChunkCorrupted, "Java biome section missing Y");
    }
    if (yIt->second->id() == TagId::Byte) {
        sectionY = static_cast<i8>(sectionNbt.get<byte_tag>("Y"));
    } else if (yIt->second->id() == TagId::Int) {
        sectionY = static_cast<i32>(sectionNbt.get<int_tag>("Y"));
    } else {
        return Error(ErrorCode::ChunkCorrupted, "Java biome section has invalid Y tag type");
    }

    SectionBiomePalette result;
    result.sectionY = sectionY;
    result.palette.reserve(paletteTag->size());

    for (size_t i = 0; i < paletteTag->size(); ++i) {
        auto entry = (*paletteTag)[i];
        if (entry->id() != TagId::String) {
            return Error(ErrorCode::ChunkCorrupted, "Java biome palette entry is not a string");
        }
        const auto* biomeName = dynamic_cast<const string_tag*>(entry.get());
        MC_ASSERT_RELEASE(biomeName != nullptr);
        result.palette.push_back(mapBiomeName(biomeName->value));
    }

    auto dataIt = biomesNbt->value.find("data");
    if (dataIt == biomesNbt->value.end() || result.palette.size() == 1) {
        return std::optional<SectionBiomePalette>(std::move(result));
    }
    if (dataIt->second->id() != TagId::LongArray) {
        return Error(ErrorCode::ChunkCorrupted, "Java biome palette data tag is not a long array");
    }

    const auto& packed = biomesNbt->get<longarray_tag>("data");
    const i32 bitsPerEntry =
        std::max(1, static_cast<i32>(std::ceil(std::log2(std::max(static_cast<i32>(result.palette.size()), 2)))));
    result.indices = unpackLongArray(packed, bitsPerEntry, BiomeContainer::SECTION_BIOME_SIZE, false);
    return std::optional<SectionBiomePalette>(std::move(result));
}

} // namespace mc::world::storage::reader::java

#pragma pop_macro("BYTE_SIZE")
