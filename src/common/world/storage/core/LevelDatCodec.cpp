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

#include "common/world/storage/core/LevelDatCodec.hpp"
#include "common/util/CompressionUtils.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/storage/list/WorldNameSanitizer.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

namespace {

constexpr i32 MC_REGION_VERSION = 19132;
constexpr i32 MC_ANVIL_VERSION = 19133;

} // anonymous namespace

LevelVersionInfo::LevelVersionInfo(i32 storageVersion, i32 dataVersion, std::string versionName, bool snapshot)
    : storageVersion(storageVersion)
    , dataVersion(dataVersion)
    , versionName(std::move(versionName))
    , snapshot(snapshot)
{}

LevelSummaryData::LevelSummaryData(std::string displayName,
    i64 lastPlayedMs,
    GameMode gameMode,
    Difficulty difficulty,
    bool hardcore,
    bool allowCommands,
    u64 seed,
    WorldType worldType,
    resource::ResourceLocation worldPresetId,
    LevelVersionInfo version,
    i32 storageVersion,
    i32 dataVersion,
    WorldCompatibility compatibility,
    std::string errorMessage)
    : displayName(std::move(displayName))
    , lastPlayedMs(lastPlayedMs)
    , gameMode(gameMode)
    , difficulty(difficulty)
    , hardcore(hardcore)
    , allowCommands(allowCommands)
    , seed(seed)
    , worldType(worldType)
    , worldPresetId(std::move(worldPresetId))
    , version(std::move(version))
    , storageVersion(storageVersion)
    , dataVersion(dataVersion)
    , compatibility(compatibility)
    , errorMessage(std::move(errorMessage))
{}

LevelRuntimeData::LevelRuntimeData(LevelSummaryData summary,
    i32 spawnX,
    i32 spawnY,
    i32 spawnZ,
    f32 spawnAngle,
    i64 gameTime,
    i64 dayTime,
    i32 clearWeatherTime,
    i32 rainTime,
    bool raining,
    i32 thunderTime,
    bool thundering,
    bool initialized,
    bool difficultyLocked)
    : summary(std::move(summary))
    , spawnX(spawnX)
    , spawnY(spawnY)
    , spawnZ(spawnZ)
    , spawnAngle(spawnAngle)
    , gameTime(gameTime)
    , dayTime(dayTime)
    , clearWeatherTime(clearWeatherTime)
    , rainTime(rainTime)
    , raining(raining)
    , thunderTime(thunderTime)
    , thundering(thundering)
    , initialized(initialized)
    , difficultyLocked(difficultyLocked)
{}

Result<std::unique_ptr<nbt::tags::compound_tag>> LevelDatCodec::_readGzipNbt(const std::filesystem::path& filePath)
{
    // 读取整个文件
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileNotFound, "Cannot open level.dat: " + filePath.string());
    }

    std::vector<u8> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (compressed.empty()) {
        return Error(ErrorCode::FileCorrupted, "level.dat is empty: " + filePath.string());
    }

    // 解压 gzip
    std::vector<u8> decompressed = util::decompressGzip(compressed);
    if (decompressed.empty()) {
        return Error(ErrorCode::DecompressionFailed, "Failed to decompress level.dat: " + filePath.string());
    }

    // 解析 NBT
    std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
    stream >> nbt::contexts::java;

    auto root = nbt::tags::compound_tag::read(stream);
    if (!root) {
        return Error(ErrorCode::FileCorrupted, "Failed to parse level.dat NBT: " + filePath.string());
    }

    return root;
}

Result<void> LevelDatCodec::_writeGzipNbt(const std::filesystem::path& filePath, const nbt::tags::compound_tag& root)
{
    // 序列化 NBT
    std::ostringstream stream;
    stream << nbt::contexts::java;
    root.write(stream);

    std::string nbtData = stream.str();
    std::vector<u8> uncompressed(nbtData.begin(), nbtData.end());

    // 压缩为 gzip
    std::vector<u8> compressed = util::compressGzip(uncompressed);
    if (compressed.empty()) {
        return Error(ErrorCode::FileWriteFailed, "Failed to compress level.dat");
    }

    // 写入文件
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return Error(ErrorCode::FileWriteFailed, "Cannot open level.dat for writing: " + filePath.string());
    }

    file.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    file.close();

    return Result<void>::ok();
}

