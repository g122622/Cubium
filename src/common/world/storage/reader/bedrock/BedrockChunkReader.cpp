/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software are
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

#include "BedrockChunkReader.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include <algorithm>
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc::world::storage::reader::bedrock {

using namespace mc::nbt;
using namespace mc::nbt::tags;

namespace {

std::string buildCacheKey(const std::string& blockName, const std::unordered_map<std::string, std::string>& states)
{
    std::vector<std::pair<std::string, std::string>> ordered(states.begin(), states.end());
    std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

    std::string key = blockName;
    for (const auto& [name, value] : ordered) {
        key += ',';
        key += name;
        key += '=';
        key += value;
    }
    return key;
}

const compound_tag* tryGetCompound(const compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const compound_tag*>(it->second.get());
}

} // namespace

BedrockChunkReader::BedrockChunkReader(BedrockBiomeMapper& biomeMapper)
    : m_biomeMapper(biomeMapper)
{}

Result<std::unique_ptr<ChunkData>> BedrockChunkReader::readChunk(
    i32 chunkX, i32 chunkZ, DimensionId dimension, BedrockLevelDb& db)
{
    auto chunk = std::make_unique<ChunkData>(chunkX, chunkZ);
    bool hasAnySection = false;

    // 读取区块版本
    auto versionKey = (dimension == 0)
        ? BedrockLevelDb::buildKey(chunkX, chunkZ, BedrockLevelDb::ChunkType::Version)
        : BedrockLevelDb::buildKey(chunkX, chunkZ, dimension, BedrockLevelDb::ChunkType::Version);
    auto versionResult = db.get(versionKey);
    // 版本号用于确定数据格式，暂时不需要特殊处理

    // 读取子区块数据（SubChunkPrefix，类型 47）
    // 基岩版子区块 Y 从 -4 到 19（1.18+），或 0 到 15（旧版）
    for (i8 subY = -64; subY < 64; ++subY) {
        auto subKey = (dimension == 0)
            ? BedrockLevelDb::buildSubChunkKey(chunkX, chunkZ, BedrockLevelDb::ChunkType::SubChunkPrefix, subY)
            : BedrockLevelDb::buildSubChunkKey(
                  chunkX, chunkZ, dimension, BedrockLevelDb::ChunkType::SubChunkPrefix, subY);

        auto subResult = db.get(subKey);
        if (subResult.failed()) {
            continue;
        }

        auto& subData = subResult.value();
        if (!subData.has_value()) {
            continue;
        }

        auto sectionResult = readSubChunk(subData.value(), subY, *chunk);
        if (sectionResult.failed()) {
            spdlog::debug("BedrockChunkReader: Failed to read sub-chunk {} for ({}, {}): {}",
                subY,
                chunkX,
                chunkZ,
                sectionResult.error().message());
            continue;
        }

        hasAnySection = true;
    }

    if (!hasAnySection) {
        return std::unique_ptr<ChunkData>{};
    }

    // 读取生物群系数据
    // 优先尝试新版 BiomeState（类型 53），其次旧版 Data2D（类型 45/46）
    auto biomeStateKey = (dimension == 0)
        ? BedrockLevelDb::buildKey(chunkX, chunkZ, BedrockLevelDb::ChunkType::BiomeState)
        : BedrockLevelDb::buildKey(chunkX, chunkZ, dimension, BedrockLevelDb::ChunkType::BiomeState);
    auto biomeStateResult = db.get(biomeStateKey);
    if (biomeStateResult.success() && biomeStateResult.value().has_value()) {
        auto biomeResult = readBiomeState(biomeStateResult.value().value(), *chunk);
        if (biomeResult.failed()) {
            spdlog::debug("BedrockChunkReader: Failed to read biome state for ({}, {})", chunkX, chunkZ);
        }
    } else {
        // 尝试旧版 Data2D
        auto data2DKey = (dimension == 0)
            ? BedrockLevelDb::buildKey(chunkX, chunkZ, BedrockLevelDb::ChunkType::Data2D)
            : BedrockLevelDb::buildKey(chunkX, chunkZ, dimension, BedrockLevelDb::ChunkType::Data2D);
        auto data2DResult = db.get(data2DKey);
        if (data2DResult.success() && data2DResult.value().has_value()) {
            auto biomeResult = readData2D(data2DResult.value().value(), *chunk);
            if (biomeResult.failed()) {
                spdlog::debug("BedrockChunkReader: Failed to read Data2D for ({}, {})", chunkX, chunkZ);
            }
        }
    }

    chunk->setLoaded(true);
    chunk->setFullyGenerated(true);
    chunk->setDirty(false);
    return chunk;
}

Result<void> BedrockChunkReader::readSubChunk(const std::vector<u8>& data, i8 subChunkY, ChunkData& chunk)
{
    if (data.size() < 2) {
        return Error(ErrorCode::ChunkCorrupted, "Sub-chunk data too short");
    }

    size_t pos = 0;

    // 版本字节
    u8 version = data[pos++];

    // 对于版本 9+，有额外的存储层数
    u8 storageCount = 1;
    if (version >= 9) {
        if (pos >= data.size()) {
            return Error(ErrorCode::ChunkCorrupted, "Sub-chunk data truncated at storage count");
        }
        storageCount = data[pos++];
    }

    // 映射到 ChunkSection
    const i32 sectionIndex = static_cast<i32>(subChunkY) - (world::MIN_BUILD_HEIGHT >> 4);
    if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
        return Error(ErrorCode::Unsupported,
            fmt::format(
                "Sub-chunk Y {} maps outside supported section range [0, {})", subChunkY, world::CHUNK_SECTIONS));
    }

    ChunkSection* section = chunk.createSection(sectionIndex);
    if (!section) {
        return Error(ErrorCode::ChunkCorrupted, fmt::format("Failed to create section {} for chunk", sectionIndex));
    }

    // 解析每个存储层
    for (u8 storageIdx = 0; storageIdx < storageCount; ++storageIdx) {
        if (pos >= data.size()) {
            break;
        }

        // 调色板数据头
        u8 paletteHeader = data[pos++];
        i32 bitsPerEntry = paletteHeader >> 1;
        bool isRuntimeEncoding = (paletteHeader & 0x1) != 0;

        // 127 = 空调色板
        if (bitsPerEntry == 127) {
            continue;
        }

        // 计算 word 数量（基岩版使用 32 位 word）
        auto indicesResult = readPackedIndices(data, pos, bitsPerEntry, 4096, 32);
        if (indicesResult.failed()) {
            return indicesResult.error();
        }
        const auto indices = std::move(indicesResult.value());

        // 读取调色板
        auto paletteSizeResult = readVarUint(data, pos);
        if (paletteSizeResult.failed()) {
            return paletteSizeResult.error();
        }
        i32 paletteSize = static_cast<i32>(paletteSizeResult.value());

        if (paletteSize == 0) {
            continue;
        }

        // 解析调色板条目
        std::vector<u32> paletteIds;
        paletteIds.reserve(paletteSize);

        for (i32 i = 0; i < paletteSize; ++i) {
            if (pos >= data.size()) {
                break;
            }

            u32 blockId;
            if (isRuntimeEncoding) {
                auto runtimeIdResult = readVarUint(data, pos);
                if (runtimeIdResult.failed()) {
                    return runtimeIdResult.error();
                }
                blockId = runtimeIdResult.value();
            } else {
                auto blockResult = readPaletteEntry(data, pos, false);
                if (blockResult.failed()) {
                    return blockResult.error();
                }
                blockId = blockResult.value();
            }

            paletteIds.push_back(blockId);
        }

        if (paletteSize == 1 && indices.empty()) {
            std::vector<u32> singleIndices(4096, 0);
            applyBlockPalette(*section, singleIndices, paletteIds, storageIdx != 0);
            continue;
        }

        applyBlockPalette(*section, indices, paletteIds, storageIdx != 0);
    }

    return {};
}

