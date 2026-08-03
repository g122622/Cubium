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

#include "ArmorItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 可染色盔甲
 *
 * 皮革盔甲可以使用染料染色。
 * 参考: net.minecraft.item.DyeableArmorItem
 *
 * 颜色存储在物品的NBT标签中：
 * - tag: { display: { color: 0xFF0000 } }
 */
class DyeableArmorItem : public ArmorItem {
public:
    /**
     * @brief 构造可染色盔甲
     * @param material 盔甲材质
     * @param slot 盔甲槽位
     * @param properties 物品属性
     */
    DyeableArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties);

    // ========== 染色相关方法 ==========

    /**
     * @brief 获取物品颜色
     * @param stack 物品堆
     * @return 颜色值（ARGB格式），无颜色时返回默认颜色
     */
    [[nodiscard]] u32 getColor(const ItemStack& stack) const noexcept;

    /**
     * @brief 设置物品颜色
     * @param stack 物品堆
     * @param color 颜色值（ARGB格式）
     */
    static void setColor(ItemStack& stack, u32 color) noexcept;

    /**
     * @brief 清除物品颜色
     * @param stack 物品堆
     */
    static void clearColor(ItemStack& stack) noexcept;

    /**
     * @brief 检查物品是否有颜色
     * @param stack 物品堆
     * @return 是否有颜色
     */
    [[nodiscard]] static bool hasColor(const ItemStack& stack) noexcept;

    /**
     * @brief 获取默认颜色
     *
     * 皮革盔甲的默认颜色是 0xA06540（棕色）
     *
     * @return 默认颜色值
     */
    [[nodiscard]] virtual u32 getDefaultColor() const { return DEFAULT_COLOR; }

private:
    /// 皮革盔甲的默认颜色（棕色）
    static constexpr u32 DEFAULT_COLOR = 0xA06540;

    /// NBT标签键
    static constexpr const char* TAG_DISPLAY = "display";
    static constexpr const char* TAG_COLOR = "color";
};

} // namespace item::items
} // namespace mc
