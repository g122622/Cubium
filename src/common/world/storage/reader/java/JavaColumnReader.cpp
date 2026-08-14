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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/storage/reader/java/JavaChunkReader.hpp"
#include <array>
#include <cstddef>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
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

std::optional<BlockPos> readBlockEntityPos(const compound_tag& tag)
{
    auto xIt = tag.value.find("x");
    auto yIt = tag.value.find("y");
    auto zIt = tag.value.find("z");
    if (xIt == tag.value.end() || yIt == tag.value.end() || zIt == tag.value.end()) {
        return std::nullopt;
    }
    if (xIt->second->id() != TagId::Int || yIt->second->id() != TagId::Int || zIt->second->id() != TagId::Int) {
        return std::nullopt;
    }

    return BlockPos(static_cast<i32>(tag.get<int_tag>("x")),
        static_cast<i32>(tag.get<int_tag>("y")),
        static_cast<i32>(tag.get<int_tag>("z")));
}

void applyHeightmapArray(ChunkData& chunk, HeightmapType type, const longarray_tag& packedHeights, i32 heightOffset)
{
    // Java 版高度图使用 9 位编码每个高度值（支持 -512 到 2047 的范围）
    // Java 存储值语义：Y+1-minY（相对维度最低 Y），0 表示无方块。
    // 转换为内部存储语义：encoded != 0 时为 Y+1（绝对，= encoded + heightOffset），
    //                       encoded == 0 时为 NO_BLOCK_SENTINEL（无方块）。
    constexpr i32 BITS_PER_ENTRY = 9;
    constexpr i32 ENTRY_COUNT = world::CHUNK_WIDTH * world::CHUNK_WIDTH;

    auto unpacked = JavaChunkReader::unpackLongArray(packedHeights.value, BITS_PER_ENTRY, ENTRY_COUNT, true);
    std::array<BlockCoord, Heightmap::SIZE> heights{};
    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            const i32 index = z * world::CHUNK_WIDTH + x;
            const i32 encoded = (index < static_cast<i32>(unpacked.size()))
                ? static_cast<i32>(unpacked[static_cast<size_t>(index)])
                : 0;
            heights[static_cast<size_t>(index)] =
                encoded != 0 ? static_cast<BlockCoord>(encoded + heightOffset) : Heightmap::NO_BLOCK_SENTINEL;
        }
    }

    // 直接整列写回 Heightmap 并标记槽位已初始化。
    // 旧实现调用 chunk.updateHeightmap(type, x, heights[i] - 1, z, nullptr) 试图逐列更新，
    // 但 updateHeightmap 内部调用 Heightmap::update，后者依赖 _isOpaque(state) 判定，
    // state 为 nullptr 时 _isOpaque 返回 false，导致所有列的写入被跳过，
    // 持久化的高度值从未真正进入 m_heightmaps，仅 m_heightmapInitialized 被置 true。
    // 此处使用专门的 setHeightmapFromStorage 绕过 _isOpaque 判定。
    chunk.setHeightmapFromStorage(type, heights);
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

    // 获取维度类型，用于维度特定的区块处理
    const DimensionType dimType = DimensionType::fromId(dimension);
    const i32 dimMinHeight = dimType.minHeight();
    const i32 dimMaxHeight = dimType.maxHeight();
    const bool dimHasSkyLight = dimType.hasSkyLight();

    ChunkData chunk(x, z);
    auto biomesResult = _readBiomes(columnNbt, chunk, dimMinHeight);
    if (biomesResult.failed()) {
        return biomesResult.error();
    }
    _readHeightmaps(columnNbt, chunk, dimMinHeight);
    _readEntities(columnNbt, chunk);
    _readBlockEntities(columnNbt, chunk);
    auto sectionsResult = _readSections(columnNbt, chunk, dimMinHeight, dimMaxHeight, dimHasSkyLight);
    if (sectionsResult.failed()) {
        return sectionsResult.error();
    }

    // 读取居住时间（InhabitedTime）
    if (columnNbt.value.count("InhabitedTime") != 0) {
        const auto& tag = columnNbt.value.at("InhabitedTime");
        if (tag->id() == nbt::TagId::Long) {
            chunk.setInhabitedTime(static_cast<i64>(columnNbt.get<long_tag>("InhabitedTime")));
        }
    }

    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);
    return std::optional<ChunkData>(std::move(chunk));
}