Result<LevelSummaryData> LevelDatCodec::readSummary(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDat = worldDir / "level.dat";
    std::filesystem::path levelDatOld = worldDir / "level.dat_old";

    // 尝试读取 level.dat
    auto rootResult = _readGzipNbt(levelDat);
    if (rootResult.failed()) {
        // 尝试读取 level.dat_old
        rootResult = _readGzipNbt(levelDatOld);
        if (rootResult.failed()) {
            return rootResult.error();
        }
    }

    return parseSummary(*rootResult.value());
}

GameMode LevelDatCodec::_parseGameMode(const nbt::tags::compound_tag& data)
{
    static constexpr GameMode GAME_MODE_TABLE[] = {
        GameMode::Survival,  // 0
        GameMode::Creative,  // 1
        GameMode::Adventure, // 2
        GameMode::Spectator  // 3
    };

    auto it = data.value.find("GameType");
    if (it != data.value.end() && it->second->id() == nbt::TagId::Int) {
        i32 gameType = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        if (gameType >= 0 && gameType <= 3) {
            return GAME_MODE_TABLE[gameType];
        }
    }
    return GameMode::Survival;
}

void LevelDatCodec::_writeGameMode(nbt::tags::compound_tag& data, GameMode gameMode)
{
    static constexpr i32 GAME_TYPE_TABLE[] = {
        0, // Survival
        1, // Creative
        2, // Adventure
        3, // Spectator
        0  // NotSet -> Survival
    };

    i32 index = static_cast<i32>(gameMode);
    if (index < 0 || index > 4) {
        index = 0;
    }
    data.put("GameType", GAME_TYPE_TABLE[index]);
}

Difficulty LevelDatCodec::_parseDifficulty(const nbt::tags::compound_tag& data)
{
    static constexpr Difficulty DIFFICULTY_TABLE[] = {
        Difficulty::Peaceful, // 0
        Difficulty::Easy,     // 1
        Difficulty::Normal,   // 2
        Difficulty::Hard      // 3
    };

    auto it = data.value.find("Difficulty");
    if (it != data.value.end() && it->second->id() == nbt::TagId::Byte) {
        i8 difficulty = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
        if (difficulty >= 0 && difficulty <= 3) {
            return DIFFICULTY_TABLE[difficulty];
        }
    }
    return Difficulty::Normal;
}

void LevelDatCodec::_writeDifficulty(nbt::tags::compound_tag& data, Difficulty difficulty)
{
    static constexpr i8 DIFFICULTY_TABLE[] = {
        0, // Peaceful
        1, // Easy
        2, // Normal
        3  // Hard
    };

    i32 index = static_cast<i32>(difficulty);
    if (index >= 0 && index <= 3) {
        data.put("Difficulty", DIFFICULTY_TABLE[index]);
    } else {
        data.put("Difficulty", static_cast<i8>(2)); // 默认 Normal
    }
}