Result<void> BedrockChunkReader::readData2D(const std::vector<u8>& data, ChunkData& chunk)
{
    // Data2D 格式：前 256 字节是高度图（16x16，每字节高度），后 256 字节是生物群系（16x16，每字节 ID）
    if (data.size() < 512) {
        return Error(ErrorCode::ChunkCorrupted, "Data2D data too short");
    }

    BiomeContainer biomeContainer;
    // 旧版生物群系：16x16 = 256 个字节，每个代表一列的生物群系
    // 映射到 4x4x4 = 64 的 BiomeContainer：取每 4x4 区域的第一个值
    for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
        for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
            // 取 4x4 区域中心值
            i32 srcZ = bz * 4 + 2;
            i32 srcX = bx * 4 + 2;
            i32 srcIdx = srcZ * 16 + srcX;
            u8 biomeByte = data[256 + srcIdx];
            BiomeId biomeId = m_biomeMapper.mapBiome(static_cast<i32>(biomeByte));
            // 所有 Y 层使用相同的生物群系
            for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
                biomeContainer.setBiome(bx, by, bz, biomeId);
            }
        }
    }

    chunk.setBiomes(std::move(biomeContainer));
    return {};
}

Result<void> BedrockChunkReader::readBiomeState(const std::vector<u8>& data, ChunkData& chunk)
{
    if (data.empty()) {
        return Error(ErrorCode::ChunkCorrupted, "BiomeState data empty");
    }

    size_t pos = 0;
    std::vector<BiomeSectionData> sections;
    const i32 minSectionY = world::MIN_BUILD_HEIGHT >> 4;
    const i32 maxSectionY = (world::MAX_BUILD_HEIGHT >> 4) - 1;

    for (i32 sectionY = minSectionY; sectionY <= maxSectionY && pos < data.size(); ++sectionY) {
        u8 paletteHeader = data[pos];
        i32 bitsPerEntry = paletteHeader >> 1;
        if (bitsPerEntry == 127) {
            ++pos;
            continue;
        }

        auto sectionResult = readBiomeSectionPalette(data, pos, sectionY, 0);
        if (sectionResult.failed()) {
            return sectionResult.error();
        }
        sections.push_back(std::move(sectionResult.value()));
    }

    applyBiomeSectionsToChunk(sections, chunk);
    return {};
}

