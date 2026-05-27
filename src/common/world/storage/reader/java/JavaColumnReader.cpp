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

#include "JavaColumnReader.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/biome/Biome.hpp"
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

const bytearray_tag* getByteArray(const compound_tag& parent, const std::string& name)
{
    auto it = parent.value.find(name);
    if (it == parent.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const bytearray_tag*>(it->second.get());
}

const intarray_tag* getIntArray(const compound_tag& parent, const std::string& name)
{
    auto it = parent.value.find(name);
    if (it == parent.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const intarray_tag*>(it->second.get());
}

const compound_tag& unwrapColumnRoot(const compound_tag& root)
{
    const compound_tag* level = getCompound(root, "Level");
    return level != nullptr ? *level : root;
}

const std::set<std::string> UNFINISHED_STATUSES = {
    "empty",
    "structure_starts",
    "structure_references",
    "biomes",
    "noise",
    "surface",
    "carvers",
    "liquid_carvers",
    "minecraft:empty",
    "minecraft:structure_starts",
    "minecraft:structure_references",
    "minecraft:biomes",
    "minecraft:noise",
    "minecraft:surface",
    "minecraft:carvers",
    "minecraft:liquid_carvers",
};
} // namespace

JavaColumnReader::JavaColumnReader(JavaChunkReader& chunkReader)
    : m_chunkReader(chunkReader)
{}

Result<std::optional<ChunkData>> JavaColumnReader::readColumn(
    const std::vector<u8>& nbtData, ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    std::istringstream stream(std::string(nbtData.begin(), nbtData.end()));
    stream >> contexts::java;
    auto root = compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::ChunkCorrupted, "Failed to parse chunk NBT");
    }

    const compound_tag& columnNbt = unwrapColumnRoot(*root);
    if (columnNbt.value.count("xPos") == 0 || columnNbt.value.count("zPos") == 0) {
        return std::optional<ChunkData>{};
    }

    const i32 xPos = static_cast<i32>(columnNbt.get<int_tag>("xPos"));
    const i32 zPos = static_cast<i32>(columnNbt.get<int_tag>("zPos"));
    if (xPos != x || zPos != z) {
        spdlog::warn(
            "JavaColumnReader: Mislocated chunk, chunk states ({}, {}) but requested ({}, {})", xPos, zPos, x, z);
    }

    if (columnNbt.value.count("Status") != 0) {
        const std::string status = columnNbt.get<string_tag>("Status");
        if (UNFINISHED_STATUSES.contains(status)) {
            return std::optional<ChunkData>{};
        }
    }

    ChunkData chunk(x, z);
    auto biomesResult = readBiomes(columnNbt, chunk);
    if (biomesResult.failed()) {
        return biomesResult.error();
    }
    readHeightmaps(columnNbt, chunk);
    readEntities(columnNbt, chunk);
    readBlockEntities(columnNbt, chunk);
    auto sectionsResult = readSections(columnNbt, chunk);
    if (sectionsResult.failed()) {
        return sectionsResult.error();
    }

    MC_UNUSED(dimension);
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);
    return std::optional<ChunkData>(std::move(chunk));
}

Result<void> JavaColumnReader::readSections(const compound_tag& columnNbt, ChunkData& chunk)
{
    const list_tag* sections = getList(columnNbt, "Sections");
    if (sections == nullptr) {
        sections = getList(columnNbt, "sections");
    }
    if (sections == nullptr) {
        return {};
    }

    for (size_t i = 0; i < sections->size(); ++i) {
        const auto entry = (*sections)[i];
        const auto* sectionNbt = dynamic_cast<const compound_tag*>(entry.get());
        if (sectionNbt == nullptr || sectionNbt->value.count("Y") == 0) {
            continue;
        }

        i32 sectionY = 0;
        const TagId yTagId = sectionNbt->value.at("Y")->id();
        if (yTagId == TagId::Byte) {
            sectionY = static_cast<i8>(sectionNbt->get<byte_tag>("Y"));
        } else if (yTagId == TagId::Int) {
            sectionY = static_cast<i32>(sectionNbt->get<int_tag>("Y"));
        } else {
            continue;
        }

        auto result = m_chunkReader.readSection(*sectionNbt, chunk, sectionY);
        if (result.failed()) {
            spdlog::warn("JavaColumnReader: Failed to read section {} for chunk ({}, {}): {}",
                sectionY,
                chunk.x(),
                chunk.z(),
                result.error().message());
        }
    }

    return {};
}

