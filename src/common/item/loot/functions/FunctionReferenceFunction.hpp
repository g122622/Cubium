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

#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 命名函数引用（minecraft:reference）
 *
 * 对应原版 FunctionReference：引用注册在函数管理器中的命名函数（LootItemFunctions 注册表）。
 * 函数在构造时解析，找不到目标时构造失败（加载期报错）。
 *
 * JSON：{ "name": "minecraft:my_function" }
 */
class FunctionReferenceFunction : public LootFunction {
public:
    /**
     * @brief 创建引用函数
     * @param name 被引用函数的资源位置
     * @param target 目标函数（引用保持存活，由调用方保证生命周期）
     */
    FunctionReferenceFunction(std::string name, const LootFunction* target);

    [[nodiscard]] ItemStack run(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "reference";
    }

    [[nodiscard]] const std::string& getName() const
    {
        return m_name;
    }

private:
    std::string m_name;
    const LootFunction* m_target;
};

} // namespace loot
} // namespace mc