Result<std::vector<u32>> BedrockChunkReader::readPackedIndices(
    const std::vector<u8>& data, size_t& pos, i32 bitsPerEntry, i32 entryCount, i32 wordBits) const
{
    if (bitsPerEntry <= 0 || bitsPerEntry > wordBits) {
        return Error(ErrorCode::ChunkCorrupted, fmt::format("Invalid bits per entry: {}", bitsPerEntry));
    }

    const i32 valuesPerWord = wordBits / bitsPerEntry;
    if (valuesPerWord <= 0) {
        return Error(ErrorCode::ChunkCorrupted, "Invalid palette packing parameters");
    }
    const i32 wordCount = (entryCount + valuesPerWord - 1) / valuesPerWord;
    const size_t bytesNeeded = static_cast<size_t>(wordCount) * static_cast<size_t>(wordBits / 8);
    if (pos + bytesNeeded > data.size()) {
        return Error(ErrorCode::ChunkCorrupted, "Packed palette indices truncated");
    }

    std::vector<u32> indices(static_cast<size_t>(entryCount), 0);
    const u32 mask = (bitsPerEntry == 32) ? 0xFFFFFFFFu : ((1u << bitsPerEntry) - 1u);
    i32 index = 0;
    for (i32 word = 0; word < wordCount && index < entryCount; ++word) {
        u32 value = static_cast<u32>(data[pos]) | (static_cast<u32>(data[pos + 1]) << 8) |
            (static_cast<u32>(data[pos + 2]) << 16) | (static_cast<u32>(data[pos + 3]) << 24);
        pos += 4;

        for (i32 bit = 0; bit < valuesPerWord && index < entryCount; ++bit) {
            indices[static_cast<size_t>(index++)] = (value >> (bit * bitsPerEntry)) & mask;
        }
    }

    return indices;
}

Result<u32> BedrockChunkReader::readVarUint(const std::vector<u8>& data, size_t& pos) const
{
    u32 value = 0;
    i32 shift = 0;
    while (pos < data.size() && shift < 35) {
        const u8 byte = data[pos++];
        value |= static_cast<u32>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            return value;
        }
        shift += 7;
    }
    return Error(ErrorCode::ChunkCorrupted, "Invalid Bedrock varuint");
}

