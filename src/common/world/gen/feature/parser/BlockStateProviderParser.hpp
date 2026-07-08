/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights
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
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include <memory>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// 前向声明
class IWorld;

namespace world {
namespace gen {
namespace feature {
namespace parser {

/**
 * @brief 方块状态提供者解析结果
 *
 * 项目的 SimpleBlockStateProvider 与 WeightedBlockStateProvider 不共享基类，
 * 故本结构用 kind 判别并分别持有其中一种。调用方按 kind 取用：
 * - kind==Simple：simple 持有固定 BlockState*
 * - kind==Weighted：weighted 持有加权提供者
 * - kind==RuleBased：ruleBased 持有 fallback + 规则列表（每条规则 = 谓词 + 子提供者）
 */
struct BlockStateProviderHandle {
    enum class Kind { Simple, Weighted, RuleBased };
    Kind kind = Kind::Simple;
    const BlockState* simple = nullptr;
    std::unique_ptr<state::WeightedBlockStateProvider> weighted;

    /// rule_based 的一条规则：if_true 谓词命中时取 then 提供者的状态。
    struct Rule {
        std::unique_ptr<predicate::BlockPredicate> ifTrue;
        std::unique_ptr<BlockStateProviderHandle> then;
    };
    struct RuleBasedData {
        std::unique_ptr<BlockStateProviderHandle> fallback;
        std::vector<Rule> rules;
    };
    std::unique_ptr<RuleBasedData> ruleBased;

    /**
     * @brief 取单一状态：Simple 直接返回 simple；Weighted/RuleBased 返回 nullptr（需随机源/世界）。
     */
    [[nodiscard]] const BlockState* asSingle() const noexcept;
};

/**
 * @brief 方块状态提供者 JSON 解析器
 *
 * 解析数据包中的 BlockStateProvider 对象：
 *   {"type":"minecraft:simple_state_provider","state":{"Name":...,"Properties":{...}}}
 *   {"type":"minecraft:weighted_state_provider","entries":[{"data":{...},"weight":N}, ...]}
 *   {"type":"minecraft:rule_based_state_provider",
 *    "fallback":<BlockStateProvider>, "rules":[{"if_true":<BlockPredicate>, "then":<BlockStateProvider>}, ...]}
 *
 * 支持 simple_state_provider / weighted_state_provider / rule_based_state_provider。
 * 其它 type（rotated_block_state_provider / noise_provider / randomized_int_state_provider 等）
 * 当前严格报错。
 */
namespace BlockStateProviderParser {

/**
 * @brief 解析方块状态提供者
 */
[[nodiscard]] Result<BlockStateProviderHandle> parse(const nlohmann::json& providerObj);

/**
 * @brief 从提供者采样一个方块状态
 *
 * - Simple：返回 simple；
 * - Weighted：按权重采样（仅需随机源）；
 * - RuleBased：按 rules 顺序，第一个 if_true.test(world, pos) 命中的取其 then 采样，
 *   否则取 fallback 采样。对齐 MC RuleBasedBlockStateProvider.getState。
 *
 * @param handle 提供者句柄（非 null）
 * @param world 世界（用于 RuleBased 谓词测试）
 * @param random 随机源（用于 Weighted 采样）
 * @param pos 测试位置（RuleBased 谓词在此处测试）
 * @return 采样的方块状态；提供者无可用状态时返回 nullptr
 */
[[nodiscard]] const BlockState* sampleState(
    const BlockStateProviderHandle& handle, const IWorld& world, math::IRandom& random, const BlockPos& pos);

} // namespace BlockStateProviderParser

} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
