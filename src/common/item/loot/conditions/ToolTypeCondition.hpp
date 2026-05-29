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

#include <memory>
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace loot {

/**
 * @brief 工具类型条件
 *
 * 检查使用的工具是否为指定类型。
 * 用于需要特定工具才能获得掉落的情况。
 */
class ToolTypeCondition : public LootCondition {
public:
    /**
     * @brief 构造工具类型条件
     * @param toolType 工具类型（HarvestTool 常量）
     */
    explicit ToolTypeCondition(u8 toolType);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const override;
    [[nodiscard]] std::string getType() const override { return "match_tool"; }

    [[nodiscard]] u8 getToolType() const { return m_toolType; }

private:
    u8 m_toolType;
};

} // namespace loot
} // namespace mc
