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

#include "common/core/Types.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/surface/CaveSurface.hpp"
#include "common/world/gen/surface/SurfaceCondition.hpp"
#include "common/world/gen/surface/SurfaceRule.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::world::gen::surface {

// ============================================================================
// SurfaceRules 工厂命名空间
// ============================================================================

/**
 * @brief SurfaceRules 工厂命名空间
 *
 * 提供创建条件和规则的便捷工厂方法。
 * 对应 MC 1.21 的 SurfaceRules 静态工厂方法。
 */
namespace SurfaceRules {

// ========== 条件工厂 ==========

[[nodiscard]] std::unique_ptr<SurfaceCondition> stoneDepthCheck(
    i32 offset, bool addSurfaceDepth, i32 secondaryDepthRange, CaveSurface surface);

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

} // namespace SurfaceRules

} // namespace mc::world::gen::surface
