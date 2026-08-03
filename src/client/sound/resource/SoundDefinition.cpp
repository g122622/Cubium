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

#include "client/sound/resource/SoundDefinition.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::sound {

// ============================================================================
// SoundDefinition
// ============================================================================

SoundDefinition::SoundDefinition(std::string_view path)
    : location(ResourceLocation::parse(path))
    , type(SoundType::File)
{}

SoundDefinition::SoundDefinition(const ResourceLocation& loc)
    : location(loc)
    , type(SoundType::File)
{}

ResourceLocation SoundDefinition::toOggLocation() const
{
    // sounds.json 中的路径（如 "dig/stone1"）是相对于 sounds/ 目录的
    // SoundLoader::toAudioPath() 会负责添加 "sounds/" 前缀和 ".ogg" 后缀
    // 这里直接返回原始路径，不做转换
    return location;
}

Result<SoundDefinition> SoundDefinition::parse(const nlohmann::json& json, std::string_view namespace_)
{
    SoundDefinition def;

    if (json.is_string()) {
        // 简单字符串格式: "dig/stone1"
        std::string path = json.get<std::string>();

        // 检查是否是事件引用（以 # 开头）
        if (!path.empty() && path[0] == '#') {
            // 事件引用: "#block.stone.break"
            def.type = SoundType::Event;
            def.location = ResourceLocation::parse(path.substr(1));
        } else {
            // 文件引用
            def.type = SoundType::File;
            // 如果没有命名空间，添加默认命名空间
            if (path.find(':') == std::string::npos) {
                def.location = ResourceLocation(std::string(namespace_), path);
            } else {
                def.location = ResourceLocation::parse(path);
            }
        }
    } else if (json.is_object()) {
        // 对象格式: {"name": "dig/stone1", "volume": 0.8, ...}
        if (!json.contains("name")) {
            return Error(ErrorCode::InvalidData, "Sound definition missing 'name' field");
        }

        std::string name = json["name"].get<std::string>();

        // 检查是否是事件引用
        if (!name.empty() && name[0] == '#') {
            def.type = SoundType::Event;
            def.location = ResourceLocation::parse(name.substr(1));
        } else {
            def.type = SoundType::File;
            // 如果没有命名空间，添加默认命名空间
            if (name.find(':') == std::string::npos) {
                def.location = ResourceLocation(std::string(namespace_), name);
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
                def.attenuationDistance = static_cast<u32>(std::max(0, json["attenuation_distance"].get<i32>()));
            }
        }

        // 支持 type 字段（事件引用的另一种方式）
        if (json.contains("type")) {
            std::string typeStr = json["type"].get<std::string>();
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
{}

SoundEventDefinition::SoundEventDefinition(std::string_view eventId)
    : location(ResourceLocation::parse(eventId))
{}

Result<SoundEventDefinition> SoundEventDefinition::parse(
    std::string_view eventId, const nlohmann::json& json, std::string_view namespace_)
{
    SoundEventDefinition def{std::string(eventId)};

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
            def.subtitle = json["subtitle"].get<std::string>();
        }
    }

    // 解析 sounds 数组
    if (!json.contains("sounds")) {
        return Error(ErrorCode::InvalidData, "Sound event definition missing 'sounds' array");
    }

    const auto& soundsJson = json["sounds"];
    if (!soundsJson.is_array()) {
        return Error(ErrorCode::InvalidData, "'sounds' must be an array");
    }

    const auto originalSoundCount = soundsJson.size();
    for (const auto& soundJson : soundsJson) {
        auto result = SoundDefinition::parse(soundJson, namespace_);
        if (result.success()) {
            def.sounds.push_back(std::move(result.value()));
        } else {
            spdlog::warn(
                "Sound event '{}' skipped invalid sound entry: {}", std::string(eventId), result.error().message());
        }
    }

    if (def.sounds.empty()) {
        if (originalSoundCount == 0) {
            spdlog::info("Sound event '{}' is intentionally empty", std::string(eventId));
        } else {
            spdlog::warn("Sound event '{}' has no valid sound entries after parsing", std::string(eventId));
        }
    }

    return def;
}

u32 SoundEventDefinition::totalWeight() const noexcept
{
    u32 total = 0;
    for (const auto& sound : sounds) {
        total += sound.weight;
    }
    return total;
}

const SoundDefinition* SoundEventDefinition::selectSound(mc::math::Random& rng) const noexcept
{
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
