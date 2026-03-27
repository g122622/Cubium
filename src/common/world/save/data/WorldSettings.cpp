#include "WorldSettings.hpp"

namespace mc::world::save::data {

WorldSettings WorldSettings::survival(const String& name, i64 seed) {
    WorldSettings settings;
    settings.levelName = name;
    settings.seed = seed;
    settings.gameType = GameType::Survival;
    settings.difficulty = Difficulty::Normal;
    settings.generateStructures = true;
    settings.bonusChest = false;
    settings.hardcore = false;
    settings.allowCommands = false;
    return settings;
}

WorldSettings WorldSettings::creative(const String& name, i64 seed) {
    WorldSettings settings;
    settings.levelName = name;
    settings.seed = seed;
    settings.gameType = GameType::Creative;
    settings.difficulty = Difficulty::Normal;
    settings.generateStructures = true;
    settings.bonusChest = false;
    settings.hardcore = false;
    settings.allowCommands = true;
    return settings;
}

WorldSettings WorldSettings::flat(const String& name) {
    WorldSettings settings;
    settings.levelName = name;
    settings.seed = 0;
    settings.gameType = GameType::Creative;
    settings.difficulty = Difficulty::Peaceful;
    settings.generateStructures = false;
    settings.bonusChest = false;
    settings.hardcore = false;
    settings.allowCommands = true;
    settings.generatorName = "flat";
    return settings;
}

} // namespace mc::world::save::data
