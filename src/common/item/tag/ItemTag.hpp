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

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include <unordered_set>
#include <vector>

namespace mc {

class Item;

// Forward declaration for ItemStack (defined in item/core/ItemStack.hpp)
class ItemStack;

namespace item::tag {

/**
 * @brief 物品标签
 *
 * 用于将物品分组以便配方和功能判断。
 * 参考: net.minecraft.tags.ITag
 *
 * 用法示例:
 * @code
 * // 检查物品是否在标签中
 * if (item.isIn(ItemTags::PLANKS)) {
 *     // 物品是木板
 * }
 *
 * // 检查物品堆是否匹配标签
 * if (ItemTags::LOGS.contains(stack)) {
 *     // 物品堆是原木
 * }
 * @endcode
 */
class ItemTag {
public:
    /**
     * @brief 构造物品标签
     * @param id 标签资源位置
     */
    explicit ItemTag(ResourceLocation id);

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 添加物品到标签
     * @param item 物品指针
     */
    void add(const Item* item);

    /**
     * @brief 检查物品是否在标签中
     * @param item 物品指针
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const Item* item) const;

    /**
     * @brief 检查物品堆是否在标签中
     * @param stack 物品堆
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const ItemStack& stack) const;

    /**
     * @brief 获取标签中的所有物品
     */
    [[nodiscard]] const std::unordered_set<const Item*>& getItems() const { return m_items; }

    /**
     * @brief 获取标签中的所有物品（有序列表）
     */
    [[nodiscard]] std::vector<const Item*> getItemsList() const;

private:
    ResourceLocation m_id;
    std::unordered_set<const Item*> m_items;
};

} // namespace item::tag
} // namespace mc
