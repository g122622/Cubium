#include "LevelData.hpp"
#include <chrono>

namespace mc::world::save::data {

// ========== 从设置创建 ==========

LevelData LevelData::fromSettings(const WorldSettings& settings) {
    LevelData data;
    data.levelName = settings.levelName;
    data.randomSeed = settings.seed;
    data.gameType = settings.gameType;
    data.difficulty = settings.difficulty;
    data.generateFeatures = settings.generateStructures;
    data.bonusChest = settings.bonusChest;
    data.hardcore = settings.hardcore;
    data.allowCommands = settings.allowCommands;
    data.generatorName = settings.generatorName;
    data.initialized = true;
    data.updateLastPlayed();
    return data;
}

// ========== 序列化 ==========

std::unique_ptr<nbt::CompoundTag>
LevelData::serialize(const nbt::CompoundTag* playerNbt) const {
    auto root = std::make_unique<nbt::CompoundTag>();

    // 数据版本
    root->put("DataVersion", dataVersion);

    // 版本信息
    auto versionNbt = std::make_unique<nbt::CompoundTag>();
    versionNbt->put("Id", version.id);
    versionNbt->put("Name", version.name);
    versionNbt->put("Stable", version.stable);
    root->put("Version", std::move(versionNbt));

    // Data 标签
    auto dataNbt = std::make_unique<nbt::CompoundTag>();

    // 版本（冗余存储）
    dataNbt->put("DataVersion", dataVersion);
    serializeVersion(*dataNbt);

    // 世界设置
    dataNbt->put("LevelName", levelName);
    dataNbt->put("GameType", static_cast<i32>(gameType));
    dataNbt->put("hardcore", hardcore);
    dataNbt->put("allowCommands", allowCommands);
    dataNbt->put("initialized", initialized);

    // 生成点
    dataNbt->put("SpawnX", spawnX);
    dataNbt->put("SpawnY", spawnY);
    dataNbt->put("SpawnZ", spawnZ);
    dataNbt->put("SpawnAngle", spawnAngle);

    // 时间
    dataNbt->put("Time", gameTime);
    dataNbt->put("DayTime", dayTime);
    dataNbt->put("LastPlayed", lastPlayed);

    // 天气
    serializeWeather(*dataNbt);

    // 难度
    dataNbt->put("Difficulty", static_cast<i8>(difficulty));
    dataNbt->put("DifficultyLocked", difficultyLocked);

    // 世界边界
    serializeWorldBorder(*dataNbt);

    // 世界生成设置
    serializeWorldGenSettings(*dataNbt);

    // 游戏规则
    serializeGameRules(*dataNbt);

    // 玩家数据（单人模式）
    if (playerNbt != nullptr) {
        dataNbt->put("Player", playerNbt->copy());
    }

    // 其他
    dataNbt->put("WanderingTraderSpawnDelay", wanderingTraderSpawnDelay);
    dataNbt->put("WanderingTraderSpawnChance", wanderingTraderSpawnChance);
    dataNbt->put("WasModded", wasModded);

    root->put("Data", std::move(dataNbt));
    return root;
}

void LevelData::serializeVersion(nbt::CompoundTag& nbt) const {
    auto versionNbt = std::make_unique<nbt::CompoundTag>();
    versionNbt->put("Id", version.id);
    versionNbt->put("Name", version.name);
    versionNbt->put("Stable", version.stable);
    nbt.put("Version", std::move(versionNbt));
}

void LevelData::serializeWeather(nbt::CompoundTag& nbt) const {
    nbt.put("clearWeatherTime", clearWeatherTime);
    nbt.put("rainTime", rainTime);
    nbt.put("raining", raining);
    nbt.put("thunderTime", thunderTime);
    nbt.put("thundering", thundering);
}

void LevelData::serializeWorldBorder(nbt::CompoundTag& nbt) const {
    nbt.put("BorderCenterX", worldBorder.centerX);
    nbt.put("BorderCenterZ", worldBorder.centerZ);
    nbt.put("BorderSize", worldBorder.size);
    nbt.put("BorderSizeL", worldBorder.sizeL);
    nbt.put("BorderSafeZone", worldBorder.safeZone);
    nbt.put("BorderDamagePerBlock", worldBorder.damagePerBlock);
    nbt.put("BorderWarningBlocks", worldBorder.warningBlocks);
    nbt.put("BorderWarningTime", worldBorder.warningTime);
}

void LevelData::serializeWorldGenSettings(nbt::CompoundTag& nbt) const {
    auto worldGenNbt = std::make_unique<nbt::CompoundTag>();

    // 种子
    worldGenNbt->put("seed", randomSeed);

    // 生成特征
    worldGenNbt->put("generate_features", generateFeatures);
    worldGenNbt->put("bonus_chest", bonusChest);

    // 维度设置（简化版，只存储生成器名称）
    auto dimensionsNbt = std::make_unique<nbt::CompoundTag>();
    // TODO: 完整的维度设置
    worldGenNbt->put("dimensions", std::move(dimensionsNbt));

    nbt.put("WorldGenSettings", std::move(worldGenNbt));
}

void LevelData::serializeGameRules(nbt::CompoundTag& nbt) const {
    auto rulesNbt = gameRules.serialize();
    nbt.put("GameRules", std::move(rulesNbt));
}

// ========== 反序列化 ==========

Result<std::unique_ptr<LevelData>>
LevelData::deserialize(const nbt::CompoundTag& nbt) {
    auto data = std::make_unique<LevelData>();

    // 获取 Data 标签
    const nbt::CompoundTag* dataNbt = nullptr;
    if (nbt.has("Data")) {
        dataNbt = nbt.get_if<nbt::CompoundTag>("Data");
    } else {
        // 某些旧版本可能直接在根标签存储
        dataNbt = &nbt;
    }

    if (dataNbt == nullptr) {
        return Error(ErrorCode::ResourceParseError, "Invalid level.dat: missing Data tag");
    }

    // 数据版本
    if (dataNbt->has("DataVersion")) {
        auto* tag = dataNbt->get_if<nbt::IntTag>("DataVersion");
        if (tag) {
            data->dataVersion = tag->get();
        }
    }

    // 版本信息
    deserializeVersion(*data, *dataNbt);

    // 世界设置
    if (dataNbt->has("LevelName")) {
        auto* tag = dataNbt->get_if<nbt::StringTag>("LevelName");
        if (tag) {
            data->levelName = tag->get();
        }
    }

    if (dataNbt->has("GameType")) {
        auto* tag = dataNbt->get_if<nbt::IntTag>("GameType");
        if (tag) {
            data->gameType = static_cast<GameType>(tag->get());
        }
    }

    if (dataNbt->has("hardcore")) {
        auto* tag = dataNbt->get_if<nbt::ByteTag>("hardcore");
        if (tag) {
            data->hardcore = tag->get() != 0;
        }
    }

    if (dataNbt->has("allowCommands")) {
        auto* tag = dataNbt->get_if<nbt::ByteTag>("allowCommands");
        if (tag) {
            data->allowCommands = tag->get() != 0;
        }
    }

    if (dataNbt->has("initialized")) {
        auto* tag = dataNbt->get_if<nbt::ByteTag>("initialized");
        if (tag) {
            data->initialized = tag->get() != 0;
        }
    }

    // 生成点
    if (dataNbt->has("SpawnX")) {
        auto* tag = dataNbt->get_if<nbt::IntTag>("SpawnX");
        if (tag) data->spawnX = tag->get();
    }
    if (dataNbt->has("SpawnY")) {
        auto* tag = dataNbt->get_if<nbt::IntTag>("SpawnY");
        if (tag) data->spawnY = tag->get();
    }
    if (dataNbt->has("SpawnZ")) {
        auto* tag = dataNbt->get_if<nbt::IntTag>("SpawnZ");
        if (tag) data->spawnZ = tag->get();
    }
    if (dataNbt->has("SpawnAngle")) {
        auto* tag = dataNbt->get_if<nbt::FloatTag>("SpawnAngle");
        if (tag) data->spawnAngle = tag->get();
    }

    // 时间
    if (dataNbt->has("Time")) {
        auto* tag = dataNbt->get_if<nbt::LongTag>("Time");
        if (tag) data->gameTime = tag->get();
    }
    if (dataNbt->has("DayTime")) {
        auto* tag = dataNbt->get_if<nbt::LongTag>("DayTime");
        if (tag) data->dayTime = tag->get();
    }
    if (dataNbt->has("LastPlayed")) {
        auto* tag = dataNbt->get_if<nbt::LongTag>("LastPlayed");
        if (tag) data->lastPlayed = tag->get();
    }

    // 天气
    deserializeWeather(*data, *dataNbt);

    // 难度
    if (dataNbt->has("Difficulty")) {
        auto* tag = dataNbt->get_if<nbt::ByteTag>("Difficulty");
        if (tag) {
            data->difficulty = static_cast<Difficulty>(tag->get());
        }
    }
    if (dataNbt->has("DifficultyLocked")) {
        auto* tag = dataNbt->get_if<nbt::ByteTag>("DifficultyLocked");
        if (tag) {
            data->difficultyLocked = tag->get() != 0;
        }
    }

    // 世界边界
    deserializeWorldBorder(*data, *dataNbt);

    // 世界生成设置
    deserializeWorldGenSettings(*data, *dataNbt);

    // 游戏规则
    deserializeGameRules(*data, *dataNbt);

    // 其他
    if (dataNbt->has("WanderingTraderSpawnDelay")) {
        auto* tag = dataNbt->get_if<nbt::IntTag>("WanderingTraderSpawnDelay");
        if (tag) data->wanderingTraderSpawnDelay = tag->get();
    }
    if (dataNbt->has("WanderingTraderSpawnChance")) {
        auto* tag = dataNbt->get_if<nbt::IntTag>("WanderingTraderSpawnChance");
        if (tag) data->wanderingTraderSpawnChance = tag->get();
    }
    if (dataNbt->has("WasModded")) {
        auto* tag = dataNbt->get_if<nbt::ByteTag>("WasModded");
        if (tag) data->wasModded = tag->get() != 0;
    }

    return data;
}

void LevelData::deserializeVersion(LevelData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("Version")) {
        auto* versionNbt = nbt.get_if<nbt::CompoundTag>("Version");
        if (versionNbt) {
            if (versionNbt->has("Id")) {
                auto* tag = versionNbt->get_if<nbt::IntTag>("Id");
                if (tag) data.version.id = tag->get();
            }
            if (versionNbt->has("Name")) {
                auto* tag = versionNbt->get_if<nbt::StringTag>("Name");
                if (tag) data.version.name = tag->get();
            }
            if (versionNbt->has("Stable")) {
                auto* tag = versionNbt->get_if<nbt::ByteTag>("Stable");
                if (tag) data.version.stable = tag->get() != 0;
            }
        }
    }
}