WorldType LevelDatCodec::_parseWorldType(const nbt::tags::compound_tag& data)
{
    // 检查项目私有的 WorldType 字段
    auto rebornIt = data.value.find("Reborn");
    if (rebornIt != data.value.end() && rebornIt->second->id() == nbt::TagId::Compound) {
        auto& reborn = dynamic_cast<const nbt::tags::compound_tag&>(*rebornIt->second);
        auto worldTypeIt = reborn.value.find("WorldType");
        if (worldTypeIt != reborn.value.end() && worldTypeIt->second->id() == nbt::TagId::String) {
            std::string worldTypeStr = dynamic_cast<const nbt::tags::string_tag&>(*worldTypeIt->second).value;
            // 解析世界类型字符串
            if (worldTypeStr == "flat") return WorldType::Flat;
            if (worldTypeStr == "largeBiomes" || worldTypeStr == "large_biomes") return WorldType::LargeBiomes;
            if (worldTypeStr == "amplified") return WorldType::Amplified;
            if (worldTypeStr == "debug_all_block_states" || worldTypeStr == "debug") return WorldType::Debug;
        }
    }

    // 兼容原版 generatorName
    auto it = data.value.find("generatorName");
    if (it != data.value.end() && it->second->id() == nbt::TagId::String) {
        std::string generatorName = dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;
        if (generatorName == "flat") return WorldType::Flat;
        if (generatorName == "largeBiomes") return WorldType::LargeBiomes;
        if (generatorName == "amplified") return WorldType::Amplified;
        if (generatorName == "debug_all_block_states") return WorldType::Debug;
    }

    return WorldType::Default;
}

resource::ResourceLocation LevelDatCodec::_parseWorldPresetId(const nbt::tags::compound_tag& data)
{
    // 读项目私有 Data.Reborn.WorldPresetId（worldPresetId 无原版对应，仅写 Reborn compound）
    auto rebornIt = data.value.find("Reborn");
    if (rebornIt != data.value.end() && rebornIt->second->id() == nbt::TagId::Compound) {
        auto& reborn = dynamic_cast<const nbt::tags::compound_tag&>(*rebornIt->second);
        auto it = reborn.value.find("WorldPresetId");
        if (it != reborn.value.end() && it->second->id() == nbt::TagId::String) {
            const auto& str = dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;
            if (!str.empty()) {
                return resource::ResourceLocation::parse(str);
            }
        }
    }
    return resource::ResourceLocation("minecraft", "default");
}

void LevelDatCodec::_writeReborn(
    nbt::tags::compound_tag& data, WorldType worldType, const resource::ResourceLocation& worldPresetId)
{
    // fetch-or-create Data.Reborn compound，一次性写 WorldType + WorldPresetId 两键。
    // 旧 _writeWorldType 用 emplace("Reborn", 新建 compound) 会整体替换 Reborn，
    // 若分两次 emplace 则后者被静默丢弃——故合并为单次 fetch-or-create。
    nbt::tags::compound_tag* reborn = nullptr;
    auto rebornIt = data.value.find("Reborn");
    if (rebornIt != data.value.end() && rebornIt->second->id() == nbt::TagId::Compound) {
        reborn = &dynamic_cast<nbt::tags::compound_tag&>(*rebornIt->second);
    } else {
        auto rebornPtr = std::make_unique<nbt::tags::compound_tag>();
        reborn = rebornPtr.get();
        data.value.emplace("Reborn", std::move(rebornPtr));
    }
    reborn->put("WorldType", std::string(worldTypeName(worldType)));
    reborn->put("WorldPresetId", worldPresetId.toString());

    // 兼容原版 generatorName
    std::string generatorName = "default";
    switch (worldType) {
        case WorldType::Flat:
            generatorName = "flat";
            break;
        case WorldType::LargeBiomes:
            generatorName = "largeBiomes";
            break;
        case WorldType::Amplified:
            generatorName = "amplified";
            break;
        case WorldType::Debug:
            generatorName = "debug_all_block_states";
            break;
        case WorldType::Default:
            break;
    }
    data.put("generatorName", generatorName);
}

WorldCompatibility LevelDatCodec::_determineCompatibility(i32 storageVersion, i32 dataVersion)
{
    // 检查存储格式版本
    if (storageVersion != MC_ANVIL_VERSION) {
        if (storageVersion == MC_REGION_VERSION) {
            return WorldCompatibility::UnsupportedStorage;
        }
        return WorldCompatibility::Unknown;
    }

    // 检查数据版本
    if (dataVersion > CURRENT_DATA_VERSION) {
        return WorldCompatibility::Newer;
    } else if (dataVersion < CURRENT_DATA_VERSION) {
        return WorldCompatibility::Older;
    }

    return WorldCompatibility::Current;
}