Result<void> JavaColumnReader::_readSections(
    const compound_tag& columnNbt, ChunkData& chunk, i32 dimMinHeight, i32 dimMaxHeight, bool dimHasSkyLight)
{
    const list_tag* sections = getList(columnNbt, "Sections");
    if (sections == nullptr) {
        sections = getList(columnNbt, "sections");
    }
    if (sections == nullptr) {
        return {};
    }

    const i32 minSectionY = dimMinHeight / world::CHUNK_SECTION_HEIGHT;
    const i32 maxSectionY = (dimMaxHeight - 1) / world::CHUNK_SECTION_HEIGHT;

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

        // 跳过超出维度合法范围的 section（对应 MC Java SerializableChunkData.parse 中的 section Y 校验）
        if (sectionY < minSectionY || sectionY > maxSectionY) {
            continue;
        }

        auto result = m_chunkReader.readSection(*sectionNbt, chunk, sectionY, dimHasSkyLight);
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

Result<void> JavaColumnReader::_readBiomes(const compound_tag& columnNbt, ChunkData& chunk, i32 dimMinHeight)
{
    // 生物群系采样参数：Java 版每 4x4x4 方块区域共享一个生物群系
    constexpr i32 BIOME_SAMPLE_STRIDE = 4;
    constexpr i32 BIOME_SAMPLE_OFFSET = 2;

    const list_tag* sections = getList(columnNbt, "sections");
    if (sections != nullptr) {
        BiomeContainer biomeContainer;

        for (size_t i = 0; i < sections->size(); ++i) {
            auto entry = (*sections)[i];
            const auto* sectionNbt = dynamic_cast<const compound_tag*>(entry.get());
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
            const i32 sectionIndex = world::sectionCoordToIndex(sectionBiomes.sectionY);
            if (sectionIndex < 0 || sectionIndex >= BiomeContainer::SECTION_COUNT) {
                continue;
            }

            if (sectionBiomes.palette.empty()) {
                continue;
            }

            for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
                for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
                    const i32 localIndex = bz * BiomeContainer::HORIZ_SIZE + bx;
                    u32 paletteIndex = 0;
                    if (!sectionBiomes.indices.empty() && localIndex < static_cast<i32>(sectionBiomes.indices.size())) {
                        paletteIndex = sectionBiomes.indices[static_cast<size_t>(localIndex)];
                    }
                    const BiomeId biome = (paletteIndex < sectionBiomes.palette.size())
                        ? sectionBiomes.palette[paletteIndex]
                        : Biomes::Ocean;
                    for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
                        biomeContainer.setBiome(sectionIndex, bx, by, bz, biome);
                    }
                }
            }
        }

        chunk.setBiomes(std::move(biomeContainer));
        return {};
    }

    const bytearray_tag* biomeBytes = getByteArray(columnNbt, "Biomes");
    if (biomeBytes != nullptr) {
        BiomeContainer biomeContainer;
        for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
                const i32 srcZ = bz * BIOME_SAMPLE_STRIDE + BIOME_SAMPLE_OFFSET;
                const i32 srcX = bx * BIOME_SAMPLE_STRIDE + BIOME_SAMPLE_OFFSET;
                const i32 srcIdx = srcZ * world::CHUNK_WIDTH + srcX;
                const i32 javaBiomeId = (srcIdx < static_cast<i32>(biomeBytes->value.size()))
                    ? static_cast<u8>(biomeBytes->value[static_cast<size_t>(srcIdx)])
                    : static_cast<i32>(Biomes::Ocean);
                const BiomeId biome = m_chunkReader.mapBiomeId(javaBiomeId);
                for (i32 sectionIndex = 0; sectionIndex < BiomeContainer::SECTION_COUNT; ++sectionIndex) {
                    for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
                        biomeContainer.setBiome(sectionIndex, bx, by, bz, biome);
                    }
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

    const i32 baseSectionY = dimMinHeight / world::CHUNK_SECTION_HEIGHT;
    BiomeContainer biomeContainer;
    // 1024 = 4x4x4 生物群系采样 * 16 个区块段（旧版 3D 生物群系格式，仅覆盖下半部分）
    constexpr i32 JAVA_BIOME_3D_ARRAY_SIZE = 1024;
    if (biomeInts->value.size() == JAVA_BIOME_3D_ARRAY_SIZE) {
        for (i32 sectionIndex = 0; sectionIndex < 16; ++sectionIndex) {
            for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
                for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
                    for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
                        const i32 globalY = (baseSectionY * world::CHUNK_SECTION_HEIGHT) +
                            (sectionIndex * world::CHUNK_SECTION_HEIGHT) + by * BIOME_SAMPLE_STRIDE;
                        const i32 srcIdx =
                            (globalY / BIOME_SAMPLE_STRIDE) * world::CHUNK_WIDTH + bz * BIOME_SAMPLE_STRIDE + bx;
                        const i32 javaBiomeId = (srcIdx < static_cast<i32>(biomeInts->value.size()))
                            ? biomeInts->value[static_cast<size_t>(srcIdx)]
                            : -1;
                        const BiomeId biome =
                            (javaBiomeId >= 0) ? m_chunkReader.mapBiomeId(javaBiomeId) : Biomes::Ocean;
                        biomeContainer.setBiome(sectionIndex, bx, by, bz, biome);
                    }
                }
            }
        }
        chunk.setBiomes(std::move(biomeContainer));
        return {};
    }

    // 256 = 16x16 2D 生物群系格式（旧版 Java）
    constexpr i32 JAVA_BIOME_2D_ARRAY_SIZE = 256;
    if (biomeInts->value.size() == JAVA_BIOME_2D_ARRAY_SIZE) {
        for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
                const i32 srcZ = bz * BIOME_SAMPLE_STRIDE + BIOME_SAMPLE_OFFSET;
                const i32 srcX = bx * BIOME_SAMPLE_STRIDE + BIOME_SAMPLE_OFFSET;
                const i32 srcIdx = srcZ * world::CHUNK_WIDTH + srcX;
                const i32 javaBiomeId = (srcIdx < static_cast<i32>(biomeInts->value.size()))
                    ? biomeInts->value[static_cast<size_t>(srcIdx)]
                    : -1;
                const BiomeId biome = (javaBiomeId >= 0) ? m_chunkReader.mapBiomeId(javaBiomeId) : Biomes::Ocean;
                for (i32 sectionIndex = 0; sectionIndex < BiomeContainer::SECTION_COUNT; ++sectionIndex) {
                    for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
                        biomeContainer.setBiome(sectionIndex, bx, by, bz, biome);
                    }
                }
            }
        }
        chunk.setBiomes(std::move(biomeContainer));
        return {};
    }

    spdlog::warn("JavaColumnReader: Unsupported Biomes array size {}", biomeInts->value.size());
    chunk.setBiomes(std::move(biomeContainer));
    return {};
}

