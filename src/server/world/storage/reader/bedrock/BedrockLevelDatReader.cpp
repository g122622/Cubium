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

#include "BedrockLevelDatReader.hpp"
#include "BedrockConstants.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/spawn/RespawnData.hpp"
#include "server/world/storage/core/LevelDatCodec.hpp"
#include "server/world/storage/list/WorldListEntry.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <fmt/format.h>

namespace mc::world::storage::reader::bedrock {

using namespace mc::nbt;
using namespace mc::nbt::tags;

Result<std::unique_ptr<compound_tag>> BedrockLevelDatReader::_readBedrockNbt(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, fmt::format("Cannot open {}", filePath.string()));
    }

    // 基岩版 level.dat 有 BEDROCK_LEVEL_DAT_HEADER_SIZE 字节文件头
    u8 header[BEDROCK_LEVEL_DAT_HEADER_SIZE] = {};
    if (!file.read(reinterpret_cast<char*>(header), BEDROCK_LEVEL_DAT_HEADER_SIZE)) {
        return Error(ErrorCode::FileCorrupted, "Failed to read Bedrock level.dat header");
    }

    // 跳过文件头后读取 NBT
    std::vector<u8> nbtData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (nbtData.empty()) {
        return Error(ErrorCode::FileCorrupted, "Bedrock level.dat has no NBT data");
    }

    std::istringstream stream(std::string(nbtData.begin(), nbtData.end()));
    stream >> contexts::bedrock_disk;
    auto rootResult = compound_tag::read(stream);
    if (rootResult.failed()) {
        return Error(
            ErrorCode::FileCorrupted, "Failed to parse Bedrock level.dat NBT: " + rootResult.error().message());
    }
    auto root = std::move(rootResult).value();

    return root;
}

Result<LevelSummaryData> BedrockLevelDatReader::readSummary(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    auto rootResult = _readBedrockNbt(levelDatPath);
    if (rootResult.failed()) {
        return rootResult.error();
    }

    return _parseSummary(*rootResult.value());
}

Result<LevelRuntimeData> BedrockLevelDatReader::readRuntimeData(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    auto rootResult = _readBedrockNbt(levelDatPath);
    if (rootResult.failed()) {
        return rootResult.error();
    }

    return _parseRuntimeData(*rootResult.value());
}

Result<LevelSummaryData> BedrockLevelDatReader::_parseSummary(const compound_tag& root)
{
    // 基岩版 level.dat 字段
    std::string displayName;
    if (const auto* tag = root.findAs<string_tag>("LevelName")) {
        displayName = tag->value;
    }

    i64 lastPlayedMs = 0;
    if (const auto* tag = root.findAs<long_tag>("LastPlayed")) {
        lastPlayedMs = tag->value;
    }

    u64 seed = 0;
    if (const auto* tag = root.findAs<long_tag>("RandomSeed")) {
        seed = static_cast<u64>(tag->value);
    }

    i32 gameType = 0;
    if (const auto* tag = root.findAs<int_tag>("GameType")) {
        gameType = static_cast<i32>(tag->value);
    }

    i32 difficulty = 2; // 默认 Normal
    if (const auto* tag = root.findAs<int_tag>("Difficulty")) {
        difficulty = static_cast<i32>(tag->value);
    }

    bool hardcore = false;
    if (const auto* tag = root.findAs<byte_tag>("hardcore")) {
        hardcore = static_cast<bool>(tag->value);
    }

    bool allowCommands = false;
    if (const auto* tag = root.findAs<byte_tag>("commandsEnabled")) {
        allowCommands = static_cast<bool>(tag->value);
    }

    std::string generatorName;
    if (const auto* tag = root.findAs<string_tag>("Generator")) {
        generatorName = tag->value;
    }

    // 版本信息
    i32 dataVersion = 0;
    std::string versionName = "Bedrock";
    if (root.value.count("lastOpenedWithVersion") != 0) {
        auto* versionList = dynamic_cast<int_list_tag*>(root.value.at("lastOpenedWithVersion").get());
        if (versionList && versionList->size() >= 2) {
            const auto& values = versionList->value;
            i32 major = static_cast<i32>(values[0]);
            i32 minor = static_cast<i32>(values[1]);
            i32 patch = versionList->size() >= 3 ? static_cast<i32>(values[2]) : 0;
            dataVersion = major;
            versionName = fmt::format("Bedrock {}.{}.{}", major, minor, patch);
        }
    }

    return LevelSummaryData(std::move(displayName),
        lastPlayedMs,
        _parseGameMode(gameType),
        _parseDifficulty(difficulty),
        hardcore,
        allowCommands,
        seed,
        _parseWorldType(generatorName),
        _parseWorldPresetId(generatorName),
        LevelVersionInfo(0, dataVersion, std::move(versionName), false),
        0,
        dataVersion,
        WorldCompatibility::Current,
        "");
}