Result<LevelSummaryData> LevelDatCodec::parseSummary(const nbt::tags::compound_tag& root)
{
    // 获取 Data 复合标签
    auto dataIt = root.value.find("Data");
    if (dataIt == root.value.end() || dataIt->second->id() != nbt::TagId::Compound) {
        return Error(ErrorCode::FileCorrupted, "Missing Data compound in level.dat");
    }

    const auto& data = dynamic_cast<const nbt::tags::compound_tag&>(*dataIt->second);

    // 解析显示名称
    std::string displayName;
    auto levelNameIt = data.value.find("LevelName");
    if (levelNameIt != data.value.end() && levelNameIt->second->id() == nbt::TagId::String) {
        displayName = dynamic_cast<const nbt::tags::string_tag&>(*levelNameIt->second).value;
    }

    // 解析最后游玩时间
    i64 lastPlayedMs = 0;
    auto lastPlayedIt = data.value.find("LastPlayed");
    if (lastPlayedIt != data.value.end() && lastPlayedIt->second->id() == nbt::TagId::Long) {
        lastPlayedMs = dynamic_cast<const nbt::tags::long_tag&>(*lastPlayedIt->second).value;
    }

    // 解析游戏模式
    GameMode gameMode = _parseGameMode(data);

    // 解析难度
    Difficulty difficulty = _parseDifficulty(data);

    // 解析极限模式
    bool hardcore = false;
    auto hardcoreIt = data.value.find("hardcore");
    if (hardcoreIt != data.value.end() && hardcoreIt->second->id() == nbt::TagId::Byte) {
        hardcore = dynamic_cast<const nbt::tags::byte_tag&>(*hardcoreIt->second).value != 0;
    }

    // 解析作弊
    bool allowCommands = false;
    auto commandsIt = data.value.find("allowCommands");
    if (commandsIt != data.value.end() && commandsIt->second->id() == nbt::TagId::Byte) {
        allowCommands = dynamic_cast<const nbt::tags::byte_tag&>(*commandsIt->second).value != 0;
    } else {
        // 默认：创造模式允许作弊
        allowCommands = (gameMode == GameMode::Creative);
    }

    // 解析种子
    u64 seed = 0;
    auto randomSeedIt = data.value.find("RandomSeed");
    if (randomSeedIt != data.value.end() && randomSeedIt->second->id() == nbt::TagId::Long) {
        seed = static_cast<u64>(dynamic_cast<const nbt::tags::long_tag&>(*randomSeedIt->second).value);
    }

    // 解析世界类型
    WorldType worldType = _parseWorldType(data);

    // 解析世界预设资源位置（Data.Reborn.WorldPresetId，缺失默认 minecraft:default）
    resource::ResourceLocation worldPresetId = _parseWorldPresetId(data);

    // 解析版本信息
    i32 storageVersion = MC_ANVIL_VERSION;
    auto versionIt = data.value.find("version");
    if (versionIt != data.value.end() && versionIt->second->id() == nbt::TagId::Int) {
        storageVersion = dynamic_cast<const nbt::tags::int_tag&>(*versionIt->second).value;
    }

    i32 dataVersion = 0;
    std::string versionName;
    bool snapshot = false;

    auto dataVersionIt = data.value.find("DataVersion");
    if (dataVersionIt != data.value.end() && dataVersionIt->second->id() == nbt::TagId::Int) {
        dataVersion = dynamic_cast<const nbt::tags::int_tag&>(*dataVersionIt->second).value;
    }

    auto versionCompoundIt = data.value.find("Version");
    if (versionCompoundIt != data.value.end() && versionCompoundIt->second->id() == nbt::TagId::Compound) {
        const auto& versionCompound = dynamic_cast<const nbt::tags::compound_tag&>(*versionCompoundIt->second);

        auto nameIt = versionCompound.value.find("Name");
        if (nameIt != versionCompound.value.end() && nameIt->second->id() == nbt::TagId::String) {
            versionName = dynamic_cast<const nbt::tags::string_tag&>(*nameIt->second).value;
        }

        auto idIt = versionCompound.value.find("Id");
        if (idIt != versionCompound.value.end() && idIt->second->id() == nbt::TagId::Int) {
            dataVersion = dynamic_cast<const nbt::tags::int_tag&>(*idIt->second).value;
        }

        auto snapshotIt = versionCompound.value.find("Snapshot");
        if (snapshotIt != versionCompound.value.end() && snapshotIt->second->id() == nbt::TagId::Byte) {
            snapshot = dynamic_cast<const nbt::tags::byte_tag&>(*snapshotIt->second).value != 0;
        }
    }

    LevelVersionInfo versionInfo(storageVersion, dataVersion, versionName, snapshot);
    WorldCompatibility compatibility = _determineCompatibility(storageVersion, dataVersion);

    return LevelSummaryData(std::move(displayName),
        lastPlayedMs,
        gameMode,
        difficulty,
        hardcore,
        allowCommands,
        seed,
        worldType,
        std::move(worldPresetId),
        std::move(versionInfo),
        storageVersion,
        dataVersion,
        compatibility,
        "" // No error message if successfully parsed
    );
}

