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

namespace mc {
namespace loot {

/**
 * @brief 引用条件
 *
 * 引用另一个已注册的条件谓词（predicate），实现条件复用。
 * 目前谓词系统尚未完整实现，此条件作为占位。
 */
class ReferenceCondition : public LootCondition {
public:
    ReferenceCondition() noexcept = default;

    /**
     * @brief 构造引用条件
     * @param name 引用的谓词名称
     */
    explicit ReferenceCondition(const std::string& name);

    [[nodiscard]] bool test(LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "reference"; }

    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }

private:
    std::string m_name;
};

} // namespace loot
} // namespace mc
