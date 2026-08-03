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

#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/core/ItemStack.hpp"
#include "item/crafting/SpecialRecipe.hpp"
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 书复制配方
 *
 * 允许玩家使用成书和书与笔复制书的内容。
 * 每次复制会增加书的代数（generation），最多复制到第二代。
 *
 * 参考: net.minecraft.item.crafting.BookCloningRecipe
 *
 * 匹配条件：
 * - 必须有一本已写入的书（WrittenBookItem）
 * - 必须有至少一本书与笔（WritableBookItem）
 * - 原书必须有 NBT 数据
 *
 * 剩余物品：
 * - 原书保留（不会被消耗）
 * - 书与笔被消耗
 */
class BookCloningRecipe : public SpecialRecipe {
public:
    explicit BookCloningRecipe(const ResourceLocation& id);

    /**
     * @brief 检查是否匹配书复制配方
     * @param inventory 合成网格
     * @return 如果有一本成书和至少一本空书返回 true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成复制的书
     * @param inventory 合成网格
     * @return 复制的书堆（数量等于空书数量）
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 原书保留在原位置
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 检查物品是否为成书
     * @param stack 物品堆
     * @return 如果是成书返回 true
     */
    [[nodiscard]] static bool _isWrittenBook(const ItemStack& stack);

    /**
     * @brief 检查物品是否为书与笔
     * @param stack 物品堆
     * @return 如果是书与笔返回 true
     */
    [[nodiscard]] static bool _isWritableBook(const ItemStack& stack);

    /**
     * @brief 获取书的代数
     * @param stack 物品堆
     * @return 代数（0=原版, 1=副本, 2=副本的副本）
     */
    [[nodiscard]] static i32 _getGeneration(const ItemStack& stack);

    /**
     * @brief 设置书的代数
     * @param stack 物品堆
     * @param generation 代数
     */
    static void _setGeneration(ItemStack& stack, i32 generation);
};

} // namespace crafting
} // namespace mc