std::unique_ptr<nbt::tags::compound_tag> LevelDatCodec::_buildInitialNbt(
    const CreateWorldRequest& request, i64 lastPlayedMs)
{
    auto root = std::make_unique<nbt::tags::compound_tag>();
    nbt::tags::compound_tag data;

    // 基本信息
    data.put("LevelName", request.displayName);
    data.put("LastPlayed", lastPlayedMs);

    // 游戏设置
    _writeGameMode(data, request.gameMode);
    _writeDifficulty(data, request.difficulty);
    data.put("hardcore", static_cast<i8>(request.hardcore ? 1 : 0));
    data.put("allowCommands", static_cast<i8>(request.allowCommands ? 1 : 0));
    data.put("RandomSeed", static_cast<i64>(request.seed));

    // 世界类型 + worldPresetId（一并写入 Data.Reborn compound）
    _writeReborn(data, request.worldType, request.worldPresetId);

    // 版本信息
    data.put("version", MC_ANVIL_VERSION);
    data.put("DataVersion", CURRENT_DATA_VERSION);

    auto versionCompound = std::make_unique<nbt::tags::compound_tag>();
    versionCompound->put("Name", std::string(PROJECT_NAME));
    versionCompound->put("Id", CURRENT_DATA_VERSION);
    versionCompound->put("Snapshot", static_cast<i8>(0));
    data.value.emplace("Version", std::move(versionCompound));

    // 初始状态
    data.put("initialized", static_cast<i8>(0)); // 服务端初始化后会设为 1
    data.put("DifficultyLocked", static_cast<i8>(0));

    // 出生点（服务端初始化后会设置）
    data.put("SpawnX", 0);
    data.put("SpawnY", 0);
    data.put("SpawnZ", 0);
    data.put("SpawnAngle", 0.0f);

    // 时间
    data.put("Time", static_cast<i64>(0));
    data.put("DayTime", static_cast<i64>(0));

    // 天气
    data.put("clearWeatherTime", static_cast<i32>(0));
    data.put("rainTime", static_cast<i32>(0));
    data.put("raining", static_cast<i8>(0));
    data.put("thunderTime", static_cast<i32>(0));
    data.put("thundering", static_cast<i8>(0));

    auto dataPtr = std::make_unique<nbt::tags::compound_tag>(std::move(data));
    root->value.emplace("Data", std::move(dataPtr));
    root->is_root = true;

    return root;
}

