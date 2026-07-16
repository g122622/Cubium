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
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::world::gen::feature::parser {

/**
 * @brief 方块状态提供者 JSON 解析器
 *
 * 解析数据包中的 BlockStateProvider 对象，返回多态基类指针：
 *   {"type":"minecraft:simple_state_provider","state":{"Name":...,"Properties":{...}}}
 *   {"type":"minecraft:weighted_state_provider","entries":[{"data":{...},"weight":N}, ...]}
 *   {"type":"minecraft:rule_based_state_provider",
 *    "fallback":<BlockStateProvider>, "rules":[{"if_true":<BlockPredicate>, "then":<BlockStateProvider>}, ...]}
 *   {"type":"minecraft:rotated_block_provider","state":{...}}（取 Block，随机 axis）
 *   {"type":"minecraft:noise_threshold_provider","seed":L,"noise":{...},"scale":F,
 *    "threshold":F,"high_chance":F,"default_state":{...},"low_states":[...],"high_states":[...]}
 *   {"type":"minecraft:noise_provider","seed":L,"noise":{...},"scale":F,"states":[...]}
 *   {"type":"minecraft:dual_noise_provider","variety":{"min_inclusive":1,"max_inclusive":64},
 *    "slow_noise":{...},"slow_scale":F,"seed":L,"noise":{...},"scale":F,"states":[...]}
 *   {"type":"minecraft:randomized_int_state_provider","source":{...},"property":"age",
 *    "values":{...IntProvider...}}
 *
 * 运行时采样统一通过返回的多态指针调用 getState(world, random, x, y, z)。
 */
namespace BlockStateProviderParser {

/**
 * @brief 解析方块状态提供者
 * @return 多态基类指针；解析失败返回错误
 */
[[nodiscard]] Result<std::unique_ptr<state::BlockStateProvider>> parse(const nlohmann::json& providerObj);

/**
 * @brief 解析 rule_based_state_provider（无 type 字段）
 *
 * 对应 MC 1.21.11 RuleBasedBlockStateProvider.CODEC：仅 {fallback, rules}，
 * 无 "type" 字段。DiskConfiguration.stateProvider 等直接持有此类型，JSON 不写 type。
 */
[[nodiscard]] Result<std::unique_ptr<state::BlockStateProvider>> parseRuleBased(const nlohmann::json& providerObj);

} // namespace BlockStateProviderParser

} // namespace mc::world::gen::feature::parser
