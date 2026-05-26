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

#include "JavaChunkReader.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/biome/Biome.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

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

JavaChunkReader::JavaChunkReader(JavaBlockStateMapper& blockMapper, JavaBiomeMapper& biomeMapper)
    : m_blockMapper(blockMapper)
    , m_biomeMapper(biomeMapper)
{}

Result<std::unique_ptr<ChunkData>> JavaChunkReader::readChunk(
    const std::vector<u8>& nbtData, ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    // 解析 NBT（Java 大端序）
    std::istringstream stream(std::string(nbtData.begin(), nbtData.end()));
    stream >> contexts::java;
    auto root = compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to parse chunk NBT");
    }

    // 读取 Level compound
    if (root->value.count("Level") == 0) {
        // 1.18+ 格式：根节点直接是 Level 数据
        // 尝试直接使用 root
        auto chunk = std::make_unique<ChunkData>(x, z);

        auto biomesResult = readBiomes(*root, *chunk);
        if (biomesResult.failed()) {
            return biomesResult.error();
        }

        readHeightmaps(*root, *chunk);

        // 读取 sections
        const auto* sections = getList(*root, "sections");
        if (sections) {
            for (size_t i = 0; i < sections->size(); ++i) {
                auto entryPtr = (*sections)[i];
                auto* sectionNbt = dynamic_cast<const compound_tag*>(entryPtr.get());
                if (!sectionNbt) {
                    continue;
                }
                if (sectionNbt->value.count("Y") == 0) {
                    continue;
                }
                i8 sectionY = static_cast<i8>(sectionNbt->get<byte_tag>("Y"));
                auto result = readSection(*sectionNbt, *chunk, sectionY);
                if (result.failed()) {
                    spdlog::warn("JavaChunkReader: Failed to read section {} for chunk ({}, {}): {}",
                        sectionY,
                        x,
                        z,
                        result.error().message());
                }
            }
        }

        chunk->setLoaded(true);
        chunk->setFullyGenerated(true);
        chunk->setDirty(false);
        return chunk;
    }

    // 1.16.5 格式：root > Level
    const auto* level = getCompound(*root, "Level");
    if (!level) {
        return Error(ErrorCode::ChunkCorrupted, "Chunk NBT Level tag is not a compound");
    }
    auto chunk = std::make_unique<ChunkData>(x, z);

    auto biomesResult = readBiomes(*level, *chunk);
    if (biomesResult.failed()) {
        return biomesResult.error();
    }

    readHeightmaps(*level, *chunk);

    // 读取 Sections（1.16.5 格式）
    const auto* sections = getList(*level, "Sections");
    if (sections) {
        for (size_t i = 0; i < sections->size(); ++i) {
            auto entryPtr = (*sections)[i];
            auto* sectionNbt = dynamic_cast<const compound_tag*>(entryPtr.get());
            if (!sectionNbt) {
                continue;
            }
            if (sectionNbt->value.count("Y") == 0) {
                continue;
            }
            i8 sectionY = static_cast<i8>(sectionNbt->get<byte_tag>("Y"));
            if (sectionY < 0 || sectionY >= world::CHUNK_SECTIONS) {
                continue;
            }
            auto result = readSection(*sectionNbt, *chunk, sectionY);
            if (result.failed()) {
                spdlog::warn("JavaChunkReader: Failed to read section {} for chunk ({}, {}): {}",
                    sectionY,
                    x,
                    z,
                    result.error().message());
            }
        }
    }

    chunk->setLoaded(true);
    chunk->setFullyGenerated(true);
    chunk->setDirty(false);
    return chunk;
}

std::vector<u32> JavaChunkReader::unpackLongArray(const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount)
{
    std::vector<u32> result(entryCount);
    if (bitsPerEntry == 0 || data.empty()) {
        return result;
    }

    u64 mask = (1ULL << bitsPerEntry) - 1;
    i32 valuesPerLong = 64 / bitsPerEntry;

    for (i32 i = 0; i < entryCount; ++i) {
        i32 longIndex = i / valuesPerLong;
        i32 bitOffset = (i % valuesPerLong) * bitsPerEntry;
        if (longIndex >= static_cast<i32>(data.size())) {
            break;
        }
        result[i] = static_cast<u32>((static_cast<u64>(data[longIndex]) >> bitOffset) & mask);
    }
    return result;
}

