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

#include "BannerPatternLayers.hpp"

#include "common/world/blockentity/interactive/BannerPattern.hpp"
#include <array>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace mc::item::component {

namespace {

/// 16 种染料颜色，索引即 `banner_patterns` 的 Color 序号（与 NBT 侧 `color` 字符串一一对应）
constexpr std::array<std::string_view, 16> COLOR_NAMES{"white",
    "orange",
    "magenta",
    "light_blue",
    "yellow",
    "lime",
    "pink",
    "gray",
    "light_gray",
    "cyan",
    "purple",
    "blue",
    "brown",
    "green",
    "red",
    "black"};

/// 图案 ID（`minecraft:stripe_bottom`）→ 枚举；只接受 minecraft 命名空间
bool _patternFromId(const std::string& id, blockentity::BannerPatternType& out)
{
    constexpr std::string_view PREFIX = "minecraft:";
    if (id.rfind(PREFIX, 0) != 0) {
        return false;
    }
    const std::string name = id.substr(PREFIX.size());
    for (i32 i = 0; i < static_cast<i32>(blockentity::BannerPatternType::Count); ++i) {
        const auto type = static_cast<blockentity::BannerPatternType>(i);
        if (blockentity::BannerPatterns::getFileName(type) == name) {
            out = type;
            return true;
        }
    }
    return false;
}

bool _colorFromName(const std::string& name, i32& out)
{
    for (std::size_t i = 0; i < COLOR_NAMES.size(); ++i) {
        if (COLOR_NAMES[i] == name) {
            out = static_cast<i32>(i);
            return true;
        }
    }
    return false;
}

} // namespace

bool readBannerPatternsPayload(const nlohmann::json& json, nlohmann::json& out)
{
    if (!json.is_array()) {
        return false;
    }

    nlohmann::json payload = nlohmann::json::array();
    for (const auto& entry : json) {
        if (!entry.is_object() || !entry.contains("pattern") || !entry.contains("color") ||
            !entry["color"].is_string()) {
            return false;
        }

        const auto& pattern = entry.at("pattern");
        nlohmann::json layer = nlohmann::json::object();
        if (pattern.is_object()) {
            // 直挂图案（数据包自定义图案）：原样保留资源路径与翻译键，Pattern 固定占位 "b"——
            // 与 NBT 读取路径（DataComponentPayloadCodec::nbtToComponentPayload）保持一致，
            // 有 DirectPattern 时 Pattern 不被任何消费方使用。
            if (!pattern.contains("asset_id") || !pattern["asset_id"].is_string() ||
                !pattern.contains("translation_key") || !pattern["translation_key"].is_string()) {
                return false;
            }
            layer["DirectPattern"] = pattern;
            layer["Pattern"] = "b";
        } else if (pattern.is_string()) {
            blockentity::BannerPatternType type = blockentity::BannerPatternType::Base;
            // 原版对单个无法识别的层是「丢弃该层」而非整条失败，这里同样跳过
            if (!_patternFromId(pattern.get<std::string>(), type)) {
                continue;
            }
            layer["Pattern"] = blockentity::BannerPatterns::getHashName(type);
        } else {
            return false;
        }

        i32 color = 0;
        if (!_colorFromName(entry.at("color").get<std::string>(), color)) {
            continue;
        }
        layer["Color"] = color;
        payload.push_back(std::move(layer));
    }

    out = std::move(payload);
    return true;
}

} // namespace mc::item::component
