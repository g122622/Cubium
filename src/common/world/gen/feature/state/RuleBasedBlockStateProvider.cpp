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

#include "RuleBasedBlockStateProvider.hpp"

#include "common/world/block/BlockPos.hpp"

#include <utility>

namespace mc::world::gen::feature::state {

RuleBasedBlockStateProvider::Rule RuleBasedBlockStateProvider::Rule::clone() const
{
    std::unique_ptr<predicate::BlockPredicate> pred = ifTrue ? ifTrue->clone() : nullptr;
    std::unique_ptr<BlockStateProvider> thenProvider = then ? then->clone() : nullptr;
    return Rule(std::move(pred), std::move(thenProvider));
}

RuleBasedBlockStateProvider::RuleBasedBlockStateProvider(
    std::unique_ptr<BlockStateProvider> fallback, std::vector<Rule> rules)
    : m_fallback(std::move(fallback))
    , m_rules(std::move(rules))
{}

const BlockState* RuleBasedBlockStateProvider::getState(
    const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const
{
    const BlockPos pos(x, y, z);
    // 按 rules 顺序找第一个命中的谓词，取其 then 采样。
    for (const auto& rule : m_rules) {
        if (rule.ifTrue != nullptr && rule.ifTrue->test(world, pos)) {
            return (rule.then != nullptr) ? rule.then->getState(world, random, x, y, z) : nullptr;
        }
    }
    // 全部未命中 → fallback 采样。
    return (m_fallback != nullptr) ? m_fallback->getState(world, random, x, y, z) : nullptr;
}

std::unique_ptr<BlockStateProvider> RuleBasedBlockStateProvider::clone() const
{
    std::vector<Rule> rulesCopy;
    rulesCopy.reserve(m_rules.size());
    for (const auto& rule : m_rules) {
        rulesCopy.push_back(rule.clone());
    }
    return std::make_unique<RuleBasedBlockStateProvider>(
        m_fallback ? m_fallback->clone() : nullptr, std::move(rulesCopy));
}

} // namespace mc::world::gen::feature::state