void JavaColumnReader::_readHeightmaps(const compound_tag& columnNbt, ChunkData& chunk, i32 heightOffset)
{
    const compound_tag* heightmaps = getCompound(columnNbt, "Heightmaps");
    if (heightmaps != nullptr) {
        for (const auto& [name, value] : heightmaps->value) {
            if (value->id() != TagId::LongArray) {
                continue;
            }

            HeightmapType type;
            if (name == "WORLD_SURFACE" || name == "WORLD_SURFACE_WG") {
                type = (name == "WORLD_SURFACE") ? HeightmapType::WorldSurface : HeightmapType::WorldSurfaceWG;
            } else if (name == "OCEAN_FLOOR" || name == "OCEAN_FLOOR_WG") {
                type = (name == "OCEAN_FLOOR") ? HeightmapType::OceanFloor : HeightmapType::OceanFloorWG;
            } else if (name == "MOTION_BLOCKING") {
                type = HeightmapType::MotionBlocking;
            } else if (name == "MOTION_BLOCKING_NO_LEAVES") {
                type = HeightmapType::MotionBlockingNoLeaves;
            } else if (name == "LIGHT_BLOCKING") {
                type = HeightmapType::LightBlocking;
            } else {
                continue;
            }

            applyHeightmapArray(chunk, type, dynamic_cast<const longarray_tag&>(*value), heightOffset);
        }
        return;
    }

    auto legacyHeightmap = getIntArray(columnNbt, "HeightMap");
    if (legacyHeightmap == nullptr || legacyHeightmap->value.size() != world::CHUNK_WIDTH * world::CHUNK_WIDTH) {
        return;
    }

    // 旧版 HeightMap int 数组语义：每列最高方块 Y+1（绝对世界坐标），0 表示无方块。
    // 直接整列写回 Heightmap，避免 updateHeightmap + nullptr state 的 no-op 问题。
    // 0 转换为 NO_BLOCK_SENTINEL 以匹配内部存储语义。
    std::array<BlockCoord, Heightmap::SIZE> heights{};
    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            const i32 index = z * world::CHUNK_WIDTH + x;
            const i32 legacy = legacyHeightmap->value[static_cast<size_t>(index)];
            heights[static_cast<size_t>(index)] =
                legacy != 0 ? static_cast<BlockCoord>(legacy) : Heightmap::NO_BLOCK_SENTINEL;
        }
    }
    chunk.setHeightmapFromStorage(HeightmapType::WorldSurface, heights);
    chunk.setHeightmapFromStorage(HeightmapType::WorldSurfaceWG, heights);
}

