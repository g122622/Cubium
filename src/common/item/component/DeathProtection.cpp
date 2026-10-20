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

#include "DataComponentMap.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"

namespace mc {
namespace item {
namespace component {

namespace {

/// 资源位置缺失命名空间时补 minecraft:（与 DataComponentPatchNbt 的 _normalizeId 同语义）
std::string _normalizeId(const std::string& text)
{
    return text.find(':') == std::string::npos ? "minecraft:" + text : text;
}

/// 单个 MobEffectInstance 条目（vanilla MobEffectInstance.CODEC，字段大写驼峰）。
/// id 解析失败（含数据包引用，Cubium 无效果注册表）时 instance 置空，由调用方跳过。
void _readEffectEntry(const nlohmann::json& json, ConsumeEffectEntry::EffectEntry& out)
{
    if (!json.is_object()) {
        return;
    }
    auto idIt = json.find("id");
    if (idIt == json.end() || !idIt->is_string()) {
        return;
    }
    auto type = entity::effect::getEffectByResourceLocation(ResourceLocation(_normalizeId(idIt->get<std::string>())));
    if (!type.has_value()) {
        return;
    }
    i32 duration = 600;
    if (auto it = json.find("duration"); it != json.end() && it->is_number_integer()) {
        duration = it->get<i32>();
    }
    i32 amplifier = 0;
    if (auto it = json.find("amplifier"); it != json.end() && it->is_number_integer()) {
        amplifier = it->get<i32>();
    }
    bool ambient = false;
    if (auto it = json.find("ambient"); it != json.end() && it->is_boolean()) {
        ambient = it->get<bool>();
    }
    bool showParticles = true;
    if (auto it = json.find("show_particles"); it != json.end() && it->is_boolean()) {
        showParticles = it->get<bool>();
    }
    bool showIcon = true;
    if (auto it = json.find("show_icon"); it != json.end() && it->is_boolean()) {
        showIcon = it->get<bool>();
    }
    out.instance.emplace(*type, duration, amplifier, ambient, showParticles, showIcon);
    if (auto it = json.find("probability"); it != json.end() && it->is_number()) {
        out.chance = it->get<f32>();
    }
}

void _writeEffectEntry(const ConsumeEffectEntry::EffectEntry& entry, nlohmann::json& out)
{
    out = nlohmann::json::object();
    if (entry.instance.has_value()) {
        out["id"] = entity::effect::getEffectResourceLocation(entry.instance->type()).toString();
        out["duration"] = entry.instance->duration();
        out["amplifier"] = entry.instance->amplifier();
        out["ambient"] = entry.instance->isAmbient();
        out["show_particles"] = entry.instance->isVisible();
        out["show_icon"] = entry.instance->showIcon();
    }
    if (entry.chance != 1.0f) {
        out["probability"] = entry.chance;
    }
}

} // namespace

std::vector<ConsumeEffectEntry> consumeEffectsFromJson(const nlohmann::json& json)
{
    std::vector<ConsumeEffectEntry> result;
    if (!json.is_array()) {
        return result;
    }
    for (const auto& entry : json) {
        if (!entry.is_object()) {
            continue;
        }
        auto typeIt = entry.find("type");
        if (typeIt == entry.end() || !typeIt->is_string()) {
            continue;
        }
        const std::string type = typeIt->get<std::string>();
        ConsumeEffectEntry parsed{};
        if (type == "minecraft:apply_effects") {
            parsed.kind = ConsumeEffectKind::ApplyEffects;
            auto effectsIt = entry.find("effects");
            if (effectsIt != entry.end() && effectsIt->is_array()) {
                for (const auto& effectJson : *effectsIt) {
                    ConsumeEffectEntry::EffectEntry effectEntry{};
                    _readEffectEntry(effectJson, effectEntry);
                    // vanilla MobEffectInstance.CODEC 对缺少 id 的条目返回错误条目（getEffect() 为空），
                    // 施加阶段跳过，这里保持一致：保留条目但不带实例。
                    parsed.effects.push_back(effectEntry);
                }
            }
            // ApplyStatusEffectsConsumeEffect.probability 作用于整条效果（可选，默认 1.0）
            if (auto it = entry.find("probability"); it != entry.end() && it->is_number()) {
                const f32 probability = it->get<f32>();
                for (auto& effectEntry : parsed.effects) {
                    effectEntry.chance = probability;
                }
            }
        } else if (type == "minecraft:remove_effects") {
            parsed.kind = ConsumeEffectKind::RemoveEffects;
            auto effectsIt = entry.find("effects");
            if (effectsIt != entry.end() && effectsIt->is_array()) {
                for (const auto& idJson : *effectsIt) {
                    if (!idJson.is_string()) {
                        continue;
                    }
                    if (auto effectType = entity::effect::getEffectByResourceLocation(
                            ResourceLocation(_normalizeId(idJson.get<std::string>())));
                        effectType.has_value()) {
                        parsed.removedEffects.push_back(*effectType);
                    }
                }
            }
        } else if (type == "minecraft:clear_all_effects") {
            parsed.kind = ConsumeEffectKind::ClearAllEffects;
        } else if (type == "minecraft:teleport_randomly") {
            parsed.kind = ConsumeEffectKind::TeleportRandomly;
            if (auto it = entry.find("diameter"); it != entry.end() && it->is_number()) {
                parsed.teleportDiameter = it->get<f32>();
            }
        } else if (type == "minecraft:play_sound") {
            parsed.kind = ConsumeEffectKind::PlaySound;
            if (auto it = entry.find("sound"); it != entry.end() && it->is_string()) {
                parsed.soundId = _normalizeId(it->get<std::string>());
            }
        } else {
            // 未知 ConsumeEffect 类型（数据包扩展）：丢弃，与 vanilla 未知 type 解析失败一致
            continue;
        }
        result.push_back(std::move(parsed));
    }
    return result;
}

nlohmann::json consumeEffectsToJson(const std::vector<ConsumeEffectEntry>& effects)
{
    auto array = nlohmann::json::array();
    for (const auto& entry : effects) {
        nlohmann::json out = nlohmann::json::object();
        switch (entry.kind) {
            case ConsumeEffectKind::ApplyEffects: {
                out["type"] = "minecraft:apply_effects";
                auto list = nlohmann::json::array();
                for (const auto& effectEntry : entry.effects) {
                    nlohmann::json item;
                    _writeEffectEntry(effectEntry, item);
                    list.push_back(std::move(item));
                }
                out["effects"] = std::move(list);
                break;
            }
            case ConsumeEffectKind::RemoveEffects: {
                out["type"] = "minecraft:remove_effects";
                auto list = nlohmann::json::array();
                for (const auto& type : entry.removedEffects) {
                    list.push_back(entity::effect::getEffectResourceLocation(type).toString());
                }
                out["effects"] = std::move(list);
                break;
            }
            case ConsumeEffectKind::ClearAllEffects:
                out["type"] = "minecraft:clear_all_effects";
                break;
            case ConsumeEffectKind::TeleportRandomly:
                out["type"] = "minecraft:teleport_randomly";
                out["diameter"] = entry.teleportDiameter;
                break;
            case ConsumeEffectKind::PlaySound:
                out["type"] = "minecraft:play_sound";
                out["sound"] = entry.soundId;
                break;
        }
        array.push_back(std::move(out));
    }
    return array;
}

DeathProtectionPayload totemOfUndyingDeathProtection()
{
    // 对齐 Java 1.21.11 DeathProtection.TOTEM_OF_UNDYING：
    //   先清空全部状态效果，再一次性施加抗性提升 II（15 秒）、伤害吸收 II（10 秒）、抗火 I（40 秒）。
    ConsumeEffectEntry clear{};
    clear.kind = ConsumeEffectKind::ClearAllEffects;

    ConsumeEffectEntry apply{};
    apply.kind = ConsumeEffectKind::ApplyEffects;
    apply.effects.push_back({entity::effect::EffectInstance(entity::effect::EffectType::Regeneration, 900, 1), 1.0f});
    apply.effects.push_back({entity::effect::EffectInstance(entity::effect::EffectType::Absorption, 100, 1), 1.0f});
    apply.effects.push_back({entity::effect::EffectInstance(entity::effect::EffectType::FireResistance, 800, 0), 1.0f});

    DeathProtectionPayload payload;
    payload.effects.push_back(std::move(clear));
    payload.effects.push_back(std::move(apply));
    return payload;
}

} // namespace component
} // namespace item
} // namespace mc