Result<LevelRuntimeData> BedrockLevelDatReader::_parseRuntimeData(const compound_tag& root)
{
    auto summaryResult = _parseSummary(root);
    if (summaryResult.failed()) {
        return summaryResult.error();
    }

    auto& summary = summaryResult.value();

    // 基岩版 level.dat 用 SpawnX/SpawnY/SpawnZ 记录主世界出生方块。
    i32 spawnX = 0, spawnY = 64, spawnZ = 0;
    if (const auto* tag = root.findAs<int_tag>("SpawnX")) {
        spawnX = static_cast<i32>(tag->value);
    }
    if (const auto* tag = root.findAs<int_tag>("SpawnY")) {
        spawnY = static_cast<i32>(tag->value);
    }
    if (const auto* tag = root.findAs<int_tag>("SpawnZ")) {
        spawnZ = static_cast<i32>(tag->value);
    }
    const RespawnData spawnData =
        RespawnData::of(DimensionManager::OVERWORLD, BlockPos(spawnX, spawnY, spawnZ), 0.0f, 0.0f);

    i64 gameTime = 0;
    if (const auto* tag = root.findAs<long_tag>("Time")) {
        gameTime = tag->value;
    }

    i64 dayTime = 0;
    if (const auto* tag = root.findAs<long_tag>("DayTime")) {
        dayTime = tag->value;
    }

    i32 rainTime = 0;
    if (const auto* tag = root.findAs<int_tag>("rainTime")) {
        rainTime = static_cast<i32>(tag->value);
    }

    bool raining = false;
    if (const auto* tag = root.findAs<byte_tag>("raining")) {
        raining = static_cast<bool>(tag->value);
    }

    i32 thunderTime = 0;
    if (const auto* tag = root.findAs<int_tag>("lightningTime")) {
        thunderTime = static_cast<i32>(tag->value);
    }

    bool thundering = false;
    if (const auto* levelTag = root.findAs<float_tag>("lightningLevel")) {
        // 基岩版用 lightningLevel > 0 判断是否雷暴
        thundering = levelTag->value > 0.0f;
    }

    bool initialized = true;
    if (const auto* tag = root.findAs<byte_tag>("hasBeenLoadedInCreative")) {
        initialized = static_cast<bool>(tag->value);
    }

    bool difficultyLocked = false;
    if (const auto* tag = root.findAs<byte_tag>("DifficultyLocked")) {
        difficultyLocked = static_cast<bool>(tag->value);
    }

    return LevelRuntimeData(std::move(summary),
        spawnData,
        gameTime,
        dayTime,
        0, // clearWeatherTime - 基岩版无此字段
        rainTime,
        raining,
        thunderTime,
        thundering,
        initialized,
        difficultyLocked);
}

GameMode BedrockLevelDatReader::_parseGameMode(i32 gameType)
{
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

Difficulty BedrockLevelDatReader::_parseDifficulty(i32 difficulty)
{
    switch (difficulty) {
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

WorldType BedrockLevelDatReader::_parseWorldType(const std::string& generatorName)
{
    if (generatorName == "flat") {
        return WorldType::Flat;
    }
    if (generatorName == "largeBiomes") {
        return WorldType::LargeBiomes;
    }
    if (generatorName == "amplified") {
        return WorldType::Amplified;
    }
    return WorldType::Default;
}

mc::resource::ResourceLocation BedrockLevelDatReader::_parseWorldPresetId(const std::string& generatorName)
{
    // Bedrock level.dat 无 Data.Reborn compound，worldPresetId 按 generatorName 推导（与 worldType 对齐）。
    if (generatorName == "flat") {
        return mc::resource::ResourceLocation("minecraft", "flat");
    }
    if (generatorName == "largeBiomes") {
        return mc::resource::ResourceLocation("minecraft", "large_biomes");
    }
    if (generatorName == "amplified") {
        return mc::resource::ResourceLocation("minecraft", "amplified");
    }
    return mc::resource::ResourceLocation("minecraft", "normal");
}

} // namespace mc::world::storage::reader::bedrock
