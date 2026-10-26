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
#include "common/item/component/DataComponentPatchNbt.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 复制组件函数（minecraft:copy_components）
 *
 * 对应原版 CopyComponentsFunction：把来源（方块实体或本实体）的组件复制到物品上。
 * include / exclude 以组件类型名（"minecraft:custom_name" 或省略命名空间）指定，
 * 两者至多指定其一；都未指定时复制全部组件。
 *
 * JSON：{ "source": "block_entity", "include": ["minecraft:custom_name"] }
 */
class CopyComponentsFunction : public LootFunction {
public:
    /// 组件来源（原版 LootContextArg：block_entity 或 this）
    enum class Source : u8 {
        BlockEntity,
        This,
    };

    CopyComponentsFunction(Source source,
        std::vector<item::component::DataComponentType> include,
        std::vector<item::component::DataComponentType> exclude);

    [[nodiscard]] ItemStack run(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "copy_components";
    }

    [[nodiscard]] Source getSource() const
    {
        return m_source;
    }

    [[nodiscard]] const std::vector<item::component::DataComponentType>& getInclude() const
    {
        return m_include;
    }

    [[nodiscard]] const std::vector<item::component::DataComponentType>& getExclude() const
    {
        return m_exclude;
    }

private:
    Source m_source;
    std::vector<item::component::DataComponentType> m_include;
    std::vector<item::component::DataComponentType> m_exclude;
};

} // namespace loot
} // namespace mc