Result<BedrockChunkReader::BiomeSectionData> BedrockChunkReader::readBiomeSectionPalette(
    const std::vector<u8>& data, size_t& pos, i32 sectionY, DimensionId dimension) const
{
    if (pos >= data.size()) {
        return Error(ErrorCode::ChunkCorrupted, "Biome section palette truncated");
    }

    const u8 paletteHeader = data[pos++];
    const i32 bitsPerEntry = paletteHeader >> 1;
    const bool runtimeEncoding = (paletteHeader & 0x1) != 0;
    MC_UNUSED(runtimeEncoding);

    auto indicesResult = readPackedIndices(data, pos, bitsPerEntry, BiomeContainer::BIOME_SIZE, 32);
    if (indicesResult.failed()) {
        return indicesResult.error();
    }

    auto paletteSizeResult = readVarUint(data, pos);
    if (paletteSizeResult.failed()) {
        return paletteSizeResult.error();
    }
    const i32 paletteSize = static_cast<i32>(paletteSizeResult.value());
    if (paletteSize <= 0) {
        return Error(ErrorCode::ChunkCorrupted, "Biome section palette is empty");
    }

    std::vector<BiomeId> palette;
    palette.reserve(static_cast<size_t>(paletteSize));
    for (i32 i = 0; i < paletteSize; ++i) {
        auto biomeIdResult = readVarUint(data, pos);
        if (biomeIdResult.failed()) {
            return biomeIdResult.error();
        }
        palette.push_back(m_biomeMapper.mapBiome(static_cast<i32>(biomeIdResult.value()), dimension));
    }

    BiomeSectionData section;
    section.sectionY = sectionY;
    for (i32 i = 0; i < BiomeContainer::BIOME_SIZE; ++i) {
        const u32 paletteIndex = indicesResult.value()[static_cast<size_t>(i)];
        section.biomes[static_cast<size_t>(i)] = paletteIndex < palette.size() ? palette[paletteIndex] : Biomes::Ocean;
    }
    return section;
}

void BedrockChunkReader::applyBiomeSectionsToChunk(
    const std::vector<BiomeSectionData>& sections, ChunkData& chunk) const
{
    if (sections.empty()) {
        return;
    }

    BiomeContainer biomeContainer;
    const i32 baseSectionY = world::MIN_BUILD_HEIGHT >> 4;
    for (const auto& section : sections) {
        if (section.sectionY != baseSectionY) {
            continue;
        }
        for (i32 idx = 0; idx < BiomeContainer::BIOME_SIZE; ++idx) {
            const i32 bx = idx & 0x3;
            const i32 bz = (idx >> 2) & 0x3;
            const i32 by = (idx >> 4) & 0x3;
            biomeContainer.setBiome(bx, by, bz, section.biomes[static_cast<size_t>(idx)]);
        }
        chunk.setBiomes(std::move(biomeContainer));
        return;
    }

    const auto& selected = sections.front();
    for (i32 idx = 0; idx < BiomeContainer::BIOME_SIZE; ++idx) {
        const i32 bx = idx & 0x3;
        const i32 bz = (idx >> 2) & 0x3;
        const i32 by = (idx >> 4) & 0x3;
        biomeContainer.setBiome(bx, by, bz, selected.biomes[static_cast<size_t>(idx)]);
    }
    chunk.setBiomes(std::move(biomeContainer));
}

void BedrockChunkReader::applyBlockPalette(
    ChunkSection& section, const std::vector<u32>& indices, const std::vector<u32>& paletteIds, bool isAuxiliaryLayer)
{
    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                // 基岩版索引顺序：x = (i >> 8) & 0xF, y = i & 0xF, z = (i >> 4) & 0xF
                i32 index = (x << 8) | (z << 4) | y;
                u32 paletteIndex = (index < static_cast<i32>(indices.size())) ? indices[index] : 0;
                u32 stateId = (paletteIndex < paletteIds.size()) ? paletteIds[paletteIndex] : 0;
                if (isAuxiliaryLayer) {
                    if (stateId == 0) {
                        continue;
                    }
                    const BlockState* existing = section.getBlockState(x, y, z);
                    if (!existing || existing->isAir()) {
                        section.setBlockStateId(x, y, z, stateId);
                    }
                    continue;
                }
                section.setBlockStateId(x, y, z, stateId);
            }
        }
    }
}

