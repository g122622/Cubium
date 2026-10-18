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
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 设置组件函数（minecraft:set_components）
 *
 * 对应原版 SetComponentsFunction：把组件补丁（DataComponentPatch）应用到物品上。
 * 加载期即按数据包 JSON 解码为补丁；本实现无法解码的组件由加载器跳过并告警。
 *
 * JSON：{ "components": { "minecraft:trim": { ... }, "!minecraft:custom_name": {} } }
 */
class SetComponentsFunction : public LootFunction {
public:
    /**
     * @brief 以组件补丁构造
     *
     * 补丁以共享指针持有：组件载荷（容器/投射物等）的拷贝与析构需要完整 ItemStack，
     * 共享持有让函数对象可以在任何 TU 中安全拷贝/析构。
     */
    explicit SetComponentsFunction(std::shared_ptr<const item::component::DataComponentPatch> patch);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "set_components"; }

    [[nodiscard]] const item::component::DataComponentPatch& getComponents() const { return *m_patch; }

private:
    std::shared_ptr<const item::component::DataComponentPatch> m_patch;
};

} // namespace loot
} // namespace mc
