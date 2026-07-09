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
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief BlockPredicate JSON 解析器
 *
 * 解析数据包中的方块谓词对象（allowed_placement / block_predicate_filter 等），
 * 按 JSON "type" 字段分派到 predicate::BlockPredicate 子类。
 * 支持：
 *   {"type":"minecraft:matching_blocks","blocks":"minecraft:air" | ["minecraft:air",...],"offset":[x,y,z]?}
 *   {"type":"minecraft:matching_fluids","fluids":"minecraft:water" | ["minecraft:water",...] |
 * "#tag","offset":[x,y,z]?}
 *   {"type":"minecraft:matching_block_tag","tag":"#minecraft:xxx"}
 *   {"type":"minecraft:all_of","predicates":[...]}
 *   {"type":"minecraft:any_of","predicates":[...]}
 *   {"type":"minecraft:not","predicate":{...}}
 *   {"type":"minecraft:would_survive","state":{...}}
 *   {"type":"minecraft:replaceable"}
 *   {"type":"minecraft:solid"}
 *   {"type":"minecraft:has_sturdy_face","direction":"up"}
 *   {"type":"minecraft:true"}
 */
namespace BlockPredicateParser {

[[nodiscard]] Result<std::unique_ptr<predicate::BlockPredicate>> parse(const nlohmann::json& predicateObj);

} // namespace BlockPredicateParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
