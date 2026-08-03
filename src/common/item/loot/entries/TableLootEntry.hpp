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

// 前向声明
class LootTable;

/**
 * @brief 掉落表引用条目
 *
 * 引用另一个掉落表。引用方式有两种：
 * - 外部引用：通过表 ID 从 LootTableManager 解析（m_tableId）。
 * - 内联表：直接内嵌一个完整 LootTable（m_inlineTable）。
 *
 * 参考: net.minecraft.world.level.storage.loot.entries.NestedLootTable
 */
class TableLootEntry : public LootEntry {
public:
    /**
     * @brief 构造外部引用型条目
     * @param tableId 引用的掉落表 ID
     * @param weight 权重
     * @param quality 质量（幸运值加成系数）
     */
    TableLootEntry(const std::string& tableId, i32 weight, i32 quality);

    /**
     * @brief 构造内联表型条目
     * @param inlineTable 内联的完整掉落表
     * @param weight 权重
     * @param quality 质量（幸运值加成系数）
     */
    TableLootEntry(std::unique_ptr<LootTable> inlineTable, i32 weight, i32 quality);

    [[nodiscard]] LootEntryType getType() const override { return LootEntryType::Table; }
    [[nodiscard]] std::unique_ptr<LootEntry> clone() const override;

    // 析构定义在 .cpp，确保 m_inlineTable 销毁时 LootTable 为完整类型
    ~TableLootEntry() override;

    [[nodiscard]] const std::string& getTableId() const { return m_tableId; }

    /**
     * @brief 是否为内联表型
     */
    [[nodiscard]] bool isInline() const noexcept { return m_inlineTable != nullptr; }

    /**
     * @brief 获取内联掉落表（仅内联型有效，外部引用型返回 nullptr）
     */
    [[nodiscard]] const LootTable* getInlineTable() const noexcept { return m_inlineTable.get(); }

    void expand(LootContext& context, std::function<void(LootEntry&)> consumer) const override;

    bool generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const override;

private:
    std::string m_tableId;
    std::unique_ptr<LootTable> m_inlineTable;
};

} // namespace loot
} // namespace mc