Result<void> LevelDatCodec::_atomicWrite(const std::filesystem::path& worldDir, const nbt::tags::compound_tag& root)
{
    std::filesystem::path levelDat = worldDir / "level.dat";
    std::filesystem::path levelDatOld = worldDir / "level.dat_old";
    std::filesystem::path levelDatTmp = worldDir / "level.dat.tmp";

    // 写入临时文件
    auto writeResult = _writeGzipNbt(levelDatTmp, root);
    if (writeResult.failed()) {
        return writeResult.error();
    }

    // 备份现有的 level.dat
    std::error_code ec;
    if (std::filesystem::exists(levelDat, ec)) {
        std::filesystem::copy_file(levelDat, levelDatOld, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            spdlog::warn("Failed to backup level.dat: {}", ec.message());
        }
    }

    // 替换 level.dat
    std::filesystem::rename(levelDatTmp, levelDat, ec);
    if (ec) {
        // 尝试恢复
        if (std::filesystem::exists(levelDatOld, ec)) {
            std::filesystem::copy_file(levelDatOld, levelDat, std::filesystem::copy_options::overwrite_existing, ec);
        }
        return Error(ErrorCode::FileWriteFailed, "Failed to replace level.dat: " + ec.message());
    }

    return Result<void>::ok();
}

Result<void> LevelDatCodec::writeInitial(const std::filesystem::path& worldDir, const CreateWorldRequest& request)
{
    // 获取当前时间戳
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    i64 lastPlayedMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // 构建 NBT
    auto root = _buildInitialNbt(request, lastPlayedMs);

    // 原子写入
    return _atomicWrite(worldDir, *root);
}

Result<void> LevelDatCodec::_readDataCompound(const std::filesystem::path& worldDir,
    std::unique_ptr<nbt::tags::compound_tag>& outRoot,
    nbt::tags::compound_tag*& outData)
{
    auto rootResult = _readGzipNbt(worldDir / "level.dat");
    if (rootResult.failed()) {
        return rootResult.error();
    }

    outRoot = rootResult.value();

    auto dataIt = outRoot->value.find("Data");
    if (dataIt == outRoot->value.end() || dataIt->second->id() != nbt::TagId::Compound) {
        return Error(ErrorCode::FileCorrupted, "Missing Data compound in level.dat");
    }

    outData = &dynamic_cast<nbt::tags::compound_tag&>(*dataIt->second);
    return Result<void>::ok();
}

Result<void> LevelDatCodec::updateDisplayName(const std::filesystem::path& worldDir, const std::string& newDisplayName)
{
    std::unique_ptr<nbt::tags::compound_tag> root;
    nbt::tags::compound_tag* data = nullptr;

    auto result = _readDataCompound(worldDir, root, data);
    if (result.failed()) {
        return result.error();
    }

    data->put("LevelName", newDisplayName);
    return _atomicWrite(worldDir, *root);
}

Result<void> LevelDatCodec::updateLastPlayed(const std::filesystem::path& worldDir, i64 lastPlayedMs)
{
    std::unique_ptr<nbt::tags::compound_tag> root;
    nbt::tags::compound_tag* data = nullptr;

    auto result = _readDataCompound(worldDir, root, data);
    if (result.failed()) {
        return result.error();
    }

    data->put("LastPlayed", lastPlayedMs);
    return _atomicWrite(worldDir, *root);
}

