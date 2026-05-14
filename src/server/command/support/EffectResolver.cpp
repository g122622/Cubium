#include "EffectResolver.hpp"

#include <algorithm>
#include <unordered_map>

namespace mc {
namespace command {
namespace support {

namespace {

const std::unordered_map<std::string, entity::effect::EffectType> s_effectNameMap = {
    {"speed", entity::effect::EffectType::Speed},
    {"slowness", entity::effect::EffectType::Slowness},
    {"haste", entity::effect::EffectType::Haste},
    {"mining_fatigue", entity::effect::EffectType::MiningFatigue},
    {"strength", entity::effect::EffectType::Strength},
    {"instant_health", entity::effect::EffectType::InstantHealth},
    {"instant_damage", entity::effect::EffectType::InstantDamage},
    {"jump_boost", entity::effect::EffectType::JumpBoost},
    {"nausea", entity::effect::EffectType::Nausea},
    {"regeneration", entity::effect::EffectType::Regeneration},
    {"resistance", entity::effect::EffectType::Resistance},
    {"fire_resistance", entity::effect::EffectType::FireResistance},
    {"water_breathing", entity::effect::EffectType::WaterBreathing},
    {"invisibility", entity::effect::EffectType::Invisibility},
    {"blindness", entity::effect::EffectType::Blindness},
    {"night_vision", entity::effect::EffectType::NightVision},
    {"hunger", entity::effect::EffectType::Hunger},
    {"weakness", entity::effect::EffectType::Weakness},
    {"poison", entity::effect::EffectType::Poison},
    {"wither", entity::effect::EffectType::Wither},
    {"health_boost", entity::effect::EffectType::HealthBoost},
    {"absorption", entity::effect::EffectType::Absorption},
    {"saturation", entity::effect::EffectType::Saturation},
    {"bad_omen", entity::effect::EffectType::BadOmen},
    {"hero_of_the_village", entity::effect::EffectType::HeroOfTheVillage},
    {"levitation", entity::effect::EffectType::Levitation},
    {"luck", entity::effect::EffectType::Luck},
    {"bad_luck", entity::effect::EffectType::BadLuck},
    {"slow_falling", entity::effect::EffectType::SlowFalling},
    {"conduit_power", entity::effect::EffectType::ConduitPower},
    {"dolphins_grace", entity::effect::EffectType::DolphinsGrace},
};

}

std::optional<entity::effect::EffectType> tryParseEffectType(std::string_view name) noexcept
{
    std::string lowerName(name.size(), '\0');
    std::transform(name.begin(), name.end(), lowerName.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });

    auto it = s_effectNameMap.find(lowerName);
    if (it != s_effectNameMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

const char* getEffectCommandName(entity::effect::EffectType type) noexcept
{
    return entity::effect::getEffectName(type);
}

} // namespace support
} // namespace command
} // namespace mc
