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
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief FoliagePlacer JSON 解析器
 *
 * 解析树木配置 foliage_placer 字段，按 JSON "type" 分派到对应树叶放置器子类。
 * 通用字段：radius / offset（裸整数，→ FeatureSpread::fixed）；多数再加 height（裸整数）。
 *   {"type":"minecraft:blob_foliage_placer","radius":2,"offset":0,"height":3}
 * 特殊字段：
 *   cherry_foliage_placer：4 个概率参数(wide_bottom_layer_hole_chance 等)
 *   random_spread_foliage_placer：foliage_height(IntProvider) + leaf_placement_attempts
 */
namespace FoliagePlacerParser {

[[nodiscard]] Result<std::unique_ptr<FoliagePlacer>> parse(const nlohmann::json& placerObj);

} // namespace FoliagePlacerParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
