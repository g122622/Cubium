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
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 设置乐器函数（minecraft:set_instrument）
 *
 * 对应原版 SetInstrumentFunction：从乐器标签（如 "#minecraft:regular_goat_horns"）
 * 中随机选择一个设置到 instrument 组件。
 *
 * 标签成员取自 `item::instrument::InstrumentTags`（内置占位 + 数据包 `tags/instrument/` 加载结果）。
 *
 * JSON：{ "options": "#minecraft:regular_goat_horns" }
 */
class SetInstrumentFunction : public LootFunction {
public:
    explicit SetInstrumentFunction(ResourceLocation instrumentTagId);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;

    [[nodiscard]] std::string getType() const noexcept override
    {
        return "set_instrument";
    }

    [[nodiscard]] const ResourceLocation& getOptionsTag() const
    {
        return m_instrumentTagId;
    }

    /**
     * @brief 查询乐器标签的成员列表
     *
     * @param tagId 乐器标签资源位置
     * @return 成员列表指针（按资源名升序）；标签未登记时返回 nullptr
     */
    [[nodiscard]] static const std::vector<ResourceLocation>* getInstrumentTag(const ResourceLocation& tagId);

private:
    ResourceLocation m_instrumentTagId;
};

} // namespace loot
} // namespace mc
