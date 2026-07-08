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
#include "common/world/gen/feature/Feature.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief RuleTest JSON 解析器
 *
 * 解析矿石配置 targets[].target 中的方块匹配规则，构造 mc::RuleTest 体系
 * （Feature.hpp 中定义的层次，OreFeature/OreTarget 实际持有的类型）。
 * JSON 用 "predicate_type" 字段分派：
 *   {"predicate_type":"minecraft:always_true"}
 *   {"predicate_type":"minecraft:block_match","block":"minecraft:stone"}
 *   {"predicate_type":"minecraft:random_block_match","block":"...","probability":0.5}
 *   {"predicate_type":"minecraft:tag_match","tag":"minecraft:stone_ore_replaceables"}
 *   {"predicate_type":"minecraft:block_state_match","block_state":{"Name":...,"Properties":{...}}}
 *   {"predicate_type":"minecraft:random_block_state_match","block_state":{...},"probability":0.5}
 */
namespace RuleTestParser {

[[nodiscard]] Result<std::unique_ptr<RuleTest>> parse(const nlohmann::json& predicateObj);

} // namespace RuleTestParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