void LevelData::deserializeWeather(LevelData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("clearWeatherTime")) {
        auto* tag = nbt.get_if<nbt::IntTag>("clearWeatherTime");
        if (tag) data.clearWeatherTime = tag->get();
    }
    if (nbt.has("rainTime")) {
        auto* tag = nbt.get_if<nbt::IntTag>("rainTime");
        if (tag) data.rainTime = tag->get();
    }
    if (nbt.has("raining")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("raining");
        if (tag) data.raining = tag->get() != 0;
    }
    if (nbt.has("thunderTime")) {
        auto* tag = nbt.get_if<nbt::IntTag>("thunderTime");
        if (tag) data.thunderTime = tag->get();
    }
    if (nbt.has("thundering")) {
        auto* tag = nbt.get_if<nbt::ByteTag>("thundering");
        if (tag) data.thundering = tag->get() != 0;
    }
}

void LevelData::deserializeWorldBorder(LevelData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("BorderCenterX")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("BorderCenterX");
        if (tag) data.worldBorder.centerX = tag->get();
    }
    if (nbt.has("BorderCenterZ")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("BorderCenterZ");
        if (tag) data.worldBorder.centerZ = tag->get();
    }
    if (nbt.has("BorderSize")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("BorderSize");
        if (tag) data.worldBorder.size = tag->get();
    }
    if (nbt.has("BorderSizeL")) {
        auto* tag = nbt.get_if<nbt::LongTag>("BorderSizeL");
        if (tag) data.worldBorder.sizeL = static_cast<f64>(tag->get());
    }
    if (nbt.has("BorderSafeZone")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("BorderSafeZone");
        if (tag) data.worldBorder.safeZone = tag->get();
    }
    if (nbt.has("BorderDamagePerBlock")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("BorderDamagePerBlock");
        if (tag) data.worldBorder.damagePerBlock = tag->get();
    }
    if (nbt.has("BorderWarningBlocks")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("BorderWarningBlocks");
        if (tag) data.worldBorder.warningBlocks = tag->get();
    }
    if (nbt.has("BorderWarningTime")) {
        auto* tag = nbt.get_if<nbt::DoubleTag>("BorderWarningTime");
        if (tag) data.worldBorder.warningTime = tag->get();
    }
}

