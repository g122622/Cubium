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
 * copies of substantial portions of the Software.
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
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace ruletest {

/**
 * @brief 位置规则测试基类
 *
 * 用于结构模板处理器中，根据位置条件决定是否应用替换规则。
 */
class PosRuleTest {
public:
    virtual ~PosRuleTest() = default;

    /**
     * @brief 测试位置是否匹配
     * @param originalPos 原始位置（模板内）
     * @param worldPos 世界位置
     * @param seedPos 种子位置（结构起点）
     * @param rng 随机数生成器
     * @return 是否匹配
     */
    [[nodiscard]] virtual bool test(
        const BlockPos& originalPos, const BlockPos& worldPos, const BlockPos& seedPos, math::Random& rng) const = 0;

    /**
     * @brief 克隆测试
     */
    [[nodiscard]] virtual std::unique_ptr<PosRuleTest> clone() const = 0;
};

/**
 * @brief 总是返回 true 的位置规则测试
 */
class AlwaysTruePosRuleTest : public PosRuleTest {
public:
    AlwaysTruePosRuleTest() = default;

    [[nodiscard]] bool test(const BlockPos& /*originalPos*/,
        const BlockPos& /*worldPos*/,
        const BlockPos& /*seedPos*/,
        math::Random& /*rng*/) const override
    {
        return true;
    }

    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override
    {
        return std::make_unique<AlwaysTruePosRuleTest>();
    }
};

/**
 * @brief 线性距离位置规则测试
 *
 * 根据 worldPos 到 seedPos 的曼哈顿距离线性插值概率
 */
class LinearPosRuleTest : public PosRuleTest {
public:
    LinearPosRuleTest(i32 minDistance, i32 maxDistance, f32 minProbability, f32 maxProbability);

    [[nodiscard]] bool test(const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const override;

    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override
    {
        return std::make_unique<LinearPosRuleTest>(m_minDistance, m_maxDistance, m_minProbability, m_maxProbability);
    }

private:
    i32 m_minDistance;
    i32 m_maxDistance;
    f32 m_minProbability;
    f32 m_maxProbability;
};

/**
 * @brief 轴对齐线性位置规则测试
 *
 * 根据指定轴方向上的距离线性插值概率
 */
class AxisAlignedLinearPosTest : public PosRuleTest {
public:
    AxisAlignedLinearPosTest(f32 minProbability, f32 maxProbability, i32 minDistance, i32 maxDistance, Axis axis);

    [[nodiscard]] bool test(const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const override;

    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override
    {
        return std::make_unique<AxisAlignedLinearPosTest>(
            m_minProbability, m_maxProbability, m_minDistance, m_maxDistance, m_axis);
    }

private:
    f32 m_minProbability;
    f32 m_maxProbability;
    i32 m_minDistance;
    i32 m_maxDistance;
    Axis m_axis;
};

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
