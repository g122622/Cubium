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

#include "StructureProcessor.hpp"
#include "common/world/gen/feature/ruletest/RuleEntry.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 规则结构处理器
 * 根据一组规则来替换方块，每条规则包含：
 * - inputPredicate: 测试输入方块（模板中的方块）
 * - locationPredicate: 测试位置方块（世界中已有的方块）
 * - posPredicate: 测试位置条件
 * - outputState: 匹配时的输出方块状态
 */
class RuleStructureProcessor : public StructureProcessor {
public:
    /**
     * @brief 构造规则处理器
     * @param rules 规则列表
     */
    explicit RuleStructureProcessor(std::vector<std::unique_ptr<ruletest::RuleEntry>> rules);

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& seedPos,
        const BlockPos& pos,
        const BlockInfo& rawBlockInfo,
        const BlockInfo& blockInfo,
        const PlacementSettings& settings) override;

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override;

    [[nodiscard]] const std::vector<std::unique_ptr<ruletest::RuleEntry>>& rules() const { return m_rules; }

private:
    std::vector<std::unique_ptr<ruletest::RuleEntry>> m_rules;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
