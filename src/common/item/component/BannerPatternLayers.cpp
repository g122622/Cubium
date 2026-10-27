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
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::item::component {

namespace {

/// 16 种染料颜色，索引即 `banner_patterns` 的 color 排序（与 NBT 侧 `color` 字符串一一对应）
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
bool _patternFromId(const std::string& id, int& out)
{
    constexpr std::string_view PREFIX = "minecraft:";
    if (id.rfind(PREFIX, 0) != 0) {
        return false;
    }
    const std::string name = id.substr(PREFIX.size());
    for (int i = 0; i < static_cast<int>(blockentity::BannerPatternType::Count); ++i) {
        if (blockentity::BannerPatterns::getFileName(static_cast<blockentity::BannerPatternType>(i)) == name) {
            out = i;
            return true;
        }
    }
    return false;
}

bool _colorFromName(const std::string& name, int& out)
{
    for (std::size_t i = 0; i < COLOR_NAMES.size(); ++i) {
        if (COLOR_NAMES[i] == name) {
            out = static_cast<int>(i);
            return true;
        }
    }
    return false;
}

} // namespace

bool readBannerPatternLayers(const nlohmann::json& json, std::vector<BannerPatternLayer>& out)
{
    if (!json.is_array()) {
        return false;
    }
    std::vector<BannerPatternLayer> layers;
    layers.reserve(json.size());
    for (const auto& entry : json) {
        if (!entry.is_object() || !entry.contains("pattern") || !entry.contains("color") ||
            !entry["pattern"].is_string() || !entry["color"].is_string()) {
            return false;
        }
        BannerPatternLayer layer;
        // 原版对单个无法识别的层是「丢弃该层」而非整条失败，这里同样跳过
        if (!_patternFromId(entry["pattern"].get<std::string>(), layer.pattern)) {
            continue;
        }
        if (!_colorFromName(entry["color"].get<std::string>(), layer.color)) {
            continue;
        }
        layers.push_back(layer);
    }
    out = std::move(layers);
    return true;
}

nlohmann::json writeBannerPatternLayers(const std::vector<BannerPatternLayer>& layers)
{
    nlohmann::json json = nlohmann::json::array();
    for (const auto& layer : layers) {
        if (layer.pattern < 0 || layer.pattern >= static_cast<int>(blockentity::BannerPatternType::Count) ||
            layer.color < 0 || layer.color >= static_cast<int>(COLOR_NAMES.size())) {
            continue;
        }
        json.push_back(nlohmann::json{
            {"pattern",
                "minecraft:" +
                    blockentity::BannerPatterns::getFileName(
                        static_cast<blockentity::BannerPatternType>(layer.pattern))},
            {"color", std::string(COLOR_NAMES[static_cast<std::size_t>(layer.color)])},
        });
    }
    return json;
}

} // namespace mc::item::component
