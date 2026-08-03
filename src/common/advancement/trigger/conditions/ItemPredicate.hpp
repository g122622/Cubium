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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, WHETHER
 * EXPRESS OR IMPLIED, INCLUDING STATUTORY OR OTHERWISE, IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#pragma once

#include "EnchantmentPredicate.hpp"
#include "NBTPredicate.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class BlockState;
class ItemStack;
namespace item::items {
class EnchantedBookItem;
}
} // namespace mc

namespace mc::advancement {

/**
 * @brief 物品谓词
 *
 * 用于匹配物品的条件谓词，检查物品类型、数量、耐久、药水、附魔、NBT等。
 * 对应 MC Java 的 ItemPredicate。
 */
class ItemPredicate {
public:
    /**
     * @brief 默认构造（匹配任意物品）
     */
    ItemPredicate() = default;

    /**
     * @brief 构造物品谓词
     * @param item 物品ID
     * @param count 数量范围
     * @param durability 耐久范围
     * @param potion 药水类型
     * @param enchantments 附魔谓词列表
     * @param storedEnchantments 存储附魔谓词列表（用于附魔书）
     * @param nbt NBT谓词
     */
    ItemPredicate(std::optional<ResourceLocation> item,
        IntBounds count,
        IntBounds durability,
        std::optional<ResourceLocation> potion,
        std::vector<EnchantmentPredicate> enchantments,
        std::vector<EnchantmentPredicate> storedEnchantments,
        NBTPredicate nbt);

    /**
     * @brief 构造带标签的物品谓词
     *
     * 对应 MC Java 的 ItemPredicate，支持通过标签匹配物品。
     * item 和 tag 互斥：同时指定时，item 优先（与 MC Java 行为一致）。
     *
     * @param item 物品ID（可选，与 tag 互斥）
     * @param tag 物品标签ID（可选，与 item 互斥）
     * @param count 数量范围
     * @param durability 耐久范围
     * @param potion 药水类型
     * @param enchantments 附魔谓词列表
     * @param storedEnchantments 存储附魔谓词列表（用于附魔书）
     * @param nbt NBT谓词
     */
    ItemPredicate(std::optional<ResourceLocation> item,
        std::optional<ResourceLocation> tag,
        IntBounds count,
        IntBounds durability,
        std::optional<ResourceLocation> potion,
        std::vector<EnchantmentPredicate> enchantments,
        std::vector<EnchantmentPredicate> storedEnchantments,
        NBTPredicate nbt);

    /**
     * @brief 复制构造函数
     */
    ItemPredicate(const ItemPredicate& other) = default;

    /**
     * @brief 复制赋值运算符
     */
    ItemPredicate& operator=(const ItemPredicate& other) = default;

    /**
     * @brief 移动构造函数
     */
    ItemPredicate(ItemPredicate&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     */
    ItemPredicate& operator=(ItemPredicate&& other) noexcept = default;

    /**
     * @brief 检查物品是否匹配
     * @param stack 物品堆
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const ItemStack& stack) const;

    /**
     * @brief 检查是否匹配任意物品
     */
    [[nodiscard]] bool isAny() const noexcept;

    /**
     * @brief 从JSON解析
     */
    static Result<ItemPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::optional<ResourceLocation>& getItem() const noexcept { return m_item; }
    [[nodiscard]] const std::optional<ResourceLocation>& getTag() const noexcept { return m_tag; }
    [[nodiscard]] const IntBounds& getCount() const noexcept { return m_count; }
    [[nodiscard]] const IntBounds& getDurability() const noexcept { return m_durability; }
    [[nodiscard]] const std::optional<ResourceLocation>& getPotion() const noexcept { return m_potion; }
    [[nodiscard]] const std::vector<EnchantmentPredicate>& getEnchantments() const noexcept { return m_enchantments; }
    [[nodiscard]] const std::vector<EnchantmentPredicate>& getStoredEnchantments() const noexcept
    {
        return m_storedEnchantments;
    }
    [[nodiscard]] const NBTPredicate& getNbt() const noexcept { return m_nbt; }

    // ========== Setters ==========

    void setNbt(NBTPredicate nbt)
    {
        m_nbt = std::move(nbt);
        _updateIsAny();
    }

private:
    /**
     * @brief 更新 isAny 状态
     */
    void _updateIsAny();

    /**
     * @brief 检查附魔谓词列表是否匹配附魔容器
     * @param predicates 附魔谓词列表
     * @param enchantments 附魔容器
     * @return 是否匹配
     */
    static bool _testEnchantments(
        const std::vector<EnchantmentPredicate>& predicates, const item::enchant::EnchantmentContainer& enchantments);

    std::optional<ResourceLocation> m_item;                 ///< 物品ID
    std::optional<ResourceLocation> m_tag;                  ///< 物品标签ID（与 m_item 互斥）
    IntBounds m_count;                                      ///< 数量范围
    IntBounds m_durability;                                 ///< 耐久范围
    std::optional<ResourceLocation> m_potion;               ///< 药水类型
    std::vector<EnchantmentPredicate> m_enchantments;       ///< 附魔谓词列表
    std::vector<EnchantmentPredicate> m_storedEnchantments; ///< 存储附魔谓词列表（附魔书）
    NBTPredicate m_nbt;                                     ///< NBT谓词
    bool m_isAny = true;                                    ///< 是否匹配任意物品
};

} // namespace mc::advancement
