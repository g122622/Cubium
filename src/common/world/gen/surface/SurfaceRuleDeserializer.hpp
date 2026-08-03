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

#pragma once

#include "common/core/Result.hpp"

#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::world::gen::surface {

class SurfaceRule;

/**
 * @brief surface_rule JSON → SurfaceRule 树反序列化器（MC 1.21.11）
 *
 * noise_settings JSON 的 surface_rule 字段是多态规则树。本反序列化器按 type 字段分发：
 *
 * 规则节点（rule，返回 SurfaceRule）：
 * - minecraft:sequence  → SequenceRule（sequence: [rule...]）
 * - minecraft:condition → IfTrueRule（if_true: condition, then_run: rule）
 * - minecraft:block     → BlockRule（result_state: {Name, Properties}）
 * - minecraft:bandlands → BandlandsRule（无参）
 *
 * 条件节点（condition，返回 SurfaceCondition）：
 * - minecraft:vertical_gradient          → VerticalGradientCondition
 *   (random_name, true_at_and_below: anchor, false_at_and_above: anchor)
 * - minecraft:above_preliminary_surface  → AbovePreliminarySurfaceCondition（无参）
 * - minecraft:stone_depth  → StoneDepthCondition
 *   (offset, add_surface_depth, secondary_depth_range, surface_type: "floor"|"ceiling")
 * - minecraft:biome        → BiomeCondition（biome_is: [RL...]）
 * - minecraft:y_above      → YCondition（anchor, surface_depth_multiplier, add_stone_depth）
 * - minecraft:noise_threshold → NoiseThresholdCondition（noise, min_threshold, max_threshold）
 * - minecraft:water        → WaterCondition（offset, surface_depth_multiplier, add_stone_depth）
 * - minecraft:not          → NotCondition（invert: condition）
 * - minecraft:hole         → HoleCondition（无参）
 * - minecraft:temperature  → TemperatureCondition（无参）
 * - minecraft:steep        → SteepCondition（无参）
 *
 * 锚点对象（anchor，单键）：
 * - {"absolute": <int>}
 * - {"above_bottom": <int>}
 * - {"below_top": <int>}
 *
 * 方块状态对象（result_state / default_block / default_fluid）：{Name: RL, Properties?: {str:str}}
 * 复用 BlockStateParser::parse。
 */
class SurfaceRuleDeserializer {
public:
    /** 解析 surface_rule 根节点 JSON 为 SurfaceRule 树。 */
    [[nodiscard]] static Result<std::unique_ptr<SurfaceRule>> fromJson(const nlohmann::json& root);
};

} // namespace mc::world::gen::surface
