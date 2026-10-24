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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 设置附魔函数（minecraft:set_enchantments）
 *
 * 对应原版 SetEnchantmentsFunction：为物品（或附魔书）设置附魔等级。
 * add=false 时为设置语义（覆盖列表中每个附魔的等级，其余附魔保持不变）；
 * add=true 时在现有等级上叠加。附魔书物品会被转化为成书附魔书。
 *
 * JSON：{ "enchantments": { "minecraft:fortune": 3 }, "add": false }
 */
class SetEnchantmentsFunction : public LootFunction {
public:
    /// 单个附魔的等级生成器（等级值可能来自随机数提供器）
    struct Entry {
        ResourceLocation enchantmentId; ///< 附魔资源位置
        RandomValueRange level;         ///< 等级范围
    };

    SetEnchantmentsFunction(std::vector<Entry> enchantments, bool add);

    /**
     * @brief 设置 add 模式的等级生成器
     *
     * 默认取 Entry::level；earlier 版本解析器对 default 参数的支持见 _parseEnchantRandomly。
     */
    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "set_enchantments";
    }

    [[nodiscard]] const std::vector<Entry>& getEnchantments() const
    {
        return m_enchantments;
    }

    [[nodiscard]] bool isAdd() const
    {
        return m_add;
    }

private:
    std::vector<Entry> m_enchantments;
    bool m_add;
};

} // namespace loot
} // namespace mc
