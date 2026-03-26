#include "client/sound/resource/SoundDefinition.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <algorithm>

namespace mc::client::sound {

// ============================================================================
// SoundDefinition
// ============================================================================

SoundDefinition::SoundDefinition(StringView path)
    : location(ResourceLocation::parse(path))
    , type(SoundType::File)
{
}

SoundDefinition::SoundDefinition(const ResourceLocation& loc)
    : location(loc)
    , type(SoundType::File)
{
}

ResourceLocation SoundDefinition::toOggLocation() const {
    // 将 "minecraft:dig/stone1" 转换为 "minecraft:sounds/dig/stone1.ogg"
    String oggPath = "sounds/" + location.path() + ".ogg";
    return ResourceLocation(location.namespace_(), std::move(oggPath));
}

Result<SoundDefinition> SoundDefinition::parse(
    const nlohmann::json& json,
    StringView namespace_
) {
    SoundDefinition def;

    if (json.is_string()) {
        // 简单字符串格式: "dig/stone1"
        String path = json.get<String>();

        // 检查是否是事件引用（以 # 开头）
        if (!path.empty() && path[0] == '#') {
            // 事件引用: "#block.stone.break"
            def.type = SoundType::Event;
            def.location = ResourceLocation::parse(path.substr(1));
        } else {
            // 文件引用
            def.type = SoundType::File;
            // 如果没有命名空间，添加默认命名空间
            if (path.find(':') == String::npos) {
                def.location = ResourceLocation(String(namespace_), path);
            } else {
                def.location = ResourceLocation::parse(path);
            }
        }
    } else if (json.is_object()) {
        // 对象格式: {"name": "dig/stone1", "volume": 0.8, ...}
        if (!json.contains("name")) {
            return Error(ErrorCode::InvalidData, "Sound definition missing 'name' field");
        }

        String name = json["name"].get<String>();

        // 检查是否是事件引用
        if (!name.empty() && name[0] == '#') {
            def.type = SoundType::Event;
            def.location = ResourceLocation::parse(name.substr(1));
        } else {
            def.type = SoundType::File;
            // 如果没有命名空间，添加默认命名空间
            if (name.find(':') == String::npos) {
                def.location = ResourceLocation(String(namespace_), name);
            } else {
                def.location = ResourceLocation::parse(name);
            }
        }

        // 解析可选字段
        if (json.contains("volume")) {
            if (json["volume"].is_number()) {
                def.volume = json["volume"].get<f32>();
                // 音量不能为负
                def.volume = std::max(0.0f, def.volume);
            }
        }

        if (json.contains("pitch")) {
            if (json["pitch"].is_number()) {
                def.pitch = json["pitch"].get<f32>();
                // 音调范围限制在 0.5-2.0
                def.pitch = std::clamp(def.pitch, 0.5f, 2.0f);
            }
        }

        if (json.contains("weight")) {
            if (json["weight"].is_number_unsigned()) {
                def.weight = json["weight"].get<u32>();
                // 权重至少为 1
                def.weight = std::max(1u, def.weight);
            } else if (json["weight"].is_number_integer()) {
                auto val = json["weight"].get<i32>();
                def.weight = static_cast<u32>(std::max(1, val));
            }
        }

        if (json.contains("stream")) {
            if (json["stream"].is_boolean()) {
                def.stream = json["stream"].get<bool>();
            }
        }

        if (json.contains("preload")) {
            if (json["preload"].is_boolean()) {
                def.preload = json["preload"].get<bool>();
            }
        }

        if (json.contains("attenuation_distance")) {
            if (json["attenuation_distance"].is_number_unsigned()) {
                def.attenuationDistance = json["attenuation_distance"].get<u32>();
            } else if (json["attenuation_distance"].is_number_integer()) {
                def.attenuationDistance = static_cast<u32>(
                    std::max(0, json["attenuation_distance"].get<i32>())
                );
            }
        }

        // 支持 type 字段（事件引用的另一种方式）
        if (json.contains("type")) {
            String typeStr = json["type"].get<String>();
            if (typeStr == "event") {
                def.type = SoundType::Event;
            } else if (typeStr == "file") {
                def.type = SoundType::File;
            }
        }
    } else {
        return Error(ErrorCode::InvalidData, "Sound definition must be string or object");
    }

    return def;
}

// ============================================================================
// SoundEventDefinition
// ============================================================================

SoundEventDefinition::SoundEventDefinition(ResourceLocation location)
    : location(std::move(location))
{
}

SoundEventDefinition::SoundEventDefinition(StringView eventId)
    : location(ResourceLocation::parse(eventId))
{
}

Result<SoundEventDefinition> SoundEventDefinition::parse(
    StringView eventId,
    const nlohmann::json& json,
    StringView namespace_
) {
    SoundEventDefinition def(String(eventId));

    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Sound event definition must be an object");
    }

    // 解析 replace 字段
    if (json.contains("replace")) {
        if (json["replace"].is_boolean()) {
            def.replace = json["replace"].get<bool>();
        }
    }

    // 解析 subtitle 字段
    if (json.contains("subtitle")) {
        if (json["subtitle"].is_string()) {
            def.subtitle = json["subtitle"].get<String>();
        }
    }

    // 解析 sounds 数组
    if (!json.contains("sounds")) {
        return Error(ErrorCode::InvalidData,
                      "Sound event definition missing 'sounds' array");
    }

    const auto& soundsJson = json["sounds"];
    if (!soundsJson.is_array()) {
        return Error(ErrorCode::InvalidData, "'sounds' must be an array");
    }

    for (const auto& soundJson : soundsJson) {
        auto result = SoundDefinition::parse(soundJson, namespace_);
        if (result.success()) {
            def.sounds.push_back(std::move(result.value()));
        } else {
            // 记录警告但继续解析其他声音
            // TODO: 使用日志系统
        }
    }

    // 至少需要一个有效的声音
    if (def.sounds.empty()) {
        return Error(ErrorCode::InvalidData,
                      "Sound event must have at least one valid sound");
    }

    return def;
}

u32 SoundEventDefinition::totalWeight() const noexcept {
    u32 total = 0;
    for (const auto& sound : sounds) {
        total += sound.weight;
    }
    return total;
}

const SoundDefinition* SoundEventDefinition::selectSound(
    mc::math::Random& rng
) const noexcept {
    if (sounds.empty()) {
        return nullptr;
    }

    // 如果只有一个声音，直接返回
    if (sounds.size() == 1) {
        return &sounds[0];
    }

    // 计算总权重
    u32 total = totalWeight();
    if (total == 0) {
        return &sounds[0];
    }

    // 根据权重随机选择
    u32 random = rng.nextInt(total);
    u32 accumulated = 0;

    for (const auto& sound : sounds) {
        accumulated += sound.weight;
        if (random < accumulated) {
            return &sound;
        }
    }

    // 不应该到达这里，但返回最后一个作为后备
    return &sounds.back();
}

} // namespace mc::client::sound
