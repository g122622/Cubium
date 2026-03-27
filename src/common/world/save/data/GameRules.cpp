#include "GameRules.hpp"

namespace mc::world::save::data {

// ========== 规则名称映射 ==========

namespace {

struct RuleInfo {
    GameRules::Type type;
    const char* name;
    bool defaultBool;
    i32 defaultInt;
    f64 defaultDouble;
};

constexpr RuleInfo RULE_INFO[] = {
    // 布尔型规则
    {GameRules::Type::Boolean, "doDaylightCycle", true, 0, 0.0},
    {GameRules::Type::Boolean, "doEntityDrops", true, 0, 0.0},
    {GameRules::Type::Boolean, "doFireTick", true, 0, 0.0},
    {GameRules::Type::Boolean, "doMobLoot", true, 0, 0.0},
    {GameRules::Type::Boolean, "doMobSpawning", true, 0, 0.0},
    {GameRules::Type::Boolean, "doTileDrops", true, 0, 0.0},
    {GameRules::Type::Boolean, "doWeatherCycle", true, 0, 0.0},
    {GameRules::Type::Boolean, "keepInventory", false, 0, 0.0},
    {GameRules::Type::Boolean, "logAdminCommands", true, 0, 0.0},
    {GameRules::Type::Boolean, "mobGriefing", true, 0, 0.0},
    {GameRules::Type::Boolean, "naturalRegeneration", true, 0, 0.0},
    {GameRules::Type::Boolean, "reducedDebugInfo", false, 0, 0.0},
    {GameRules::Type::Boolean, "sendCommandFeedback", true, 0, 0.0},
    {GameRules::Type::Boolean, "showDeathMessages", true, 0, 0.0},
    {GameRules::Type::Boolean, "spectatorsGenerateChunks", true, 0, 0.0},
    {GameRules::Type::Boolean, "disableElytraMovementCheck", false, 0, 0.0},
    {GameRules::Type::Boolean, "doInsomnia", true, 0, 0.0},
    {GameRules::Type::Boolean, "doLimitedCrafting", false, 0, 0.0},
    {GameRules::Type::Boolean, "doPatrolSpawning", true, 0, 0.0},
    {GameRules::Type::Boolean, "doTraderSpawning", true, 0, 0.0},
    {GameRules::Type::Boolean, "fallDamage", true, 0, 0.0},
    {GameRules::Type::Boolean, "fireDamage", true, 0, 0.0},
    {GameRules::Type::Boolean, "freezeDamage", true, 0, 0.0},
    {GameRules::Type::Boolean, "universalAnger", false, 0, 0.0},
    {GameRules::Type::Boolean, "forgivingDeathMessages", false, 0, 0.0},

    // 整型规则
    {GameRules::Type::Integer, "maxCommandChainLength", false, 65536, 0.0},
    {GameRules::Type::Integer, "maxEntityCramming", false, 24, 0.0},
    {GameRules::Type::Integer, "randomTickSpeed", false, 3, 0.0},
    {GameRules::Type::Integer, "spawnRadius", false, 10, 0.0},
    {GameRules::Type::Integer, "commandBlockOutput", false, 1, 0.0},

    // 浮点型规则
    {GameRules::Type::Double, "playerSpawnAngle", false, 0, 0.0},
};

} // namespace

// ========== 构造函数 ==========

GameRules::GameRules() {
    initializeDefaults();
}

void GameRules::initializeDefaults() {
    // 布尔型规则默认值
    m_boolRules[Key::DoDaylightCycle] = true;
    m_boolRules[Key::DoEntityDrops] = true;
    m_boolRules[Key::DoFireTick] = true;
    m_boolRules[Key::DoMobLoot] = true;
    m_boolRules[Key::DoMobSpawning] = true;
    m_boolRules[Key::DoTileDrops] = true;
    m_boolRules[Key::DoWeatherCycle] = true;
    m_boolRules[Key::KeepInventory] = false;
    m_boolRules[Key::LogAdminCommands] = true;
    m_boolRules[Key::MobGriefing] = true;
    m_boolRules[Key::NaturalRegeneration] = true;
    m_boolRules[Key::ReducedDebugInfo] = false;
    m_boolRules[Key::SendCommandFeedback] = true;
    m_boolRules[Key::ShowDeathMessages] = true;
    m_boolRules[Key::SpectatorsGenerateChunks] = true;
    m_boolRules[Key::DisableElytraMovementCheck] = false;
    m_boolRules[Key::DoInsomnia] = true;
    m_boolRules[Key::DoLimitedCrafting] = false;
    m_boolRules[Key::DoPatrolSpawning] = true;
    m_boolRules[Key::DoTraderSpawning] = true;
    m_boolRules[Key::FallDamage] = true;
    m_boolRules[Key::FireDamage] = true;
    m_boolRules[Key::FreezeDamage] = true;
    m_boolRules[Key::UniversalAnger] = false;
    m_boolRules[Key::ForgivingDeathMessages] = false;

    // 整型规则默认值
    m_intRules[Key::MaxCommandChainLength] = 65536;
    m_intRules[Key::MaxEntityCramming] = 24;
    m_intRules[Key::RandomTickSpeed] = 3;
    m_intRules[Key::SpawnRadius] = 10;
    m_intRules[Key::CommandBlockOutput] = 1;

    // 浮点型规则默认值
    m_doubleRules[Key::PlayerSpawnAngle] = 0.0;
}

// ========== 布尔型规则访问器 ==========

bool GameRules::doDaylightCycle() const { return m_boolRules.at(Key::DoDaylightCycle); }
void GameRules::setDoDaylightCycle(bool value) { m_boolRules[Key::DoDaylightCycle] = value; }

bool GameRules::doEntityDrops() const { return m_boolRules.at(Key::DoEntityDrops); }
void GameRules::setDoEntityDrops(bool value) { m_boolRules[Key::DoEntityDrops] = value; }

bool GameRules::doFireTick() const { return m_boolRules.at(Key::DoFireTick); }
void GameRules::setDoFireTick(bool value) { m_boolRules[Key::DoFireTick] = value; }

bool GameRules::doMobLoot() const { return m_boolRules.at(Key::DoMobLoot); }
void GameRules::setDoMobLoot(bool value) { m_boolRules[Key::DoMobLoot] = value; }

bool GameRules::doMobSpawning() const { return m_boolRules.at(Key::DoMobSpawning); }
void GameRules::setDoMobSpawning(bool value) { m_boolRules[Key::DoMobSpawning] = value; }

bool GameRules::doTileDrops() const { return m_boolRules.at(Key::DoTileDrops); }
void GameRules::setDoTileDrops(bool value) { m_boolRules[Key::DoTileDrops] = value; }

bool GameRules::doWeatherCycle() const { return m_boolRules.at(Key::DoWeatherCycle); }
void GameRules::setDoWeatherCycle(bool value) { m_boolRules[Key::DoWeatherCycle] = value; }

bool GameRules::keepInventory() const { return m_boolRules.at(Key::KeepInventory); }
void GameRules::setKeepInventory(bool value) { m_boolRules[Key::KeepInventory] = value; }

bool GameRules::mobGriefing() const { return m_boolRules.at(Key::MobGriefing); }
void GameRules::setMobGriefing(bool value) { m_boolRules[Key::MobGriefing] = value; }

bool GameRules::naturalRegeneration() const { return m_boolRules.at(Key::NaturalRegeneration); }
void GameRules::setNaturalRegeneration(bool value) { m_boolRules[Key::NaturalRegeneration] = value; }

// ========== 整型规则访问器 ==========

i32 GameRules::maxCommandChainLength() const { return m_intRules.at(Key::MaxCommandChainLength); }
void GameRules::setMaxCommandChainLength(i32 value) { m_intRules[Key::MaxCommandChainLength] = value; }

i32 GameRules::maxEntityCramming() const { return m_intRules.at(Key::MaxEntityCramming); }
void GameRules::setMaxEntityCramming(i32 value) { m_intRules[Key::MaxEntityCramming] = value; }

i32 GameRules::randomTickSpeed() const { return m_intRules.at(Key::RandomTickSpeed); }
void GameRules::setRandomTickSpeed(i32 value) { m_intRules[Key::RandomTickSpeed] = value; }

i32 GameRules::spawnRadius() const { return m_intRules.at(Key::SpawnRadius); }
void GameRules::setSpawnRadius(i32 value) { m_intRules[Key::SpawnRadius] = value; }

// ========== 浮点型规则访问器 ==========

f64 GameRules::playerSpawnAngle() const { return m_doubleRules.at(Key::PlayerSpawnAngle); }
void GameRules::setPlayerSpawnAngle(f64 value) { m_doubleRules[Key::PlayerSpawnAngle] = value; }

// ========== 通用访问器 ==========

bool GameRules::getBoolean(Key key, bool defaultValue) const {
    auto it = m_boolRules.find(key);
    return it != m_boolRules.end() ? it->second : defaultValue;
}

void GameRules::setBoolean(Key key, bool value) {
    m_boolRules[key] = value;
}

i32 GameRules::getInteger(Key key, i32 defaultValue) const {
    auto it = m_intRules.find(key);
    return it != m_intRules.end() ? it->second : defaultValue;
}

void GameRules::setInteger(Key key, i32 value) {
    m_intRules[key] = value;
}

f64 GameRules::getDouble(Key key, f64 defaultValue) const {
    auto it = m_doubleRules.find(key);
    return it != m_doubleRules.end() ? it->second : defaultValue;
}

void GameRules::setDouble(Key key, f64 value) {
    m_doubleRules[key] = value;
}

bool GameRules::setByName(const String& name, const String& value) {
    // 查找规则
    for (size_t i = 0; i < sizeof(RULE_INFO) / sizeof(RuleInfo); ++i) {
        if (RULE_INFO[i].name == name) {
            Key key = static_cast<Key>(i);
            Type type = RULE_INFO[i].type;

            switch (type) {
                case Type::Boolean:
                    if (value == "true" || value == "1") {
                        setBoolean(key, true);
                        return true;
                    } else if (value == "false" || value == "0") {
                        setBoolean(key, false);
                        return true;
                    }
                    return false;

                case Type::Integer:
                    try {
                        setInteger(key, std::stoi(value));
                        return true;
                    } catch (...) {
                        return false;
                    }

                case Type::Double:
                    try {
                        setDouble(key, std::stod(value));
                        return true;
                    } catch (...) {
                        return false;
                    }
            }
        }
    }
    return false;
}

GameRules::Type GameRules::getType(Key key) {
    size_t index = static_cast<size_t>(key);
    if (index < sizeof(RULE_INFO) / sizeof(RuleInfo)) {
        return RULE_INFO[index].type;
    }
    return Type::Boolean;
}

const char* GameRules::getName(Key key) {
    size_t index = static_cast<size_t>(key);
    if (index < sizeof(RULE_INFO) / sizeof(RuleInfo)) {
        return RULE_INFO[index].name;
    }
    return "unknown";
}

// ========== 序列化 ==========

std::unique_ptr<nbt::CompoundTag> GameRules::serialize() const {
    auto nbt = std::make_unique<nbt::CompoundTag>();

    // 布尔型规则
    for (const auto& [key, value] : m_boolRules) {
        nbt->put(getName(key), value);
    }

    // 整型规则
    for (const auto& [key, value] : m_intRules) {
        nbt->put(getName(key), value);
    }

    // 浮点型规则
    for (const auto& [key, value] : m_doubleRules) {
        nbt->put(getName(key), value);
    }

    return nbt;
}

void GameRules::deserialize(const nbt::CompoundTag& nbt) {
    // 遍历所有规则并从 NBT 加载
    for (size_t i = 0; i < sizeof(RULE_INFO) / sizeof(RuleInfo); ++i) {
        Key key = static_cast<Key>(i);
        const char* name = RULE_INFO[i].name;
        Type type = RULE_INFO[i].type;

        // 检查 NBT 中是否存在该规则
        if (!nbt.has(name)) {
            continue;
        }

        switch (type) {
            case Type::Boolean: {
                auto* tag = nbt.get_if<nbt::ByteTag>(name);
                if (tag) {
                    setBoolean(key, tag->get() != 0);
                }
                break;
            }
            case Type::Integer: {
                auto* tag = nbt.get_if<nbt::IntTag>(name);
                if (tag) {
                    setInteger(key, tag->get());
                }
                break;
            }
            case Type::Double: {
                auto* tag = nbt.get_if<nbt::DoubleTag>(name);
                if (tag) {
                    setDouble(key, tag->get());
                }
                break;
            }
        }
    }
}

} // namespace mc::world::save::data
