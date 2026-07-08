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
#include "common/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief TrunkPlacer JSON 解析器
 *
 * 解析树木配置 trunk_placer 字段，按 JSON "type" 分派到对应树干放置器子类。
 * 通用字段：base_height / height_rand_a / height_rand_b（→ TrunkPlacer 三参构造）。
 *   {"type":"minecraft:straight_trunk_placer","base_height":4,"height_rand_a":2,"height_rand_b":1}
 * 弯曲/樱花/向上分支有额外字段：
 *   bending_trunk_placer：min_height_for_leaves + bend_length(IntProvider)
 *   cherry_trunk_placer：8 个分支参数(branch_count/horizontal_length/start_offset/end_offset 各 min/max)
 *   upwards_branching_trunk_placer：extra_branch_steps(IntProvider) + place_branch_per_log_probability(float)
 *                                   + extra_branch_length(IntProvider) + can_grow_through("#tag")
 */
namespace TrunkPlacerParser {

[[nodiscard]] Result<std::unique_ptr<TrunkPlacer>> parse(const nlohmann::json& placerObj);

} // namespace TrunkPlacerParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
