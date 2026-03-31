#include "EffectType.hpp"

namespace mc {
namespace entity {
namespace effect {

const char* getEffectName(EffectType type) {
    switch (type) {
        case EffectType::Speed: return "Speed";
        case EffectType::Slowness: return "Slowness";
        case EffectType::Haste: return "Haste";
        case EffectType::MiningFatigue: return "Mining Fatigue";
        case EffectType::Strength: return "Strength";
        case EffectType::InstantHealth: return "Instant Health";
        case EffectType::InstantDamage: return "Instant Damage";
        case EffectType::JumpBoost: return "Jump Boost";
        case EffectType::Nausea: return "Nausea";
        case EffectType::Regeneration: return "Regeneration";
        case EffectType::Resistance: return "Resistance";
        case EffectType::FireResistance: return "Fire Resistance";
        case EffectType::WaterBreathing: return "Water Breathing";
        case EffectType::Invisibility: return "Invisibility";
        case EffectType::Blindness: return "Blindness";
        case EffectType::NightVision: return "Night Vision";
        case EffectType::Hunger: return "Hunger";
        case EffectType::Weakness: return "Weakness";
        case EffectType::Poison: return "Poison";
        case EffectType::Wither: return "Wither";
        case EffectType::HealthBoost: return "Health Boost";
        case EffectType::Absorption: return "Absorption";
        case EffectType::Saturation: return "Saturation";
        case EffectType::Levitation: return "Levitation";
        case EffectType::Luck: return "Luck";
        case EffectType::BadLuck: return "Bad Luck";
        case EffectType::SlowFalling: return "Slow Falling";
        case EffectType::ConduitPower: return "Conduit Power";
        case EffectType::DolphinsGrace: return "Dolphin's Grace";
        case EffectType::BadOmen: return "Bad Omen";
        case EffectType::HeroOfTheVillage: return "Hero of the Village";
        default: return "Unknown";
    }
}

bool isBeneficialEffect(EffectType type) {
    switch (type) {
        case EffectType::Speed:
        case EffectType::Haste:
        case EffectType::Strength:
        case EffectType::InstantHealth:
        case EffectType::JumpBoost:
        case EffectType::Regeneration:
        case EffectType::Resistance:
        case EffectType::FireResistance:
        case EffectType::WaterBreathing:
        case EffectType::Invisibility:
        case EffectType::NightVision:
        case EffectType::HealthBoost:
        case EffectType::Absorption:
        case EffectType::Saturation:
        case EffectType::Luck:
        case EffectType::SlowFalling:
        case EffectType::ConduitPower:
        case EffectType::DolphinsGrace:
        case EffectType::HeroOfTheVillage:
            return true;
        default:
            return false;
    }
}

u32 getEffectColor(EffectType type) {
    // 参考 MC 1.16.5 效果颜色
    switch (type) {
        case EffectType::Speed: return 0x7CAFC6;
        case EffectType::Slowness: return 0x5A6C81;
        case EffectType::Haste: return 0xD9C043;
        case EffectType::MiningFatigue: return 0x4A7210;
        case EffectType::Strength: return 0x932423;
        case EffectType::InstantHealth: return 0xF82423;
        case EffectType::InstantDamage: return 0x430A09;
        case EffectType::JumpBoost: return 0x22FF4C;
        case EffectType::Nausea: return 0xC31C4D;
        case EffectType::Regeneration: return 0xCD5CAB;
        case EffectType::Resistance: return 0x99453A;
        case EffectType::FireResistance: return 0xE49A3A;
        case EffectType::WaterBreathing: return 0x2E5299;
        case EffectType::Invisibility: return 0x7F8392;
        case EffectType::Blindness: return 0x1F1F23;
        case EffectType::NightVision: return 0x1F1FA1;
        case EffectType::Hunger: return 0x587653;
        case EffectType::Weakness: return 0x484D48;
        case EffectType::Poison: return 0x4E9331;
        case EffectType::Wither: return 0x352A27;
        case EffectType::HealthBoost: return 0xF87D23;
        case EffectType::Absorption: return 0x2552A5;
        case EffectType::Saturation: return 0xF82423;
        case EffectType::Levitation: return 0xCEFFFF;
        case EffectType::Luck: return 0x339900;
        case EffectType::BadLuck: return 0xC0A44D;
        case EffectType::SlowFalling: return 0xFEFFF0;
        case EffectType::ConduitPower: return 0x1DC2D1;
        case EffectType::DolphinsGrace: return 0x7294C4;
        case EffectType::BadOmen: return 0x0B0B0B;
        case EffectType::HeroOfTheVillage: return 0x44FF44;
        default: return 0xFFFFFF;
    }
}

} // namespace effect
} // namespace entity
} // namespace mc
