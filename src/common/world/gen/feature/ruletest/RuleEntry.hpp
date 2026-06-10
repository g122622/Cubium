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

#include "PosRuleTest.hpp"
#include "RuleTest.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <optional>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace ruletest {

/**
 * @brief 规则条目
 *
 * 定义一个完整的替换规则：条件 + 输出。
 * 用于结构模板处理器（RuleStructureProcessor）中。
 */
class RuleEntry {
public:
    /**
     * @brief 构造规则条目
     * @param inputPredicate 输入方块测试（测试模板中的方块）
     * @param locationPredicate 位置方块测试（测试世界中的方块）
     * @param posPredicate 位置测试（可选，默认 AlwaysTrue）
     * @param outputStateId 输出方块状态ID
     * @param outputNbt 输出NBT数据（可选，用于方块实体）
     */
    RuleEntry(std::unique_ptr<RuleTest> inputPredicate,
        std::unique_ptr<RuleTest> locationPredicate,
        std::unique_ptr<PosRuleTest> posPredicate,
        u32 outputStateId,
        std::optional<nbt::tags::compound_tag> outputNbt = std::nullopt);

    RuleEntry(std::unique_ptr<RuleTest> inputPredicate, std::unique_ptr<RuleTest> locationPredicate, u32 outputStateId);

    /**
     * @brief 测试是否匹配规则
     * @param inputState 输入方块状态（模板中的）
     * @param locationState 位置方块状态（世界中的）
     * @param originalPos 原始位置
     * @param worldPos 世界位置
     * @param seedPos 种子位置
     * @param rng 随机数生成器
     */
    [[nodiscard]] bool matches(const BlockState& inputState,
        const BlockState& locationState,
        const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const;

    [[nodiscard]] u32 outputStateId() const { return m_outputStateId; }
    [[nodiscard]] const RuleTest* inputPredicate() const { return m_inputPredicate.get(); }
    [[nodiscard]] const RuleTest* locationPredicate() const { return m_locationPredicate.get(); }
    [[nodiscard]] const PosRuleTest* posPredicate() const { return m_posPredicate.get(); }
    [[nodiscard]] const std::optional<nbt::tags::compound_tag>& outputNbt() const { return m_outputNbt; }

    /**
     * @brief 深拷贝规则条目
     */
    [[nodiscard]] std::unique_ptr<RuleEntry> clone() const;

private:
    std::unique_ptr<RuleTest> m_inputPredicate;
    std::unique_ptr<RuleTest> m_locationPredicate;
    std::unique_ptr<PosRuleTest> m_posPredicate;
    u32 m_outputStateId;
    std::optional<nbt::tags::compound_tag> m_outputNbt;
};

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