Result<void> JavaChunkReader::readSection(const compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY)
{
    return readBlockStates(sectionNbt, chunk, sectionY);
}

Result<void> JavaChunkReader::readBlockStates(const compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY)
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
        blockIndices = unpackLongArray(dataArray, bitsPerEntry, 4096);
    } else if (sectionNbt.value.count("BlockStates") != 0) {
        // 1.16.5 格式
        auto& dataArray = sectionNbt.get<longarray_tag>("BlockStates");
        i32 bitsPerEntry =
            std::max(4, static_cast<i32>(std::ceil(std::log2(std::max(static_cast<i32>(paletteIds.size()), 2)))));
        blockIndices = unpackLongArray(dataArray, bitsPerEntry, 4096);
    }

    // 如果只有单个调色板条目且无数据数组，填充整个section
    if (blockIndices.empty()) {
        if (paletteIds.size() == 1) {
            blockIndices.resize(4096, 0);
        } else {
            return {};
        }
    }

    // 创建 ChunkSection 并填充方块状态
    ChunkSection* section = chunk.createSection(sectionY);
    if (!section) {
        return Error(ErrorCode::ChunkCorrupted, fmt::format("Failed to create section {} for chunk", sectionY));
    }

    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                i32 index = y * 256 + z * 16 + x;
                u32 paletteIndex = (index < static_cast<i32>(blockIndices.size())) ? blockIndices[index] : 0;
                u32 stateId = (paletteIndex < paletteIds.size()) ? paletteIds[paletteIndex] : 0;
                section->setBlockStateId(x, y, z, stateId);
            }
        }
    }

    // 读取光照数据
    readLightData(sectionNbt, *section);

    return {};
}

Result<void> JavaChunkReader::readBiomes(const compound_tag& levelNbt, ChunkData& chunk)
{
    if (levelNbt.value.count("Biomes") == 0) {
        return {};
    }

    auto& biomeInts = levelNbt.get<intarray_tag>("Biomes");

    if (biomeInts.size() != 1024) {
        spdlog::debug("JavaChunkReader: Unexpected Biomes array size: {} (expected 1024)", biomeInts.size());
        return {};
    }

    const i32 baseSectionY = world::MIN_BUILD_HEIGHT / world::CHUNK_SECTION_HEIGHT;
    BiomeContainer biomeContainer;
    for (i32 sy = 0; sy < BiomeContainer::BIOME_HEIGHT; ++sy) {
        for (i32 sz = 0; sz < BiomeContainer::BIOME_DEPTH; ++sz) {
            for (i32 sx = 0; sx < BiomeContainer::BIOME_WIDTH; ++sx) {
                const i32 globalY = (baseSectionY * world::CHUNK_SECTION_HEIGHT) + sy * 4;
                const i32 srcIdx = (globalY / 4) * 16 + sz * 4 + sx;
                BiomeId biomeId;
                if (srcIdx < static_cast<i32>(biomeInts.size())) {
                    i32 javaBiomeId = biomeInts[srcIdx];
                    biomeId = (javaBiomeId >= 0) ? m_biomeMapper.mapBiome(javaBiomeId) : Biomes::Ocean;
                } else {
                    biomeId = Biomes::Ocean;
                }
                biomeContainer.setBiome(sx, sy, sz, biomeId);
            }
        }
    }

    chunk.setBiomes(std::move(biomeContainer));
    return {};
}

void JavaChunkReader::readHeightmaps(const compound_tag& levelNbt, ChunkData& chunk)
{
    // 高度图数据目前仅保存到 ChunkData 的元数据中
    // 未来可在 ChunkData 中添加 heightmap 字段
    // 暂时跳过，不阻塞区块加载
}

void JavaChunkReader::readLightData(const compound_tag& sectionNbt, ChunkSection& section)
{
    // 读取 SkyLight（byte[2048]）和 BlockLight（byte[2048]）
    // 这些数据是可选的，1.18+ 可能存储在独立文件中
    // 目前跳过，未来可添加光照支持
}

} // namespace mc::world::storage::reader::java