void LevelData::deserializeWorldGenSettings(LevelData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("WorldGenSettings")) {
        auto* worldGenNbt = nbt.get_if<nbt::CompoundTag>("WorldGenSettings");
        if (worldGenNbt) {
            if (worldGenNbt->has("seed")) {
                auto* tag = worldGenNbt->get_if<nbt::LongTag>("seed");
                if (tag) data.randomSeed = tag->get();
            }
            if (worldGenNbt->has("generate_features")) {
                auto* tag = worldGenNbt->get_if<nbt::ByteTag>("generate_features");
                if (tag) data.generateFeatures = tag->get() != 0;
            }
            if (worldGenNbt->has("bonus_chest")) {
                auto* tag = worldGenNbt->get_if<nbt::ByteTag>("bonus_chest");
                if (tag) data.bonusChest = tag->get() != 0;
            }
        }
    }

    // 兼容旧版本的 generatorName
    if (nbt.has("generatorName")) {
        auto* tag = nbt.get_if<nbt::StringTag>("generatorName");
        if (tag) data.generatorName = tag->get();
    }
    if (nbt.has("generatorOptions")) {
        auto* tag = nbt.get_if<nbt::StringTag>("generatorOptions");
        if (tag) data.generatorName = tag->get();
    }

    // 兼容旧版本的 RandomSeed
    if (nbt.has("RandomSeed")) {
        auto* tag = nbt.get_if<nbt::LongTag>("RandomSeed");
        if (tag) data.randomSeed = tag->get();
    }
}

void LevelData::deserializeGameRules(LevelData& data, const nbt::CompoundTag& nbt) {
    if (nbt.has("GameRules")) {
        auto* rulesNbt = nbt.get_if<nbt::CompoundTag>("GameRules");
        if (rulesNbt) {
            data.gameRules.deserialize(*rulesNbt);
        }
    }
}

// ========== 兼容性检查 ==========

bool LevelData::isCompatible() const {
    return DataVersion::isCompatible(dataVersion);
}

} // namespace mc::world::save::data
