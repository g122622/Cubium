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

#include "JavaLevelDatReader.hpp"
#include "common/util/CompressionUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <fstream>
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
} // namespace

Result<std::unique_ptr<compound_tag>> JavaLevelDatReader::_readGzipNbt(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, fmt::format("Cannot open {}", filePath.string()));
    }

    std::vector<u8> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (compressed.empty()) {
        return Error(ErrorCode::FileCorrupted, "level.dat is empty");
    }

    auto decompressed = mc::util::decompressGzip(compressed);
    if (decompressed.empty()) {
        return Error(ErrorCode::DecompressionFailed, "Failed to decompress level.dat");
    }

    std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
    stream >> contexts::java;
    auto root = compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::FileCorrupted, "Failed to parse level.dat NBT");
    }

    return root;
}

Result<LevelSummaryData> JavaLevelDatReader::readSummary(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    auto rootResult = _readGzipNbt(levelDatPath);
    if (rootResult.failed()) {
        return rootResult.error();
    }

    auto root = rootResult.value();
    if (root->value.count("Data") == 0) {
        return Error(ErrorCode::FileCorrupted, "level.dat missing Data tag");
    }

    const auto* data = getCompound(*root, "Data");
    if (!data) {
        return Error(ErrorCode::FileCorrupted, "level.dat Data tag is not a compound");
    }
    return _parseSummary(*data);
}

Result<LevelRuntimeData> JavaLevelDatReader::readRuntimeData(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    auto rootResult = _readGzipNbt(levelDatPath);
    if (rootResult.failed()) {
        return rootResult.error();
    }

    auto root = rootResult.value();
    if (root->value.count("Data") == 0) {
        return Error(ErrorCode::FileCorrupted, "level.dat missing Data tag");
    }

    const auto* data = getCompound(*root, "Data");
    if (!data) {
        return Error(ErrorCode::FileCorrupted, "level.dat Data tag is not a compound");
    }
    return _parseRuntimeData(*data);
}

Result<std::optional<PlayerSaveData>> JavaLevelDatReader::readLocalPlayer(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    auto rootResult = _readGzipNbt(levelDatPath);
    if (rootResult.failed()) {
        return rootResult.error();
    }

    auto root = rootResult.value();
    if (root->value.count("Data") == 0) {
        return Error(ErrorCode::FileCorrupted, "level.dat missing Data tag");
    }

    const auto* data = getCompound(*root, "Data");
    if (!data) {
        return Error(ErrorCode::FileCorrupted, "level.dat Data tag is not a compound");
    }
    return _parseLocalPlayer(*data);
}

Result<LevelSummaryData> JavaLevelDatReader::_parseSummary(const compound_tag& data)
{
    // 读取版本信息
    i32 storageVersion = 0;
    i32 dataVersion = 0;
    std::string versionName = "Java Unknown";
    bool snapshot = false;

    if (data.value.count("DataVersion") != 0) {
        dataVersion = static_cast<i32>(data.get<int_tag>("DataVersion"));
    }

    if (data.value.count("Version") != 0) {
        const auto* version = getCompound(data, "Version");
        if (version) {
            if (version->value.count("Name") != 0) {
                versionName = "Java " + version->get<string_tag>("Name");
            }
            if (version->value.count("Snapshot") != 0) {
                snapshot = static_cast<bool>(version->get<byte_tag>("Snapshot"));
            }
        }
    }

    if (data.value.count("formatVersion") != 0) {
        storageVersion = static_cast<i32>(data.get<int_tag>("formatVersion"));
    }

    // 读取基本字段
    std::string displayName;
    if (data.value.count("LevelName") != 0) {
        displayName = data.get<string_tag>("LevelName");
    }

    i64 lastPlayedMs = 0;
    if (data.value.count("LastPlayed") != 0) {
        lastPlayedMs = data.get<long_tag>("LastPlayed");
    }

    u64 seed = 0;
    if (data.value.count("RandomSeed") != 0) {
        seed = static_cast<u64>(data.get<long_tag>("RandomSeed"));
    } else if (data.value.count("WorldGenSettings") != 0) {
        const auto* worldGen = getCompound(data, "WorldGenSettings");
        if (worldGen && worldGen->value.count("seed") != 0) {
            seed = static_cast<u64>(worldGen->get<long_tag>("seed"));
        }
    }

    GameMode gameMode = _parseGameMode(data);
    Difficulty difficulty = _parseDifficulty(data);
    WorldType worldType = _parseWorldType(data);
    mc::resource::ResourceLocation worldPresetId = _parseWorldPresetId(data);

    bool hardcore = false;
    if (data.value.count("hardcore") != 0) {
        hardcore = static_cast<bool>(data.get<byte_tag>("hardcore"));
    }

    bool allowCommands = false;
    if (data.value.count("allowCommands") != 0) {
        allowCommands = static_cast<bool>(data.get<byte_tag>("allowCommands"));
    }

    return LevelSummaryData(std::move(displayName),
        lastPlayedMs,
        gameMode,
        difficulty,
        hardcore,
        allowCommands,
        seed,
        worldType,
        std::move(worldPresetId),
        LevelVersionInfo(storageVersion, dataVersion, std::move(versionName), snapshot),
        storageVersion,
        dataVersion,
        WorldCompatibility::Current,
        "");
}

