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

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::feature::state {

/**
 * @brief 基于规则的方块状态提供者
 *
 * 按 rules 顺序逐条测试 if_true 谓词，首个命中的取其 then 提供者采样；
 * 全部未命中则取 fallback 提供者采样。
 *
 * 对应 MC 1.21.11 RuleBasedBlockStateProvider。本类型既可由带 "type":"rule_based_state_provider"
 * 的对象解析，也可由无 type 字段的 {fallback, rules} 解析（DiskConfiguration 直接持有）。
 */
class RuleBasedBlockStateProvider : public BlockStateProvider {
public:
    /// 一条规则：if_true 谓词命中时取 then 提供者的状态。
    struct Rule {
        std::unique_ptr<predicate::BlockPredicate> ifTrue;
        std::unique_ptr<BlockStateProvider> then;

        Rule() = default;
        Rule(std::unique_ptr<predicate::BlockPredicate> pred, std::unique_ptr<BlockStateProvider> thenProvider)
            : ifTrue(std::move(pred))
            , then(std::move(thenProvider))
        {}

        [[nodiscard]] Rule clone() const;
    };

    RuleBasedBlockStateProvider(std::unique_ptr<BlockStateProvider> fallback, std::vector<Rule> rules);

    [[nodiscard]] const BlockState* getState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const override;

    [[nodiscard]] std::unique_ptr<BlockStateProvider> clone() const override;

private:
    std::unique_ptr<BlockStateProvider> m_fallback;
    std::vector<Rule> m_rules;
};

} // namespace mc::world::gen::feature::state
