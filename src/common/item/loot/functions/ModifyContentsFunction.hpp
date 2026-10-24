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
 * @brief 修改容器内容函数（minecraft:modify_contents）
 *
 * 对应原版 ModifyContainerContents：把修饰函数应用到物品 container 组件中每个
 * 非空槽位的物品上（如箱子掉落物中每件物品单独掷时运）。
 *
 * JSON：{ "component": "minecraft:container", "modifier": { ... } }
 */
class ModifyContentsFunction : public LootFunction {
public:
    ModifyContentsFunction(std::string component, std::unique_ptr<LootFunction> modifier);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "modify_contents";
    }

    [[nodiscard]] const std::string& getComponent() const
    {
        return m_component;
    }

    [[nodiscard]] const LootFunction* getModifier() const
    {
        return m_modifier.get();
    }

private:
    std::string m_component;
    std::unique_ptr<LootFunction> m_modifier;
};

} // namespace loot
} // namespace mc