Result<LevelRuntimeData> LevelDatCodec::readRuntimeData(const std::filesystem::path& worldDir)
{
    // 读取 level.dat 文件
    auto rootResult = _readGzipNbt(worldDir / "level.dat");
    if (rootResult.failed()) {
        // 尝试读取备份文件
        rootResult = _readGzipNbt(worldDir / "level.dat_old");
        if (rootResult.failed()) {
            return rootResult.error();
        }
    }

    // 保存 unique_ptr 防止悬空引用
    auto rootPtr = rootResult.value();
    const auto& root = *rootPtr;

    // 获取 Data 复合标签
    auto dataIt = root.value.find("Data");
    if (dataIt == root.value.end() || dataIt->second->id() != nbt::TagId::Compound) {
        return Error(ErrorCode::FileCorrupted, "Missing Data compound in level.dat");
    }

    const auto& data = dynamic_cast<const nbt::tags::compound_tag&>(*dataIt->second);

    // 先解析摘要数据
    auto summaryResult = parseSummary(root);
    if (summaryResult.failed()) {
        return summaryResult.error();
    }

    // 解析出生点
    i32 spawnX = 0, spawnY = 64, spawnZ = 0;
    if (data.value.count("SpawnX") != 0) {
        auto it = data.value.find("SpawnX");
        if (it->second->id() == nbt::TagId::Int) {
            spawnX = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        }
    }
    if (data.value.count("SpawnY") != 0) {
        auto it = data.value.find("SpawnY");
        if (it->second->id() == nbt::TagId::Int) {
            spawnY = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        }
    }
    if (data.value.count("SpawnZ") != 0) {
        auto it = data.value.find("SpawnZ");
        if (it->second->id() == nbt::TagId::Int) {
            spawnZ = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        }
    }

    // 解析出生点朝向
    f32 spawnAngle = 0.0f;
    if (data.value.count("SpawnAngle") != 0) {
        auto it = data.value.find("SpawnAngle");
        if (it->second->id() == nbt::TagId::Float) {
            spawnAngle = dynamic_cast<const nbt::tags::float_tag&>(*it->second).value;
        }
    }

    // 解析游戏时间
    i64 gameTime = 0;
    if (data.value.count("Time") != 0) {
        auto it = data.value.find("Time");
        if (it->second->id() == nbt::TagId::Long) {
            gameTime = dynamic_cast<const nbt::tags::long_tag&>(*it->second).value;
        }
    }

    // 解析日光时间
    i64 dayTime = 0;
    if (data.value.count("DayTime") != 0) {
        auto it = data.value.find("DayTime");
        if (it->second->id() == nbt::TagId::Long) {
            dayTime = dynamic_cast<const nbt::tags::long_tag&>(*it->second).value;
        }
    }

    // 解析天气状态
    i32 clearWeatherTime = 0;
    if (data.value.count("clearWeatherTime") != 0) {
        auto it = data.value.find("clearWeatherTime");
        if (it->second->id() == nbt::TagId::Int) {
            clearWeatherTime = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        }
    }

    i32 rainTime = 0;
    if (data.value.count("rainTime") != 0) {
        auto it = data.value.find("rainTime");
        if (it->second->id() == nbt::TagId::Int) {
            rainTime = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        }
    }

    bool raining = false;
    if (data.value.count("raining") != 0) {
        auto it = data.value.find("raining");
        if (it->second->id() == nbt::TagId::Byte) {
            raining = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
        }
    }

    i32 thunderTime = 0;
    if (data.value.count("thunderTime") != 0) {
        auto it = data.value.find("thunderTime");
        if (it->second->id() == nbt::TagId::Int) {
            thunderTime = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        }
    }

    bool thundering = false;
    if (data.value.count("thundering") != 0) {
        auto it = data.value.find("thundering");
        if (it->second->id() == nbt::TagId::Byte) {
            thundering = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
        }
    }

    bool initialized = false;
    if (data.value.count("initialized") != 0) {
        auto it = data.value.find("initialized");
        if (it->second->id() == nbt::TagId::Byte) {
            initialized = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
        }
    }

    bool difficultyLocked = false;
    if (data.value.count("DifficultyLocked") != 0) {
        auto it = data.value.find("DifficultyLocked");
        if (it->second->id() == nbt::TagId::Byte) {
            difficultyLocked = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
        }
    }

    return LevelRuntimeData(std::move(summaryResult.value()),
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

Result<void> LevelDatCodec::updateRuntimeData(const std::filesystem::path& worldDir,
    i64 gameTime,
    i64 dayTime,
    i32 spawnX,
    i32 spawnY,
    i32 spawnZ,
    f32 spawnAngle,
    i32 clearWeatherTime,
    i32 rainTime,
    bool raining,
    i32 thunderTime,
    bool thundering)
{
    std::unique_ptr<nbt::tags::compound_tag> root;
    nbt::tags::compound_tag* data = nullptr;

    auto result = _readDataCompound(worldDir, root, data);
    if (result.failed()) {
        return result.error();
    }

    // 更新时间字段
    data->put("Time", gameTime);
    data->put("DayTime", dayTime);

    // 更新出生点
    data->put("SpawnX", spawnX);
    data->put("SpawnY", spawnY);
    data->put("SpawnZ", spawnZ);
    data->put("SpawnAngle", spawnAngle);

    // 更新天气状态
    data->put("clearWeatherTime", clearWeatherTime);
    data->put("rainTime", rainTime);
    data->put("raining", static_cast<i8>(raining ? 1 : 0));
    data->put("thunderTime", thunderTime);
    data->put("thundering", static_cast<i8>(thundering ? 1 : 0));

    // 更新最后游玩时间
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    i64 lastPlayedMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    data->put("LastPlayed", lastPlayedMs);

    return _atomicWrite(worldDir, *root);
}

Result<std::unique_ptr<nbt::tags::compound_list_tag>> LevelDatCodec::readScheduledEvents(
    const std::filesystem::path& worldDir)
{
    auto rootResult = _readGzipNbt(worldDir / "level.dat");
    if (rootResult.failed()) {
        rootResult = _readGzipNbt(worldDir / "level.dat_old");
        if (rootResult.failed()) {
            return rootResult.error();
        }
    }

    auto rootPtr = rootResult.value();
    const auto& root = *rootPtr;

    auto dataIt = root.value.find("Data");
    if (dataIt == root.value.end() || dataIt->second->id() != nbt::TagId::Compound) {
        // 没有 Data 复合标签，返回空列表
        return std::make_unique<nbt::tags::compound_list_tag>();
    }

    const auto& data = dynamic_cast<const nbt::tags::compound_tag&>(*dataIt->second);
    auto eventsIt = data.value.find("ScheduledEvents");
    if (eventsIt == data.value.end()) {
        // 没有 ScheduledEvents 字段，返回空列表
        return std::make_unique<nbt::tags::compound_list_tag>();
    }

    if (eventsIt->second->id() != nbt::TagId::List) {
        spdlog::warn("ScheduledEvents in level.dat is not a list tag, ignoring");
        return std::make_unique<nbt::tags::compound_list_tag>();
    }

    const auto& listTag = dynamic_cast<const nbt::tags::list_tag&>(*eventsIt->second);
    if (listTag.element_id() != nbt::TagId::Compound) {
        spdlog::warn("ScheduledEvents in level.dat is not a compound list, ignoring");
        return std::make_unique<nbt::tags::compound_list_tag>();
    }

    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(listTag);
    auto result = std::make_unique<nbt::tags::compound_list_tag>();
    result->value.reserve(compoundList.value.size());
    for (const auto& entry : compoundList.value) {
        result->value.push_back(entry); // compound_tag 拷贝构造
    }

    return result;
}

Result<void> LevelDatCodec::updateScheduledEvents(
    const std::filesystem::path& worldDir, const nbt::tags::compound_list_tag& events)
{
    std::unique_ptr<nbt::tags::compound_tag> root;
    nbt::tags::compound_tag* data = nullptr;

    auto result = _readDataCompound(worldDir, root, data);
    if (result.failed()) {
        return result.error();
    }

    if (events.value.empty()) {
        // 空列表时删除 ScheduledEvents 键
        data->value.erase("ScheduledEvents");
    } else {
        // 写入 ScheduledEvents 列表
        auto listCopy = std::make_unique<nbt::tags::compound_list_tag>();
        listCopy->value.reserve(events.value.size());
        for (const auto& entry : events.value) {
            listCopy->value.push_back(entry);
        }
        data->value.emplace("ScheduledEvents", std::move(listCopy));
    }

    return _atomicWrite(worldDir, *root);
}

} // namespace mc::world::storage
