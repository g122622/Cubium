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

#include "LootEntry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <functional>
#include <memory>
#include <string>

namespace mc {
namespace loot {

/**
 * @brief 动态掉落条目
 *
 * 根据动态名称从上下文中获取物品。
 * 目前仅支持 "minecraft:contents"，用于从方块实体容器中读取物品。
 */
class DynamicLootEntry : public LootEntry {
public:
    /**
     * @brief 构造动态条目
     * @param name 动态名称（如 "minecraft:contents"）
     * @param weight 权重
     * @param quality 质量
     */
    DynamicLootEntry(const std::string& name, i32 weight = 1, i32 quality = 0);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Dynamic; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    [[nodiscard]] const std::string& getName() const { return m_name; }

    void expand(LootContext& context, std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const override;

private:
    std::string m_name;
};

} // namespace loot
} // namespace mc
