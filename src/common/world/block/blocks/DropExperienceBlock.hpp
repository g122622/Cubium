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
#include "common/util/assert/AssertAll.hpp"
#include "world/block/blocks/SimpleBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 破坏时掉落经验的方块（vanilla `DropExperienceBlock`）
 *
 * 原版由 Java 侧的子类登记（`Blocks.java` 里 `new DropExperienceBlock(UniformInt.of(3, 7), ...)`），
 * 区间只存在于代码而不在数据包里；Cubium 的方块由数据包注册，经验区间因此在这里以两个
 * 整数重新登记，语义与原版的 IntProvider 一致：
 *
 * - `min == max`：固定值（原版 `ConstantInt`），**不消耗随机数**；
 * - `min < max`：闭区间均匀采样（原版 `UniformInt`），消耗一次随机数。
 *
 * 目前原版只用到这两种形态（见 `blocks.java` 全部 17 个 DropExperienceBlock），
 * 出现新的分布形态时再扩展 `IntProvider` 成员。
 */
class DropExperienceBlock : public SimpleBlock {
public:
    DropExperienceBlock(const BlockProperties& properties, i32 minExperience, i32 maxExperience)
        : SimpleBlock(properties)
        , m_minExperience(minExperience)
        , m_maxExperience(maxExperience)
    {
        MC_ASSERT_RELEASE_MSG(minExperience >= 0 && minExperience <= maxExperience,
            "DropExperienceBlock 的经验区间必须满足 0 <= min <= max");
    }

    [[nodiscard]] i32 dropExperience(math::IRandom& random) const override;

    [[nodiscard]] i32 minExperience() const noexcept
    {
        return m_minExperience;
    }

    [[nodiscard]] i32 maxExperience() const noexcept
    {
        return m_maxExperience;
    }

private:
    i32 m_minExperience;
    i32 m_maxExperience;
};

} // namespace blocks
} // namespace mc