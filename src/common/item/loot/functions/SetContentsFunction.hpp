/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

class LootEntry;

/**
 * @brief 设置内容物函数
 *
 * 设置容器类物品的内容物，用于生成带有物品的容器（如箱子矿车）。
 *
 * 实现逻辑：
 * 1. 遍历所有 LootEntry，生成物品
 * 2. 将生成的物品序列化到 ItemStack 的 BlockEntityTag.Items 中
 */
class SetContentsFunction : public LootFunction {
public:
    SetContentsFunction() = default;
    ~SetContentsFunction() noexcept override;

    /**
     * @brief 添加内容物条目
     * @param entry 掉落条目
     */
    void addEntry(std::unique_ptr<LootEntry> entry);

    /**
     * @brief 获取所有内容物条目
     */
    [[nodiscard]] const std::vector<std::unique_ptr<LootEntry>>& getEntries() const { return m_entries; }

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "set_contents"; }

private:
    std::vector<std::unique_ptr<LootEntry>> m_entries;
};

} // namespace loot
} // namespace mc
