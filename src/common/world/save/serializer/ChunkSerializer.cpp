#include "ChunkSerializer.hpp"
#include "../io/CompressionUtil.hpp"
#include "../io/FileUtil.hpp"
#include <sstream>

namespace mc::world::save::serializer {

// ========== ChunkSerializer ==========

std::unique_ptr<nbt::CompoundTag>
ChunkSerializer::serialize(const ChunkData& chunk, i64 gameTime) {
    auto root = std::make_unique<nbt::CompoundTag>();

    // 数据版本
    root->put("DataVersion", DATA_VERSION);

    // Level 标签
    auto level = std::make_unique<nbt::CompoundTag>();

    // 区块位置
    level->put("xPos", static_cast<i32>(chunk.x()));
    level->put("zPos", static_cast<i32>(chunk.z()));

    // 时间
    level->put("LastUpdate", gameTime);
    level->put("InhabitedTime", static_cast<i64>(0));  // TODO: 居住时间

    // 状态
    level->put("Status", chunkStatusToString(chunk.getStatus()));

    // 光照状态
    level->put("isLightOn", true);  // TODO: 光照计算状态

    // 序列化各部分
    serializeSections(*level, chunk);
    serializeBiomes(*level, chunk);
    serializeHeightmaps(*level, chunk);
    serializeBlockEntities(*level, chunk);
    serializeEntities(*level, chunk);
    serializeTicks(*level, chunk);

    // 结构引用（暂时为空）
    auto structures = std::make_unique<nbt::CompoundTag>();
    structures->put("References", std::make_unique<nbt::CompoundTag>());
    structures->put("Starts", std::make_unique<nbt::CompoundTag>());
    level->put("Structures", std::move(structures));

    root->put("Level", std::move(level));
    return root;
}

Result<std::unique_ptr<ChunkData>>
ChunkSerializer::deserialize(const nbt::CompoundTag& nbt) {
    // 获取 Level 标签
    const nbt::CompoundTag* level = nullptr;
    if (nbt.has("Level")) {
        level = nbt.get_if<nbt::CompoundTag>("Level");
    } else {
        // 某些旧版本可能直接在根标签存储
        level = &nbt;
    }

    if (level == nullptr) {
        return Error(ErrorCode::ResourceParseError, "Missing Level tag in chunk data");
    }

    // 读取区块位置
    ChunkCoord x = 0, z = 0;
    if (level->has("xPos")) {
        auto* tag = level->get_if<nbt::IntTag>("xPos");
        if (tag) x = static_cast<ChunkCoord>(tag->get());
    }
    if (level->has("zPos")) {
        auto* tag = level->get_if<nbt::IntTag>("zPos");
        if (tag) z = static_cast<ChunkCoord>(tag->get());
    }

    auto chunk = std::make_unique<ChunkData>(x, z);

    // 读取状态
    if (level->has("Status")) {
        auto* tag = level->get_if<nbt::StringTag>("Status");
        if (tag) {
            chunk->setStatus(parseChunkStatus(tag->get()));
        }
    }

    // 反序列化各部分
    auto sectionsResult = deserializeSections(*level, *chunk);
    if (sectionsResult.failed()) {
        return sectionsResult.error();
    }

    auto biomesResult = deserializeBiomes(*level, *chunk);
    if (biomesResult.failed()) {
        return biomesResult.error();
    }

    auto heightmapsResult = deserializeHeightmaps(*level, *chunk);
    if (heightmapsResult.failed()) {
        // 高度图不是必须的
    }

    auto blockEntitiesResult = deserializeBlockEntities(*level, *chunk);
    if (blockEntitiesResult.failed()) {
        // 方块实体不是必须的
    }

    auto entitiesResult = deserializeEntities(*level, *chunk);
    if (entitiesResult.failed()) {
        // 实体不是必须的
    }

    auto ticksResult = deserializeTicks(*level, *chunk);
    if (ticksResult.failed()) {
        // 计划刻不是必须的
    }

    chunk->setLoaded(true);
    return chunk;
}

std::unique_ptr<nbt::CompoundTag>
ChunkSerializer::serializeSection(const ChunkSection& section, i32 sectionY) {
    auto sectionNbt = std::make_unique<nbt::CompoundTag>();

    // Y 坐标
    sectionNbt->put("Y", static_cast<i8>(sectionY));

    // 方块状态（使用调色板压缩）
    // TODO: 实现调色板压缩
    // 当前使用简化版本：直接存储方块状态 ID 数组

    auto palette = std::make_unique<nbt::ListTag>();
    auto blockStates = std::make_unique<nbt::LongArrayTag>();

    // 构建调色板和方块状态数组
    std::unordered_map<u32, u16> stateToIndex;
    std::vector<u32> paletteEntries;

    for (i32 i = 0; i < ChunkSection::VOLUME; ++i) {
        u32 stateId = section.getBlockStateIdFast(i);
        auto it = stateToIndex.find(stateId);
        if (it == stateToIndex.end()) {
            u16 index = static_cast<u16>(paletteEntries.size());
            stateToIndex[stateId] = index;
            paletteEntries.push_back(stateId);

            // 添加调色板条目
            // TODO: 从 BlockRegistry 获取方块名称和属性
            auto entry = std::make_unique<nbt::CompoundTag>();
            entry->put("Name", "minecraft:air");  // 占位符
            palette->push_back(std::move(entry));
        }
    }

    // 编码方块状态（每 64 位存储多个索引）
    // 简化实现：每个方块使用 16 位
    for (i32 i = 0; i < ChunkSection::VOLUME; ++i) {
        u32 stateId = section.getBlockStateIdFast(i);
        auto it = stateToIndex.find(stateId);
        if (it != stateToIndex.end()) {
            // 简化：直接存储状态 ID
            blockStates->push_back(static_cast<i64>(stateId));
        }
    }

    sectionNbt->put("Palette", std::move(palette));
    sectionNbt->put("BlockStates", std::move(blockStates));

    // 光照数据
    serializeLight(*sectionNbt, section);

    return sectionNbt;
}

Result<std::unique_ptr<ChunkSection>>
ChunkSerializer::deserializeSection(const nbt::CompoundTag& nbt) {
    auto section = std::make_unique<ChunkSection>();

    // 读取 Y 坐标（暂时忽略，由调用者处理）

    // 读取调色板和方块状态
    if (nbt.has("Palette") && nbt.has("BlockStates")) {
        auto* palette = nbt.get_if<nbt::ListTag>("Palette");
        auto* blockStates = nbt.get_if<nbt::LongArrayTag>("BlockStates");

        if (palette && blockStates) {
            // 构建调色板映射
            std::vector<u32> paletteEntries;
            for (size_t i = 0; i < palette->size(); ++i) {
                auto* entry = palette->get_if<nbt::CompoundTag>(i);
                if (entry) {
                    // TODO: 从方块名称解析状态 ID
                    // 当前使用占位符
                    paletteEntries.push_back(0);  // 空气
                }
            }

            // 解码方块状态
            // 简化实现：假设每个长整型存储一个方块
            for (size_t i = 0; i < blockStates->size() && i < ChunkSection::VOLUME; ++i) {
                i64 state = (*blockStates)[i];
                if (state >= 0 && state < static_cast<i64>(paletteEntries.size())) {
                    section->setBlockStateIdFast(static_cast<i32>(i), paletteEntries[state]);
                }
            }
        }
    }

    // 读取光照数据
    auto lightResult = deserializeLight(nbt, *section);
    if (lightResult.failed()) {
        // 光照不是必须的
    }

    return section;
}

// ========== 私有方法 ==========

void ChunkSerializer::serializeSections(nbt::CompoundTag& level, const ChunkData& chunk) {
    auto sectionsList = std::make_unique<nbt::ListTag>();

    for (i32 i = 0; i < ChunkData::SECTIONS; ++i) {
        const ChunkSection* section = chunk.getSection(i);
        if (section != nullptr && !section->isEmpty()) {
            sectionsList->push_back(serializeSection(*section, i));
        }
    }

    level.put("Sections", std::move(sectionsList));
}

Result<void>
ChunkSerializer::deserializeSections(const nbt::CompoundTag& level, ChunkData& chunk) {
    if (!level.has("Sections")) {
        return {};
    }

    auto* sectionsList = level.get_if<nbt::ListTag>("Sections");
    if (sectionsList == nullptr) {
        return {};
    }

    for (size_t i = 0; i < sectionsList->size(); ++i) {
        auto* sectionNbt = sectionsList->get_if<nbt::CompoundTag>(i);
        if (sectionNbt == nullptr) {
            continue;
        }

        // 读取 Y 坐标
        i32 sectionY = 0;
        if (sectionNbt->has("Y")) {
            auto* yTag = sectionNbt->get_if<nbt::ByteTag>("Y");
            if (yTag) {
                sectionY = static_cast<i32>(static_cast<i8>(yTag->get()));
            }
        }

        if (sectionY < 0 || sectionY >= ChunkData::SECTIONS) {
            continue;
        }

        auto sectionResult = deserializeSection(*sectionNbt);
        if (sectionResult.success()) {
            // 创建区块段并复制数据
            ChunkSection* section = chunk.createSection(sectionY);
            if (section != nullptr) {
                // 移动数据
                *section = std::move(*sectionResult.value());
            }
        }
    }

    return {};
}

void ChunkSerializer::serializeBiomes(nbt::CompoundTag& level, const ChunkData& chunk) {
    const auto& biomes = chunk.getBiomes();

    // 生物群系数据（4x4x4 采样）
    auto biomesArray = std::make_unique<nbt::IntArrayTag>();

    for (i32 y = 0; y < 4; ++y) {
        for (i32 z = 0; z < 4; ++z) {
            for (i32 x = 0; x < 4; ++x) {
                BiomeId biome = biomes.getBiome(x * 4, y * 4, z * 4);
                biomesArray->push_back(static_cast<i32>(biome));
            }
        }
    }

    level.put("Biomes", std::move(biomesArray));
}

Result<void>
ChunkSerializer::deserializeBiomes(const nbt::CompoundTag& level, ChunkData& chunk) {
    if (!level.has("Biomes")) {
        return {};
    }

    auto* biomesArray = level.get_if<nbt::IntArrayTag>("Biomes");
    if (biomesArray == nullptr) {
        return {};
    }

    // 创建生物群系容器
    // TODO: 从 NBT 填充生物群系数据

    return {};
}

void ChunkSerializer::serializeHeightmaps(nbt::CompoundTag& level, const ChunkData& chunk) {
    auto heightmapsNbt = std::make_unique<nbt::CompoundTag>();

    // TODO: 序列化各种高度图
    // WORLD_SURFACE, OCEAN_FLOOR, MOTION_BLOCKING, MOTION_BLOCKING_NO_LEAVES

    // 暂时添加空的占位符
    auto emptyHeightmap = std::make_unique<nbt::LongArrayTag>(36);
    heightmapsNbt->put("WORLD_SURFACE", std::move(emptyHeightmap));

    level.put("Heightmaps", std::move(heightmapsNbt));
}

Result<void>
ChunkSerializer::deserializeHeightmaps(const nbt::CompoundTag& level, ChunkData& chunk) {
    if (!level.has("Heightmaps")) {
        return {};
    }

    auto* heightmapsNbt = level.get_if<nbt::CompoundTag>("Heightmaps");
    if (heightmapsNbt == nullptr) {
        return {};
    }

    // TODO: 反序列化高度图

    return {};
}

void ChunkSerializer::serializeLight(nbt::CompoundTag& section, const ChunkSection& chunkSection) {
    // 天空光照
    const auto& skyLight = chunkSection.skyLightNibble();
    auto skyLightArray = std::make_unique<nbt::ByteArrayTag>();
    const u8* skyData = skyLight.data();
    for (size_t i = 0; i < skyLight.size(); ++i) {
        skyLightArray->push_back(static_cast<i8>(skyData[i]));
    }
    section.put("SkyLight", std::move(skyLightArray));

    // 方块光照
    const auto& blockLight = chunkSection.blockLightNibble();
    auto blockLightArray = std::make_unique<nbt::ByteArrayTag>();
    const u8* blockData = blockLight.data();
    for (size_t i = 0; i < blockLight.size(); ++i) {
        blockLightArray->push_back(static_cast<i8>(blockData[i]));
    }
    section.put("BlockLight", std::move(blockLightArray));
}

Result<void>
ChunkSerializer::deserializeLight(const nbt::CompoundTag& section, ChunkSection& chunkSection) {
    // 天空光照
    if (section.has("SkyLight")) {
        auto* skyLightArray = section.get_if<nbt::ByteArrayTag>("SkyLight");
        if (skyLightArray != nullptr && skyLightArray->size() >= 2048) {
            auto& skyLight = chunkSection.skyLightNibble();
            for (size_t i = 0; i < 2048; ++i) {
                skyLight[i] = static_cast<u8>((*skyLightArray)[i]);
            }
        }
    }

    // 方块光照
    if (section.has("BlockLight")) {
        auto* blockLightArray = section.get_if<nbt::ByteArrayTag>("BlockLight");
        if (blockLightArray != nullptr && blockLightArray->size() >= 2048) {
            auto& blockLight = chunkSection.blockLightNibble();
            for (size_t i = 0; i < 2048; ++i) {
                blockLight[i] = static_cast<u8>((*blockLightArray)[i]);
            }
        }
    }

    return {};
}

void ChunkSerializer::serializeBlockEntities(nbt::CompoundTag& level, const ChunkData& chunk) {
    auto blockEntitiesList = std::make_unique<nbt::ListTag>();

    // TODO: 序列化方块实体
    // 当前为空实现

    level.put("TileEntities", std::move(blockEntitiesList));
}

Result<void>
ChunkSerializer::deserializeBlockEntities(const nbt::CompoundTag& level, ChunkData& chunk) {
    if (!level.has("TileEntities")) {
        return {};
    }

    auto* blockEntitiesList = level.get_if<nbt::ListTag>("TileEntities");
    if (blockEntitiesList == nullptr) {
        return {};
    }

    // TODO: 反序列化方块实体

    return {};
}

void ChunkSerializer::serializeEntities(nbt::CompoundTag& level, const ChunkData& chunk) {
    auto entitiesList = std::make_unique<nbt::ListTag>();

    // TODO: 序列化实体
    // 当前为空实现

    level.put("Entities", std::move(entitiesList));
}

Result<void>
ChunkSerializer::deserializeEntities(const nbt::CompoundTag& level, ChunkData& chunk) {
    if (!level.has("Entities")) {
        return {};
    }

    auto* entitiesList = level.get_if<nbt::ListTag>("Entities");
    if (entitiesList == nullptr) {
        return {};
    }

    // TODO: 反序列化实体

    return {};
}

void ChunkSerializer::serializeTicks(nbt::CompoundTag& level, const ChunkData& chunk) {
    auto tileTicksList = std::make_unique<nbt::ListTag>();
    auto liquidTicksList = std::make_unique<nbt::ListTag>();

    // TODO: 序列化计划刻
    // 当前为空实现

    level.put("TileTicks", std::move(tileTicksList));
    level.put("LiquidTicks", std::move(liquidTicksList));
}

Result<void>
ChunkSerializer::deserializeTicks(const nbt::CompoundTag& level, ChunkData& chunk) {
    // 计划刻不是必须的，可以忽略
    return {};
}

const char* ChunkSerializer::chunkStatusToString(ChunkLoadStatus status) {
    switch (status) {
        case ChunkLoadStatus::Empty: return "empty";
        case ChunkLoadStatus::StructureStarts: return "structure_starts";
        case ChunkLoadStatus::StructureReferences: return "structure_references";
        case ChunkLoadStatus::Biomes: return "biomes";
        case ChunkLoadStatus::Noise: return "noise";
        case ChunkLoadStatus::Surface: return "surface";
        case ChunkLoadStatus::Carvers: return "carvers";
        case ChunkLoadStatus::LiquidCarvers: return "liquid_carvers";
        case ChunkLoadStatus::Features: return "features";
        case ChunkLoadStatus::Light: return "light";
        case ChunkLoadStatus::Spawn: return "spawn";
        case ChunkLoadStatus::Heightmaps: return "heightmaps";
        case ChunkLoadStatus::Full: return "full";
        default: return "empty";
    }
}

ChunkLoadStatus ChunkSerializer::parseChunkStatus(const String& status) {
    if (status == "empty") return ChunkLoadStatus::Empty;
    if (status == "structure_starts") return ChunkLoadStatus::StructureStarts;
    if (status == "structure_references") return ChunkLoadStatus::StructureReferences;
    if (status == "biomes") return ChunkLoadStatus::Biomes;
    if (status == "noise") return ChunkLoadStatus::Noise;
    if (status == "surface") return ChunkLoadStatus::Surface;
    if (status == "carvers") return ChunkLoadStatus::Carvers;
    if (status == "liquid_carvers") return ChunkLoadStatus::LiquidCarvers;
    if (status == "features") return ChunkLoadStatus::Features;
    if (status == "light") return ChunkLoadStatus::Light;
    if (status == "spawn") return ChunkLoadStatus::Spawn;
    if (status == "heightmaps") return ChunkLoadStatus::Heightmaps;
    if (status == "full") return ChunkLoadStatus::Full;
    return ChunkLoadStatus::Empty;
}

// ========== LevelDataSerializer ==========

Result<void>
LevelDataSerializer::save(const std::filesystem::path& path,
                           const data::LevelData& levelData,
                           const nbt::CompoundTag* playerNbt) {
    // 序列化到 NBT
    auto nbt = levelData.serialize(playerNbt);

    // 序列化到字节流
    std::ostringstream stream;
    stream << nbt::contexts::java << *nbt;
    std::string nbtStr = stream.str();

    // GZIP 压缩
    auto compressResult = io::CompressionUtil::gzipCompress(
        nbtStr.data(), nbtStr.size()
    );
    if (compressResult.failed()) {
        return compressResult.error();
    }

    // 原子写入文件
    return io::FileUtil::atomicWrite(path,
        compressResult.value().data(),
        compressResult.value().size()
    );
}

Result<std::unique_ptr<data::LevelData>>
LevelDataSerializer::load(const std::filesystem::path& path) {
    // 读取文件
    auto readResult = io::FileUtil::readFile(path);
    if (readResult.failed()) {
        return readResult.error();
    }

    return deserializeFromBytes(readResult.value().data(), readResult.value().size());
}

Result<std::vector<u8>>
LevelDataSerializer::serializeToBytes(const data::LevelData& levelData,
                                       const nbt::CompoundTag* playerNbt) {
    // 序列化到 NBT
    auto nbt = levelData.serialize(playerNbt);

    // 序列化到字节流
    std::ostringstream stream;
    stream << nbt::contexts::java << *nbt;
    std::string nbtStr = stream.str();

    // GZIP 压缩
    return io::CompressionUtil::gzipCompress(nbtStr.data(), nbtStr.size());
}

Result<std::unique_ptr<data::LevelData>>
LevelDataSerializer::deserializeFromBytes(const u8* data, size_t size) {
    // GZIP 解压
    auto decompressResult = io::CompressionUtil::gzipDecompress(data, size);
    if (decompressResult.failed()) {
        return decompressResult.error();
    }

    // 解析 NBT
    std::istringstream stream(std::string(
        reinterpret_cast<const char*>(decompressResult.value().data()),
        decompressResult.value().size()
    ));
    stream >> nbt::contexts::java;

    try {
        auto nbt = nbt::CompoundTag::read(stream);
        if (nbt == nullptr) {
            return Error(ErrorCode::ResourceParseError, "Failed to parse level.dat NBT");
        }

        return data::LevelData::deserialize(*nbt);
    } catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError,
                     std::string("Failed to parse level.dat: ") + e.what());
    }
}