Result<u32> BedrockChunkReader::readPaletteEntry(const std::vector<u8>& data, size_t& pos, bool isRuntimeEncoding)
{
    if (isRuntimeEncoding) {
        return Error(ErrorCode::Unsupported, "Runtime-encoded palette entry should be decoded by caller");
    }

    if (pos >= data.size()) {
        return Error(ErrorCode::ChunkCorrupted, "Palette entry truncated before NBT payload");
    }

    std::string payload(reinterpret_cast<const char*>(data.data() + pos), data.size() - pos);
    std::istringstream stream(payload);
    stream >> contexts::bedrock_disk;
    auto root = compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to parse Bedrock palette NBT entry");
    }

    const auto consumed = static_cast<size_t>(stream.tellg());
    if (consumed == static_cast<size_t>(-1)) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to determine consumed bytes for palette entry");
    }
    pos += consumed;

    auto nameIt = root->value.find("name");
    if (nameIt == root->value.end()) {
        return Error(ErrorCode::ChunkCorrupted, "Bedrock palette entry missing name");
    }

    const auto* nameTag = dynamic_cast<const string_tag*>(nameIt->second.get());
    if (!nameTag) {
        return Error(ErrorCode::ChunkCorrupted, "Bedrock palette entry name is not a string");
    }

    std::unordered_map<std::string, std::string> states;
    if (const auto* statesTag = tryGetCompound(*root, "states")) {
        for (const auto& [stateName, valueTag] : statesTag->value) {
            switch (valueTag->id()) {
                case TagId::String:
                    states[stateName] = dynamic_cast<const string_tag&>(*valueTag).value;
                    break;
                case TagId::Byte:
                    states[stateName] = dynamic_cast<const byte_tag&>(*valueTag).value != 0 ? "true" : "false";
                    break;
                case TagId::Short:
                    states[stateName] = std::to_string(dynamic_cast<const short_tag&>(*valueTag).value);
                    break;
                case TagId::Int:
                    states[stateName] = std::to_string(dynamic_cast<const int_tag&>(*valueTag).value);
                    break;
                case TagId::Long:
                    states[stateName] = std::to_string(dynamic_cast<const long_tag&>(*valueTag).value);
                    break;
                default:
                    break;
            }
        }
    }

    return mapBlockState(nameTag->value, states);
}

u32 BedrockChunkReader::mapBlockState(
    const std::string& blockName, const std::unordered_map<std::string, std::string>& states)
{
    const std::string cacheKey = buildCacheKey(blockName, states);
    auto it = m_blockStateCache.find(cacheKey);
    if (it != m_blockStateCache.end()) {
        return it->second;
    }

    Block* block = BlockRegistry::instance().getBlock(ResourceLocation(blockName));
    if (!block) {
        spdlog::debug("BedrockChunkReader: Unknown Bedrock block '{}', mapping to air", blockName);
        m_blockStateCache[cacheKey] = 0;
        return 0;
    }

    const BlockState* state = &block->defaultState();
    if (!states.empty()) {
        const auto& container = block->stateContainer();
        for (const auto& [stateName, stateValue] : states) {
            const IProperty* property = container.getProperty(stateName);
            if (!property) {
                continue;
            }

            const auto parsedValue = property->parseValue(stateValue);
            if (!parsedValue.has_value()) {
                continue;
            }

            for (const auto& validState : container.validStates()) {
                if (!validState) {
                    continue;
                }
                if (validState->getValueIndex(*property) == parsedValue) {
                    state = validState.get();
                    break;
                }
            }
        }
    }

    const u32 stateId = state->stateId();
    m_blockStateCache[cacheKey] = stateId;
    return stateId;
}

} // namespace mc::world::storage::reader::bedrock