Result<void> JavaColumnReader::readBiomes(const compound_tag& columnNbt, ChunkData& chunk)
{
    const list_tag* sections = getList(columnNbt, "sections");
    if (sections != nullptr) {
        BiomeContainer biomeContainer;
        const i32 baseSectionY = world::MIN_BUILD_HEIGHT / world::CHUNK_SECTION_HEIGHT;

        for (size_t i = 0; i < sections->size(); ++i) {
            const auto* sectionNbt = dynamic_cast<const compound_tag*>((*sections)[i].get());
            if (sectionNbt == nullptr) {
                continue;
            }

            auto sectionBiomesResult = m_chunkReader.readSectionBiomePalette(*sectionNbt);
            if (sectionBiomesResult.failed()) {
                return sectionBiomesResult.error();
            }
            if (!sectionBiomesResult.value().has_value()) {
                continue;
            }

            const auto& sectionBiomes = *sectionBiomesResult.value();
            const i32 sectionIndex = sectionBiomes.sectionY - baseSectionY;
            if (sectionIndex < 0 || sectionIndex >= BiomeContainer::BIOME_HEIGHT) {
                continue;
            }

            if (sectionBiomes.palette.empty()) {
                continue;
            }

            for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
                for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                    const i32 localIndex = bz * BiomeContainer::BIOME_WIDTH + bx;
                    u32 paletteIndex = 0;
                    if (!sectionBiomes.indices.empty() && localIndex < static_cast<i32>(sectionBiomes.indices.size())) {
                        paletteIndex = sectionBiomes.indices[static_cast<size_t>(localIndex)];
                    }
                    const BiomeId biome = (paletteIndex < sectionBiomes.palette.size())
                        ? sectionBiomes.palette[paletteIndex]
                        : Biomes::Ocean;
                    biomeContainer.setBiome(bx, sectionIndex, bz, biome);
                }
            }
        }

        chunk.setBiomes(std::move(biomeContainer));
        return {};
    }

    const bytearray_tag* biomeBytes = getByteArray(columnNbt, "Biomes");
    if (biomeBytes != nullptr) {
        BiomeContainer biomeContainer;
        for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                const i32 srcZ = bz * 4 + 2;
                const i32 srcX = bx * 4 + 2;
                const i32 srcIdx = srcZ * 16 + srcX;
                const i32 javaBiomeId = (srcIdx < static_cast<i32>(biomeBytes->value.size()))
                    ? static_cast<u8>(biomeBytes->value[static_cast<size_t>(srcIdx)])
                    : static_cast<i32>(Biomes::Ocean);
                const BiomeId biome = m_chunkReader.mapBiomeId(javaBiomeId);
                for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
                    biomeContainer.setBiome(bx, by, bz, biome);
                }
            }
        }
        chunk.setBiomes(std::move(biomeContainer));
        return {};
    }

    const intarray_tag* biomeInts = getIntArray(columnNbt, "Biomes");
    if (biomeInts == nullptr) {
        return {};
    }

    const i32 baseSectionY = world::MIN_BUILD_HEIGHT / world::CHUNK_SECTION_HEIGHT;
    BiomeContainer biomeContainer;
    if (biomeInts->size() == 1024) {
        for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
            const i32 globalY = (baseSectionY * world::CHUNK_SECTION_HEIGHT) + by * 4;
            for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
                for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                    const i32 srcIdx = (globalY / 4) * 16 + bz * 4 + bx;
                    const i32 javaBiomeId =
                        (srcIdx < static_cast<i32>(biomeInts->size())) ? (*biomeInts)[static_cast<size_t>(srcIdx)] : -1;
                    const BiomeId biome = (javaBiomeId >= 0) ? m_chunkReader.mapBiomeId(javaBiomeId) : Biomes::Ocean;
                    biomeContainer.setBiome(bx, by, bz, biome);
                }
            }
        }
        chunk.setBiomes(std::move(biomeContainer));
        return {};
    }

    if (biomeInts->size() == 256) {
        for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                const i32 srcZ = bz * 4 + 2;
                const i32 srcX = bx * 4 + 2;
                const i32 srcIdx = srcZ * 16 + srcX;
                const i32 javaBiomeId =
                    (srcIdx < static_cast<i32>(biomeInts->size())) ? (*biomeInts)[static_cast<size_t>(srcIdx)] : -1;
                const BiomeId biome = (javaBiomeId >= 0) ? m_chunkReader.mapBiomeId(javaBiomeId) : Biomes::Ocean;
                for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
                    biomeContainer.setBiome(bx, by, bz, biome);
                }
            }
        }
        chunk.setBiomes(std::move(biomeContainer));
        return {};
    }

    spdlog::warn("JavaColumnReader: Unsupported Biomes array size {}", biomeInts->size());
    chunk.setBiomes(std::move(biomeContainer));
    return {};
}

void JavaColumnReader::readHeightmaps(const compound_tag& columnNbt, ChunkData& chunk)
{
    m_chunkReader.readHeightmaps(columnNbt, chunk);
}

void JavaColumnReader::readEntities(const compound_tag& columnNbt, ChunkData& chunk)
{
    MC_UNUSED(columnNbt);
    MC_UNUSED(chunk);
}

void JavaColumnReader::readBlockEntities(const compound_tag& columnNbt, ChunkData& chunk)
{
    MC_UNUSED(columnNbt);
    MC_UNUSED(chunk);
}

} // namespace mc::world::storage::reader::java
