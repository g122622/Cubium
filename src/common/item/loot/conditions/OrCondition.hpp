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

#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 或条件
 *
 * 任一子条件满足即满足。用于组合多个条件形成逻辑或关系。
 */
class OrCondition : public LootCondition {
public:
    OrCondition() = default;

    /**
     * @brief 构造函数
     * @param conditions 子条件列表
     */
    explicit OrCondition(std::vector<std::unique_ptr<LootCondition>> conditions);

    /**
     * @brief 测试任一子条件是否满足
     * @param context 掉落上下文
     * @return 任一子条件满足返回true，否则返回false
     */
    [[nodiscard]] bool test(LootContext& context) const override;

    /**
     * @brief 创建条件副本
     * @return 当前条件的深拷贝
     */
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;

    /**
     * @brief 获取条件类型标识
     * @return 类型标识字符串 "or"
     */
    [[nodiscard]] std::string getType() const override { return "or"; }

    /**
     * @brief 添加子条件
     * @param condition 要添加的条件
     */
    void addCondition(std::unique_ptr<LootCondition> condition);

    /**
     * @brief 获取所有子条件
     * @return 子条件列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootCondition>>& getConditions() const { return m_conditions; }

private:
    std::vector<std::unique_ptr<LootCondition>> m_conditions;
};

} // namespace loot
} // namespace mc
