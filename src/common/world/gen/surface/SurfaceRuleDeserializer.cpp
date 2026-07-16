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

#include "common/world/gen/surface/SurfaceRuleDeserializer.hpp"

#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/gen/feature/parser/BlockStateParser.hpp"
#include "common/world/gen/surface/CaveSurface.hpp"
#include "common/world/gen/surface/SurfaceCondition.hpp"
#include "common/world/gen/surface/SurfaceRule.hpp"
#include "common/world/gen/surface/SurfaceRulesFactory.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <optional>
#include <string>
#include <string_view>

namespace mc::world::gen::surface {

namespace {

using json = nlohmann::json;

/// 剥离 "minecraft:" 命名空间前缀
std::string stripNamespace(std::string_view s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return std::string(s.substr(prefix.size()));
    }
    return std::string(s);
}

/// 读取必填 double 字段
Result<f64> readDouble(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_number()) {
        return Error(ErrorCode::InvalidData, "surface_rule: missing number field '" + std::string(field) + "'");
    }
    return j[field].get<f64>();
}

/// 读取必填整数字段
Result<i32> readInt(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "surface_rule: missing integer field '" + std::string(field) + "'");
    }
    return j[field].get<i32>();
}

/// 读取必填 bool 字段
Result<bool> readBool(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_boolean()) {
        return Error(ErrorCode::InvalidData, "surface_rule: missing boolean field '" + std::string(field) + "'");
    }
    return j[field].get<bool>();
}

/// 读取必填字符串字段
Result<std::string> readString(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_string()) {
        return Error(ErrorCode::InvalidData, "surface_rule: missing string field '" + std::string(field) + "'");
    }
    return j[field].get<std::string>();
}

/// 解析锚点对象（单键：absolute/above_bottom/below_top）→ VerticalAnchor
Result<VerticalAnchor> parseAnchor(const json& anchorObj)
{
    if (!anchorObj.is_object() || anchorObj.empty()) {
        return Error(ErrorCode::InvalidData, "surface_rule: anchor must be a non-empty object");
    }
    const auto it = anchorObj.begin();
    const std::string key = it.key();
    const json& val = it.value();
    if (!val.is_number_integer()) {
        return Error(ErrorCode::InvalidData, "surface_rule: anchor value must be an integer");
    }
    const i32 v = val.get<i32>();
    if (key == "absolute") {
        return VerticalAnchor::absolute(v);
    }
    if (key == "above_bottom") {
        return VerticalAnchor::aboveBottom(v);
    }
    if (key == "below_top") {
        return VerticalAnchor::belowTop(v);
    }
    return Error(ErrorCode::InvalidData, "surface_rule: unknown anchor type '" + key + "'");
}

/// "floor"|"ceiling" → CaveSurface
Result<CaveSurface> parseSurfaceType(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_string()) {
        return Error(
            ErrorCode::InvalidData, "surface_rule stone_depth: missing string field '" + std::string(field) + "'");
    }
    const std::string s = j[field].get<std::string>();
    if (s == "floor") {
        return CaveSurface::Floor;
    }
    if (s == "ceiling") {
        return CaveSurface::Ceiling;
    }
    return Error(ErrorCode::InvalidData, "surface_rule stone_depth: surface_type must be 'floor' or 'ceiling'");
}

/// biome RL 字符串 → BiomeId（镜像 BiomeTagLoader::resolveBiomeId：剥 minecraft: 后按 name 匹配注册表）
std::optional<BiomeId> resolveBiomeId(const std::string& biomeName)
{
    const std::string name = stripNamespace(biomeName);
    const auto& allBiomes = world::biome::BiomeRegistry::instance().allBiomes();
    for (const auto& biome : allBiomes) {
        if (biome.name() == name) {
            return biome.id();
        }
    }
    return std::nullopt;
}

// 前向声明：规则与条件递归
[[nodiscard]] Result<std::unique_ptr<SurfaceRule>> parseRule(const json& node);
[[nodiscard]] Result<std::unique_ptr<SurfaceCondition>> parseCondition(const json& node);

