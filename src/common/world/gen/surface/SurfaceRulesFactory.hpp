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
 */

#pragma once

#include "common/world/gen/surface/CaveSurface.hpp"
#include "common/world/gen/surface/SurfaceCondition.hpp"
#include "common/world/gen/surface/SurfaceRule.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::surface {

// ============================================================================
// SurfaceRules 工厂命名空间
// ============================================================================

/**
 * @brief SurfaceRules 工厂命名空间
 *
 * 提供创建条件和规则的便捷工厂方法，以及维度规则树构建器。
 * 对应 MC 1.21 的 SurfaceRules 静态工厂方法。
 */
namespace SurfaceRules {

// ========== 常用条件快捷方式 ==========

/** ON_FLOOR: stoneDepthCheck(0, false, Floor) — 在表面 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> onFloor();

/** UNDER_FLOOR: stoneDepthCheck(0, true, Floor) — 地表下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> underFloor();

/** DEEP_UNDER_FLOOR: stoneDepthCheck(0, true, 6, Floor) — 地表深下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> deepUnderFloor();

/** VERY_DEEP_UNDER_FLOOR: stoneDepthCheck(0, true, 30, Floor) — 地表极深下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> veryDeepUnderFloor();

/** ON_CEILING: stoneDepthCheck(0, false, Ceiling) — 在洞穴顶部 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> onCeiling();

/** UNDER_CEILING: stoneDepthCheck(0, true, Ceiling) — 洞穴顶部下方 */
[[nodiscard]] std::unique_ptr<SurfaceCondition> underCeiling();

// ========== 条件工厂 ==========

[[nodiscard]] std::unique_ptr<SurfaceCondition> stoneDepthCheck(
    i32 offset, bool addSurfaceDepth, i32 secondaryDepthRange, CaveSurface surface);

[[nodiscard]] std::unique_ptr<SurfaceCondition> yBlockCheck(VerticalAnchor anchor, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> yStartCheck(VerticalAnchor anchor, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> waterBlockCheck(i32 offset, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> waterStartCheck(i32 offset, i32 surfaceDepthMultiplier);

[[nodiscard]] std::unique_ptr<SurfaceCondition> isBiome(std::vector<BiomeId> biomes);

[[nodiscard]] std::unique_ptr<SurfaceCondition> notCondition(std::unique_ptr<SurfaceCondition> condition);

[[nodiscard]] std::unique_ptr<SurfaceCondition> noiseCondition(
    std::string noiseName, f64 minThreshold, f64 maxThreshold = 1e30);

[[nodiscard]] std::unique_ptr<SurfaceCondition> verticalGradient(
    std::string randomName, VerticalAnchor trueAtAndBelow, VerticalAnchor falseAtAndAbove);

[[nodiscard]] std::unique_ptr<SurfaceCondition> steep();

[[nodiscard]] std::unique_ptr<SurfaceCondition> temperature();

[[nodiscard]] std::unique_ptr<SurfaceCondition> hole();

[[nodiscard]] std::unique_ptr<SurfaceCondition> abovePreliminarySurface();

// ========== 规则工厂 ==========

[[nodiscard]] std::unique_ptr<SurfaceRule> blockState(const BlockState* state);

[[nodiscard]] std::unique_ptr<SurfaceRule> ifTrue(
    std::unique_ptr<SurfaceCondition> condition, std::unique_ptr<SurfaceRule> thenRule);

[[nodiscard]] std::unique_ptr<SurfaceRule> sequence(std::vector<std::unique_ptr<SurfaceRule>> rules);

/** 变参 sequence：直接传入 unique_ptr 规则，避免 initializer_list 复制问题 */
template <typename... Rules>
[[nodiscard]] std::unique_ptr<SurfaceRule> sequence(Rules... rules)
{
    std::vector<std::unique_ptr<SurfaceRule>> v;
    v.reserve(sizeof...(rules));
    (v.push_back(std::move(rules)), ...);
    return sequence(std::move(v));
}

[[nodiscard]] std::unique_ptr<SurfaceRule> bandlands();

// ========== 维度规则树 ==========

/** 创建主世界表面规则（MC 1.21 SurfaceRuleData.overworld()） */
[[nodiscard]] std::unique_ptr<SurfaceRule> overworld();

/** 创建下界表面规则 */
[[nodiscard]] std::unique_ptr<SurfaceRule> nether();

/** 创建末地表面规则 */
[[nodiscard]] std::unique_ptr<SurfaceRule> end();

} // namespace SurfaceRules

} // namespace mc::world::gen::surface
