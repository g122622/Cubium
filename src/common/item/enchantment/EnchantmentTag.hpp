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

#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <unordered_set>
#include <vector>

namespace mc {

/**
 * @brief 附魔标签（数据包 `data/minecraft/tags/enchantment/` 下的 JSON）
 *
 * 对应原版 `TagKey<Enchantment>`。原版用标签表达附魔的获取途径与行为开关：
 * `treasure`（只能从战利品/交易得到）、`curse`、`tradeable`、
 * `in_enchanting_table`、`on_random_loot`、`prevents_ice_melting`、
 * `tooltip_order`（**有序**）等 15 个。
 *
 * 成员同时保存为**有序序列**与集合：原版 `HolderSet.Named` 在绑定后保留标签 JSON 的
 * 声明顺序，`tooltip_order` 正是依赖这一顺序（提示框按标签顺序逐项列出附魔，标签外的
 * 附魔排在后面）。仅靠集合会丢失顺序，故这里两份都留。
 *
 * 用法示例:
 * @code
 * if (const auto* tag = EnchantmentTags::getTag(ResourceLocation("minecraft:treasure"))) {
 *     if (tag->contains(ResourceLocation("minecraft:mending"))) { ... }
 * }
 * @endcode
 */
class EnchantmentTag {
public:
    /**
     * @brief 构造附魔标签
     * @param id 标签资源位置
     */
    explicit EnchantmentTag(ResourceLocation id) noexcept;

    /**
     * @brief 获取标签 ID
     */
    [[nodiscard]] const ResourceLocation& getId() const
    {
        return m_id;
    }

    /**
     * @brief 添加附魔到标签
     *
     * 保持首次加入的顺序（原版 `HolderSet.Named` 绑定后不允许重复元素，重复加入是空操作）。
     *
     * @param enchantmentId 附魔资源位置（如 "minecraft:sharpness"）
     */
    void add(const ResourceLocation& enchantmentId);

    /**
     * @brief 检查附魔是否在标签中
     */
    [[nodiscard]] bool contains(const ResourceLocation& enchantmentId) const noexcept;

    /**
     * @brief 检查附魔是否在标签中
     * @param enchantmentId 附魔名称字符串（如 "minecraft:sharpness"）
     */
    [[nodiscard]] bool contains(const std::string& enchantmentId) const;

    /**
     * @brief 清空标签成员
     */
    void clear();

    /**
     * @brief 获取标签中的全部附魔 ID（集合语义，无序）
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& getEnchantmentIds() const
    {
        return m_enchantmentIds;
    }

    /**
     * @brief 获取标签中的全部附魔 ID（**按标签声明顺序**排列）
     *
     * 供依赖顺序的语义使用（目前是 `tooltip_order`）。集合 `getEnchantmentIds()` 只适合判存在。
     */
    [[nodiscard]] const std::vector<ResourceLocation>& getOrderedEnchantmentIds() const
    {
        return m_orderedEnchantmentIds;
    }

private:
    ResourceLocation m_id;
    std::unordered_set<ResourceLocation> m_enchantmentIds;
    std::vector<ResourceLocation> m_orderedEnchantmentIds;
};

} // namespace mc