/// 解析条件节点（type 分发）→ SurfaceCondition
Result<std::unique_ptr<SurfaceCondition>> parseCondition(const json& node)
{
    if (!node.is_object() || !node.contains("type") || !node["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "surface_rule condition must be an object with 'type'");
    }
    const std::string type = stripNamespace(node["type"].get<std::string>());

    if (type == "vertical_gradient") {
        auto randomName = readString(node, "random_name");
        if (randomName.failed()) {
            return randomName.error();
        }
        auto trueAt = parseAnchor(node["true_at_and_below"]);
        if (trueAt.failed()) {
            return trueAt.error();
        }
        auto falseAt = parseAnchor(node["false_at_and_above"]);
        if (falseAt.failed()) {
            return falseAt.error();
        }
        return SurfaceRules::verticalGradient(randomName.value(), trueAt.value(), falseAt.value());
    }
    if (type == "above_preliminary_surface") {
        return SurfaceRules::abovePreliminarySurface();
    }
    if (type == "stone_depth") {
        auto offset = readInt(node, "offset");
        if (offset.failed()) {
            return offset.error();
        }
        auto addSurfaceDepth = readBool(node, "add_surface_depth");
        if (addSurfaceDepth.failed()) {
            return addSurfaceDepth.error();
        }
        auto secondaryDepthRange = readInt(node, "secondary_depth_range");
        if (secondaryDepthRange.failed()) {
            return secondaryDepthRange.error();
        }
        auto surface = parseSurfaceType(node, "surface_type");
        if (surface.failed()) {
            return surface.error();
        }
        return SurfaceRules::stoneDepthCheck(
            offset.value(), addSurfaceDepth.value(), secondaryDepthRange.value(), surface.value());
    }
    if (type == "biome") {
        if (!node.contains("biome_is") || !node["biome_is"].is_array()) {
            return Error(ErrorCode::InvalidData, "surface_rule biome condition: missing 'biome_is' array");
        }
        std::vector<BiomeId> biomes;
        biomes.reserve(node["biome_is"].size());
        for (const auto& entry : node["biome_is"]) {
            if (!entry.is_string()) {
                return Error(ErrorCode::InvalidData, "surface_rule biome_is entries must be strings");
            }
            const std::string rl = entry.get<std::string>();
            auto id = resolveBiomeId(rl);
            if (!id.has_value()) {
                spdlog::warn("surface_rule biome condition: biome '{}' not found in registry, skipping", rl);
                continue;
            }
            biomes.push_back(*id);
        }
        return SurfaceRules::isBiome(std::move(biomes));
    }
    if (type == "y_above") {
        auto anchor = parseAnchor(node["anchor"]);
        if (anchor.failed()) {
            return anchor.error();
        }
        auto mult = readInt(node, "surface_depth_multiplier");
        if (mult.failed()) {
            return mult.error();
        }
        auto addStoneDepth = readBool(node, "add_stone_depth");
        if (addStoneDepth.failed()) {
            return addStoneDepth.error();
        }
        // 直接构造：工厂 yBlockCheck 硬编码 addStoneDepth=false，丢失 JSON 的 add_stone_depth
        return std::unique_ptr<SurfaceCondition>(
            std::make_unique<YCondition>(anchor.value(), mult.value(), addStoneDepth.value()));
    }
    if (type == "noise_threshold") {
        auto noiseName = readString(node, "noise");
        if (noiseName.failed()) {
            return noiseName.error();
        }
        auto minThreshold = readDouble(node, "min_threshold");
        if (minThreshold.failed()) {
            return minThreshold.error();
        }
        auto maxThreshold = readDouble(node, "max_threshold");
        if (maxThreshold.failed()) {
            return maxThreshold.error();
        }
        return SurfaceRules::noiseCondition(noiseName.value(), minThreshold.value(), maxThreshold.value());
    }
    if (type == "water") {
        auto offset = readInt(node, "offset");
        if (offset.failed()) {
            return offset.error();
        }
        auto mult = readInt(node, "surface_depth_multiplier");
        if (mult.failed()) {
            return mult.error();
        }
        auto addStoneDepth = readBool(node, "add_stone_depth");
        if (addStoneDepth.failed()) {
            return addStoneDepth.error();
        }
        // 直接构造：工厂 waterBlockCheck 硬编码 addStoneDepth=false，丢失 JSON 的 add_stone_depth
        return std::unique_ptr<SurfaceCondition>(
            std::make_unique<WaterCondition>(offset.value(), mult.value(), addStoneDepth.value()));
    }
    if (type == "not") {
        if (!node.contains("invert")) {
            return Error(ErrorCode::InvalidData, "surface_rule not condition: missing 'invert'");
        }
        auto inner = parseCondition(node["invert"]);
        if (inner.failed()) {
            return inner.error();
        }
        return SurfaceRules::notCondition(inner.value());
    }
    if (type == "hole") {
        return SurfaceRules::hole();
    }
    if (type == "temperature") {
        return SurfaceRules::temperature();
    }
    if (type == "steep") {
        return SurfaceRules::steep();
    }
    return Error(ErrorCode::NotFound, "surface_rule: unregistered condition type '" + type + "'");
}

/// 解析规则节点（type 分发）→ SurfaceRule
Result<std::unique_ptr<SurfaceRule>> parseRule(const json& node)
{
    if (!node.is_object() || !node.contains("type") || !node["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "surface_rule rule must be an object with 'type'");
    }
    const std::string type = stripNamespace(node["type"].get<std::string>());

    if (type == "sequence") {
        if (!node.contains("sequence") || !node["sequence"].is_array()) {
            return Error(ErrorCode::InvalidData, "surface_rule sequence: missing 'sequence' array");
        }
        std::vector<std::unique_ptr<SurfaceRule>> rules;
        rules.reserve(node["sequence"].size());
        for (const auto& entry : node["sequence"]) {
            auto child = parseRule(entry);
            if (child.failed()) {
                return child.error();
            }
            rules.push_back(child.value());
        }
        return SurfaceRules::sequence(std::move(rules));
    }
    if (type == "condition") {
        if (!node.contains("if_true")) {
            return Error(ErrorCode::InvalidData, "surface_rule condition: missing 'if_true'");
        }
        if (!node.contains("then_run")) {
            return Error(ErrorCode::InvalidData, "surface_rule condition: missing 'then_run'");
        }
        auto cond = parseCondition(node["if_true"]);
        if (cond.failed()) {
            return cond.error();
        }
        auto thenRule = parseRule(node["then_run"]);
        if (thenRule.failed()) {
            return thenRule.error();
        }
        return SurfaceRules::ifTrue(cond.value(), thenRule.value());
    }
    if (type == "block") {
        if (!node.contains("result_state")) {
            return Error(ErrorCode::InvalidData, "surface_rule block: missing 'result_state'");
        }
        auto state = feature::parser::BlockStateParser::parse(node["result_state"]);
        if (state.failed()) {
            return state.error();
        }
        return SurfaceRules::blockState(state.value());
    }
    if (type == "bandlands") {
        return SurfaceRules::bandlands();
    }
    return Error(ErrorCode::NotFound, "surface_rule: unregistered rule type '" + type + "'");
}

} // namespace

Result<std::unique_ptr<SurfaceRule>> SurfaceRuleDeserializer::fromJson(const json& root)
{
    return parseRule(root);
}

} // namespace mc::world::gen::surface