// ========== PlayerDataSerializer ==========

Result<void>
PlayerDataSerializer::save(const std::filesystem::path& path, const data::PlayerData& playerData) {
    // 序列化到字节流
    auto bytesResult = serializeToBytes(playerData);
    if (bytesResult.failed()) {
        return bytesResult.error();
    }

    // 原子写入文件
    return io::FileUtil::atomicWrite(path,
        bytesResult.value().data(),
        bytesResult.value().size()
    );
}

Result<std::unique_ptr<data::PlayerData>>
PlayerDataSerializer::load(const std::filesystem::path& path) {
    // 读取文件
    auto readResult = io::FileUtil::readFile(path);
    if (readResult.failed()) {
        return readResult.error();
    }

    return deserializeFromBytes(readResult.value().data(), readResult.value().size());
}

Result<std::vector<u8>>
PlayerDataSerializer::serializeToBytes(const data::PlayerData& playerData) {
    // 序列化到 NBT
    auto nbt = playerData.serialize();

    // 序列化到字节流
    std::ostringstream stream;
    stream << nbt::contexts::java << *nbt;
    std::string nbtStr = stream.str();

    // GZIP 压缩
    return io::CompressionUtil::gzipCompress(nbtStr.data(), nbtStr.size());
}

Result<std::unique_ptr<data::PlayerData>>
PlayerDataSerializer::deserializeFromBytes(const u8* data, size_t size) {
    // GZIP 解压
    auto decompressResult = io::CompressionUtil::gzipDecompress(data, size);
    if (decompressResult.failed()) {
        return decompressResult.error();
    }

    // 解析 NBT
    std::istringstream stream(std::string(
        reinterpret_cast<const char*>(decompressResult.value().data()),
        decompressResult.value().size()
    ));
    stream >> nbt::contexts::java;

    try {
        auto nbt = nbt::CompoundTag::read(stream);
        if (nbt == nullptr) {
            return Error(ErrorCode::ResourceParseError, "Failed to parse player data NBT");
        }

        return data::PlayerData::deserialize(*nbt);
    } catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError,
                     std::string("Failed to parse player data: ") + e.what());
    }
}

} // namespace mc::world::save::serializer