Result<LevelRuntimeData> JavaLevelDatReader::_parseRuntimeData(const compound_tag& data)
{
    auto summaryResult = _parseSummary(data);
    if (summaryResult.failed()) {
        return summaryResult.error();
    }

    auto summary = summaryResult.value();

    i32 spawnX = 0, spawnY = 64, spawnZ = 0;
    if (data.value.count("SpawnX") != 0) {
        spawnX = static_cast<i32>(data.get<int_tag>("SpawnX"));
    }
    if (data.value.count("SpawnY") != 0) {
        spawnY = static_cast<i32>(data.get<int_tag>("SpawnY"));
    }
    if (data.value.count("SpawnZ") != 0) {
        spawnZ = static_cast<i32>(data.get<int_tag>("SpawnZ"));
    }

    f32 spawnAngle = 0.0f;
    if (data.value.count("SpawnAngle") != 0) {
        spawnAngle = data.get<float_tag>("SpawnAngle");
    }

    i64 gameTime = 0;
    if (data.value.count("Time") != 0) {
        gameTime = data.get<long_tag>("Time");
    }

    i64 dayTime = 0;
    if (data.value.count("DayTime") != 0) {
        dayTime = data.get<long_tag>("DayTime");
    }

    i32 clearWeatherTime = 0;
    if (data.value.count("clearWeatherTime") != 0) {
        clearWeatherTime = static_cast<i32>(data.get<int_tag>("clearWeatherTime"));
    }

    i32 rainTime = 0;
    if (data.value.count("rainTime") != 0) {
        rainTime = static_cast<i32>(data.get<int_tag>("rainTime"));
    }

    bool raining = false;
    if (data.value.count("raining") != 0) {
        raining = static_cast<bool>(data.get<byte_tag>("raining"));
    }

    i32 thunderTime = 0;
    if (data.value.count("thunderTime") != 0) {
        thunderTime = static_cast<i32>(data.get<int_tag>("thunderTime"));
    }

    bool thundering = false;
    if (data.value.count("thundering") != 0) {
        thundering = static_cast<bool>(data.get<byte_tag>("thundering"));
    }

    bool initialized = false;
    if (data.value.count("initialized") != 0) {
        initialized = static_cast<bool>(data.get<byte_tag>("initialized"));
    }

    bool difficultyLocked = false;
    if (data.value.count("DifficultyLocked") != 0) {
        difficultyLocked = static_cast<bool>(data.get<byte_tag>("DifficultyLocked"));
    }

    return LevelRuntimeData(std::move(summary),
        spawnX,
        spawnY,
        spawnZ,
        spawnAngle,
        gameTime,
        dayTime,
        clearWeatherTime,
        rainTime,
        raining,
        thunderTime,
        thundering,
        initialized,
        difficultyLocked);
}

Result<std::optional<PlayerSaveData>> JavaLevelDatReader::_parseLocalPlayer(const compound_tag& data)
{
    const auto* player = getCompound(data, "Player");
    if (!player) {
        return std::optional<PlayerSaveData>{};
    }

    auto playerResult = PlayerSaveData::fromNbt(*player);
    if (playerResult.failed()) {
        return playerResult.error();
    }

    auto playerData = playerResult.value();
    if (playerData.uuid.empty()) {
        playerData.uuid = "~local_player";
    }
    if (playerData.username.empty()) {
        playerData.username = "~local_player";
    }

    return std::optional<PlayerSaveData>(std::move(playerData));
}

WorldType JavaLevelDatReader::_parseWorldType(const compound_tag& data)
{
    std::string generatorName;
    if (data.value.count("generatorName") != 0) {
        generatorName = data.get<string_tag>("generatorName");
    }

    if (generatorName == "flat") {
        return WorldType::Flat;
    }
    if (generatorName == "largeBiomes") {
        return WorldType::LargeBiomes;
    }
    if (generatorName == "amplified") {
        return WorldType::Amplified;
    }
    if (generatorName == "debug_all_block_states") {
        return WorldType::Debug;
    }
    return WorldType::Default;
}

mc::resource::ResourceLocation JavaLevelDatReader::_parseWorldPresetId(const compound_tag& data)
{
    // 读项目私有 Data.Reborn.WorldPresetId（worldPresetId 无原版对应，仅写 Reborn compound）。
    auto rebornIt = data.value.find("Reborn");
    if (rebornIt != data.value.end() && rebornIt->second->id() == mc::nbt::TagId::Compound) {
        auto& reborn = dynamic_cast<const compound_tag&>(*rebornIt->second);
        auto it = reborn.value.find("WorldPresetId");
        if (it != reborn.value.end() && it->second->id() == mc::nbt::TagId::String) {
            const auto& str = dynamic_cast<const string_tag&>(*it->second).value;
            if (!str.empty()) {
                return mc::resource::ResourceLocation::parse(str);
            }
        }
    }
    return mc::resource::ResourceLocation("minecraft", "normal");
}

GameMode JavaLevelDatReader::_parseGameMode(const compound_tag& data)
{
    if (data.value.count("GameType") != 0) {
        i32 gameType = static_cast<i32>(data.get<int_tag>("GameType"));
        switch (gameType) {
            case 0:
                return GameMode::Survival;
            case 1:
                return GameMode::Creative;
            case 2:
                return GameMode::Adventure;
            case 3:
                return GameMode::Spectator;
            default:
                return GameMode::Survival;
        }
    }
    return GameMode::Survival;
}

Difficulty JavaLevelDatReader::_parseDifficulty(const compound_tag& data)
{
    if (data.value.count("Difficulty") != 0) {
        i32 diff = static_cast<i32>(data.get<byte_tag>("Difficulty"));
        switch (diff) {
            case 0:
                return Difficulty::Peaceful;
            case 1:
                return Difficulty::Easy;
            case 2:
                return Difficulty::Normal;
            case 3:
                return Difficulty::Hard;
            default:
                return Difficulty::Normal;
        }
    }
    return Difficulty::Normal;
}

} // namespace mc::world::storage::reader::java
