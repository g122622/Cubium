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
#include "common/world/gen/surface/SurfaceCondition.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::surface {

// ============================================================================
// SurfaceRule — MC 1.21 SurfaceRules 规则接口
// ============================================================================

/**
 * @brief SurfaceRules 规则基类
 *
 * 规则在满足条件时返回替换方块状态，否则返回 nullptr。
 */
class SurfaceRule {
public:
    virtual ~SurfaceRule() = default;

    /** 尝试在指定位置应用规则，返回方块状态或 nullptr */
    [[nodiscard]] virtual const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const = 0;
};

/** 条件规则：if 条件为真则应用 */
class IfTrueRule final : public SurfaceRule {
public:
    IfTrueRule(std::unique_ptr<SurfaceCondition> condition, std::unique_ptr<SurfaceRule> thenRule)
        : m_condition(std::move(condition))
        , m_thenRule(std::move(thenRule))
    {}

    [[nodiscard]] const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const override
    {
        if (m_condition->test(ctx)) {
            return m_thenRule->tryApply(blockX, blockY, blockZ, ctx);
        }
        return nullptr;
    }

private:
    std::unique_ptr<SurfaceCondition> m_condition;
    std::unique_ptr<SurfaceRule> m_thenRule;
};

/** 序列规则：按顺序尝试规则，返回第一个非空结果 */
class SequenceRule final : public SurfaceRule {
public:
    explicit SequenceRule(std::vector<std::unique_ptr<SurfaceRule>> rules)
        : m_rules(std::move(rules))
    {}

    [[nodiscard]] const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const override
    {
        for (const auto& rule : m_rules) {
            const BlockState* result = rule->tryApply(blockX, blockY, blockZ, ctx);
            if (result != nullptr) {
                return result;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<SurfaceRule>> m_rules;
};

/** 方块规则：返回固定方块状态 */
class BlockRule final : public SurfaceRule {
public:
    explicit BlockRule(const BlockState* blockState)
        : m_blockState(blockState)
    {}

    [[nodiscard]] const BlockState* tryApply(i32, i32, i32, const SurfaceRuleContext&) const override
    {
        return m_blockState;
    }

private:
    const BlockState* m_blockState;
};

/** Bandlands 规则（MC: Badlands 陶土带） */
class BandlandsRule final : public SurfaceRule {
public:
    [[nodiscard]] const BlockState* tryApply(
        i32 blockX, i32 blockY, i32 blockZ, const SurfaceRuleContext& ctx) const override;
};

} // namespace mc::world::gen::surface