void JavaColumnReader::_readEntities(const compound_tag& columnNbt, ChunkData& chunk)
{
    const list_tag* entities = getList(columnNbt, "Entities");
    if (entities == nullptr) {
        return;
    }

    for (size_t i = 0; i < entities->size(); ++i) {
        const auto& entityPtr = (*entities)[i];
        const auto* entityTag = dynamic_cast<const compound_tag*>(entityPtr.get());
        if (entityTag == nullptr) {
            continue;
        }

        // 仅以原始 NBT 形式暂存到 ChunkData，不在此反序列化——反序列化需要所在维度的
        // ecs::EntityRegistry（Entity 构造时即 attach 高频组件，entt 实体不可跨 registry
        // 迁移），而 storage 层不持有 world。推迟到 ServerWorld::onChunkLoaded（持有
        // *entityRegistry()）spawn 点再反序列化。这里 deep copy NBT（list_tag 内元素为
        // const unique_ptr<tag>，不可直接 move 出来）。
        auto copied = entityTag->copy();
        chunk.addLoadedEntityNbt(std::unique_ptr<compound_tag>(dynamic_cast<compound_tag*>(copied.release())));
    }
}

void JavaColumnReader::_readBlockEntities(const compound_tag& columnNbt, ChunkData& chunk)
{
    const list_tag* blockEntities = getList(columnNbt, "block_entities");
    if (blockEntities == nullptr) {
        blockEntities = getList(columnNbt, "TileEntities");
    }
    if (blockEntities == nullptr) {
        return;
    }

    auto& registry = blockentity::BlockEntityRegistry::instance();
    registry.registerBuiltinTypes();

    for (size_t i = 0; i < blockEntities->size(); ++i) {
        auto entry = (*blockEntities)[i];
        const auto* tag = dynamic_cast<const compound_tag*>(entry.get());
        if (tag == nullptr) {
            continue;
        }

        auto idIt = tag->value.find("id");
        if (idIt == tag->value.end() || idIt->second->id() != TagId::String) {
            spdlog::warn("JavaColumnReader: Block entity entry missing string id");
            continue;
        }

        auto pos = readBlockEntityPos(*tag);
        if (!pos.has_value()) {
            spdlog::warn("JavaColumnReader: Block entity {} missing valid position", tag->get<string_tag>("id"));
            continue;
        }

        const ResourceLocation blockEntityId(tag->get<string_tag>("id"));
        const BlockEntityType type = blockEntityTypeFromId(blockEntityId);
        if (type == BlockEntityType::Unknown) {
            spdlog::warn("JavaColumnReader: Unsupported block entity type {}", blockEntityId.toString());
            continue;
        }

        auto entity = registry.create(type, *pos);
        if (entity == nullptr) {
            spdlog::warn("JavaColumnReader: Failed to create block entity {}", blockEntityId.toString());
            continue;
        }

        if (!entity->loadFromNBT(*tag)) {
            spdlog::warn("JavaColumnReader: Failed to load NBT for block entity {}", blockEntityId.toString());
            continue;
        }

        chunk.setBlockEntity(*pos, std::move(entity));
    }
}

} // namespace mc::world::storage::reader::java
